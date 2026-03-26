#include "BackupSessionModel.h"

#include "backuprepository.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

BackupSessionModel::BackupSessionModel(QObject *parent)
    : QObject(parent)
{
    loadSessions();
}

QVariantList BackupSessionModel::sessions() const
{
    return m_sessions;
}

bool BackupSessionModel::rollingBack() const
{
    return m_rollingBack;
}

QString BackupSessionModel::statusMessage() const
{
    return m_statusMessage;
}

QStringList BackupSessionModel::rollbackLogs() const
{
    return m_rollbackLogs;
}

void BackupSessionModel::reload()
{
    loadSessions();
}

void BackupSessionModel::clearLogs()
{
    m_rollbackLogs.clear();
    emit rollbackLogsChanged();
}

bool BackupSessionModel::rollbackSession(int sessionId)
{
    if (m_rollingBack || sessionId <= 0) {
        return false;
    }

    clearLogs();
    setRollingBack(true);
    setStatusMessage("Memulai rollback session ID " + QString::number(sessionId) + "...");
    appendLog("Memulai rollback session ID " + QString::number(sessionId));

    QJsonObject session = BackupRepository::getInstance()->getSessionById(sessionId);
    const QString folderName = session.value("folder_name").toString();
    if (folderName.isEmpty()) {
        appendLog("ERROR: session tidak ditemukan");
        setStatusMessage("Rollback gagal: session tidak ditemukan.");
        setRollingBack(false);
        return false;
    }

    appendLog("Folder session: " + folderName);

    const QString rootWorkDir = QDir::homePath() + "/iStowV2/";
    QDir().mkpath(rootWorkDir);

    const QString backupSessionDir = QDir::homePath() + "/iStowV2/IstowUpdater/" + folderName;
    if (!QDir(backupSessionDir).exists()) {
        appendLog("ERROR: folder backup tidak ditemukan: " + backupSessionDir);
        setStatusMessage("Rollback gagal: folder backup tidak ditemukan.");
        setRollingBack(false);
        return false;
    }

    appendLog("Sumber backup: " + backupSessionDir);
    appendLog("Target restore: " + rootWorkDir);

    int restored = 0;
    int failed = 0;

    QDirIterator it(backupSessionDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString sourcePath = it.next();
        const QString relativePath = QDir(backupSessionDir).relativeFilePath(sourcePath);
        const QString targetPath = rootWorkDir + relativePath;
        const QString shortName = QFileInfo(sourcePath).fileName();

        if (relativePath.isEmpty() || !QFile::exists(sourcePath)) {
            appendLog("SKIP: file sumber tidak valid - " + shortName);
            failed++;
            continue;
        }

        QFileInfo targetInfo(targetPath);
        QDir().mkpath(targetInfo.absolutePath());

        if (QFile::exists(targetPath) && !QFile::remove(targetPath)) {
            appendLog("GAGAL replace: " + shortName);
            failed++;
            continue;
        }

        if (QFile::copy(sourcePath, targetPath)) {
            appendLog("RESTORE: " + relativePath);
            restored++;
        } else {
            appendLog("GAGAL copy: " + shortName);
            failed++;
        }
    }

    if (restored > 0) {
        BackupRepository::getInstance()->updateSessionStatus(sessionId, "rolled_back");
        appendLog("Status session diupdate menjadi rolled_back");
    }

    setStatusMessage(
        QString("Rollback selesai: %1 file dipulihkan, %2 file gagal.")
            .arg(restored)
            .arg(failed));
    appendLog(QString("Selesai: %1 file dipulihkan, %2 file gagal")
                  .arg(restored)
                  .arg(failed));

    setRollingBack(false);
    loadSessions();
    return restored > 0;
}

void BackupSessionModel::loadSessions()
{
    QJsonArray data = BackupRepository::getInstance()->getAllSessions();

    m_sessions.clear();
    for (const QJsonValue &value : data) {
        QJsonObject obj = value.toObject();
        QVariantMap map = obj.toVariantMap();

        qint64 createdAt = obj.value("created_at").toVariant().toLongLong();
        map.insert("created_at_text",
                   QDateTime::fromSecsSinceEpoch(createdAt).toString("yyyy-MM-dd HH:mm:ss"));

        m_sessions.append(map);
    }

    emit sessionsChanged();
}

void BackupSessionModel::appendLog(const QString &message)
{
    m_rollbackLogs.append(message);
    emit rollbackLogsChanged();
}

void BackupSessionModel::setRollingBack(bool value)
{
    if (m_rollingBack == value) {
        return;
    }

    m_rollingBack = value;
    emit rollingBackChanged();
}

void BackupSessionModel::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}