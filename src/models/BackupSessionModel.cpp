#include "BackupSessionModel.h"

#include "backuprepository.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QCoreApplication>
#include <QXmlStreamReader>

BackupSessionModel::BackupSessionModel(QObject *parent)
    : QObject(parent)
{
    loadSessions();
}

QVariantList BackupSessionModel::sessions() const
{
    return m_sessions;
}

QVariantList BackupSessionModel::appSessions() const
{
    return m_appSessions;
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

bool BackupSessionModel::deleteSession(int sessionId)
{
    if (m_rollingBack || sessionId <= 0) {
        return false;
    }

    QJsonObject session = BackupRepository::getInstance()->getSessionById(sessionId);
    QString folderName = session.value("folder_name").toString();
    if (folderName.isEmpty()) {
        setStatusMessage("Hapus gagal: session tidak ditemukan di database.");
        return false;
    }

    const QString backupSessionDir = QDir::homePath() + "/iStowV2/IstowUpdater/" + folderName;
    QDir dir(backupSessionDir);
    if (dir.exists()) {
        if (!dir.removeRecursively()) {
            setStatusMessage("Hapus gagal: tidak dapat menghapus folder backup.");
            return false;
        }
    }

    BackupRepository::getInstance()->deleteSession(sessionId);
    setStatusMessage("Berhasil menghapus backup session: " + folderName);
    loadSessions();
    return true;
}

bool BackupSessionModel::rollbackAppSession(int sessionId, const QString &targetDirUrl)
{
    if (m_rollingBack || sessionId <= 0) {
        return false;
    }

    if(targetDirUrl.isEmpty()) {
        setStatusMessage("Rollback gagal: Direktori tujuan tidak valid.");
        return false;
    }

    QString rootWorkDir = targetDirUrl.isEmpty() ? (QDir::homePath() + "/iStowV2/") : QUrl(targetDirUrl).toLocalFile();
    if (!rootWorkDir.endsWith("/")) {
        rootWorkDir += "/";
    }

    clearLogs();
    setRollingBack(true);
    setStatusMessage("Memulai rollback aplikasi session ID " + QString::number(sessionId) + "...");
    appendLog("Memulai rollback aplikasi session ID " + QString::number(sessionId));

    QJsonObject session = BackupRepository::getInstance()->getAppSessionById(sessionId);
    const QString folderName = session.value("folder_name").toString();
    if (folderName.isEmpty()) {
        appendLog("ERROR: session aplikasi tidak ditemukan");
        setStatusMessage("Rollback gagal: session tidak ditemukan.");
        setRollingBack(false);
        return false;
    }

    appendLog("Folder session: " + folderName);

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

    if (!QFile::exists(rootWorkDir + "iStowV2.exe")) {
        appendLog("ERROR: Bukan folder iStow (tidak ada iStowV2.exe)");
        setStatusMessage("Rollback gagal: Direktori tujuan bukan folder iStow yang valid.");
        setRollingBack(false);
        return false;
    }

    appendLog("Menghapus seluruh file lama di " + rootWorkDir + "...");
    QDir targetD(rootWorkDir);
    targetD.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries | QDir::System);
    QFileInfoList listInfo = targetD.entryInfoList();
    for (const QFileInfo &fileInfo : listInfo) {
        if (fileInfo.isDir()) {
            if (fileInfo.fileName() != "IstowUpdater") {
                QDir dir(fileInfo.absoluteFilePath());
                dir.removeRecursively();
            }
        } else {
            QFile::remove(fileInfo.absoluteFilePath());
        }
    }
    QDir().mkpath(rootWorkDir);

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

        setStatusMessage("Restore file: " + relativePath);
        QCoreApplication::processEvents();

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
        BackupRepository::getInstance()->updateAppSessionStatus(sessionId, "rolled_back");
        appendLog("Status session aplikasi diupdate menjadi rolled_back");
    }

    setStatusMessage(
        QString("Rollback aplikasi selesai: %1 file dipulihkan, %2 file gagal.")
            .arg(restored)
            .arg(failed));
    appendLog(QString("Selesai: %1 file dipulihkan, %2 file gagal")
                  .arg(restored)
                  .arg(failed));

    setRollingBack(false);
    loadSessions();
    return restored > 0;
}

bool BackupSessionModel::backupOldIstow(const QString &directoryUrl)
{
    QString sourceDir = QUrl(directoryUrl).toLocalFile();
    if (sourceDir.isEmpty() || !QDir(sourceDir).exists()) {
        setStatusMessage("Backup gagal: Directory tidak valid.");
        return false;
    }

    if (!QFile::exists(sourceDir + "/iStowV2.exe")) {
        setStatusMessage("Backup gagal: Direktori folder iStow tidak valid (iStowV2.exe tidak ditemukan).");
        return false;
    }

    setStatusMessage("Memulai backup dari: " + sourceDir);

    QDir sourceQDir(sourceDir);
    QString baseFolderName = sourceQDir.dirName();
    QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString targetFolderName = baseFolderName + "_" + timeStamp;

    QString wDir = QDir::homePath() + "/iStowV2/";
    QString updaterDir = wDir + "IstowUpdater/";
    QString targetDir = updaterDir + targetFolderName;

    setStatusMessage("Menyiapkan folder backup: " + targetDir);

    if (!QDir().mkpath(targetDir)) {
        setStatusMessage("Backup gagal: Tidak dapat membuat direktori backup.");
        return false;
    }

    QString istowVersion = "unknown";
    QFile componentsFile(sourceDir + "/components.xml");
    if (componentsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QXmlStreamReader xml(&componentsFile);
        while (!xml.atEnd() && !xml.hasError()) {
            QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                if (xml.name() == QLatin1String("Version")) {
                    istowVersion = xml.readElementText();
                    break;
                }
            }
        }
        componentsFile.close();
    }

    setStatusMessage("Menyimpan sesi ke database...");
    int sessionId = BackupRepository::getInstance()->insertAppSession(targetFolderName, "iStow", istowVersion);
    int copied = 0;
    int failed = 0;

    QDirIterator it(sourceDir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString fileSrc = it.next();
        QString relativePath = sourceQDir.relativeFilePath(fileSrc);
        QString fileTarget = targetDir + "/" + relativePath;

        QFileInfo targetInfo(fileTarget);
        QDir().mkpath(targetInfo.absolutePath());

        setStatusMessage("Mengkopi file: " + relativePath);
        QCoreApplication::processEvents();

        if (QFile::copy(fileSrc, fileTarget)) {
            BackupRepository::getInstance()->insertAppRecord(sessionId, fileSrc, fileTarget, "backup_old");
            copied++;
        } else {
            setStatusMessage("Gagal mengkopi file: " + relativePath);
            QCoreApplication::processEvents();
            failed++;
        }
    }

    loadSessions();
    setStatusMessage(QString("Backup selesai: %1 file berhasil, %2 file gagal.").arg(copied).arg(failed));
    return copied > 0;
}

bool BackupSessionModel::uninstallOldIstow(const QString &directoryUrl)
{
    QString sourceDir = QUrl(directoryUrl).toLocalFile();
    if (sourceDir.isEmpty() || !QDir(sourceDir).exists()) {
        setStatusMessage("Uninstall gagal: Direktori tidak valid atau tidak ditemukan.");
        return false;
    }

    if (!QFile::exists(sourceDir + "/iStowV2.exe")) {
        setStatusMessage("Uninstall gagal: Direktori tujuan bukan folder iStow (iStowV2.exe tidak ditemukan).");
        return false;
    }

    setStatusMessage("Memulai uninstall iStow dari: " + sourceDir);
    appendLog("Menghapus direktori: " + sourceDir);

    QDir dir(sourceDir);
    if (dir.removeRecursively()) {
        setStatusMessage("Uninstall selesai: Direktori berhasil dihapus.");
        appendLog("Direktori berhasil dihapus.");
        return true;
    } else {
        setStatusMessage("Uninstall gagal: Tidak dapat menghapus semua file di direktori (mungkin ada yang sedang digunakan).");
        appendLog("Gagal menghapus direktori.");
        return false;
    }
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
    
    QJsonArray appData = BackupRepository::getInstance()->getAllAppSessions();
    m_appSessions.clear();
    for (const QJsonValue &value : appData) {
        QJsonObject obj = value.toObject();
        QVariantMap map = obj.toVariantMap();
        qint64 createdAt = obj.value("created_at").toVariant().toLongLong();
        map.insert("created_at_text", QDateTime::fromSecsSinceEpoch(createdAt).toString("yyyy-MM-dd HH:mm:ss"));
        m_appSessions.append(map);
    }

    emit sessionsChanged();
    emit appSessionsChanged();
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