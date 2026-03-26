#include "InstallerModel.h"

#include "installerrepository.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcess>
#include <QUrl>

InstallerModel::InstallerModel(QObject *parent)
    : QObject(parent)
{
    loadInstallers();
}

QVariantList InstallerModel::installers() const
{
    return m_installers;
}

QString InstallerModel::statusMessage() const
{
    return m_statusMessage;
}

void InstallerModel::reload()
{
    loadInstallers();
}

bool InstallerModel::createInstaller(const QString &version,
                                     const QString &sourceFileUrl)
{
    const QString cleanVersion = version.trimmed();
    if (cleanVersion.isEmpty()) {
        setStatusMessage("Versi wajib diisi.");
        return false;
    }

    if (sourceFileUrl.trimmed().isEmpty()) {
        setStatusMessage("File installer .exe wajib dipilih.");
        return false;
    }

    bool copied = false;
    const QString copiedAbsolutePath = copyExeToInstallerFolder(sourceFileUrl, &copied);
    if (!copied) {
        return false;
    }

    const QString derivedInstallerName = QFileInfo(copiedAbsolutePath).completeBaseName();
    const QString relativePath = relativeInstallerPath(copiedAbsolutePath);
    const QVariant createdAt = QVariant(QDateTime::currentSecsSinceEpoch());

    InstallerRepository::getInstance()->insertInstaller(derivedInstallerName, relativePath, cleanVersion, createdAt);

    loadInstallers();
    setStatusMessage("Installer berhasil ditambahkan.");
    return true;
}

bool InstallerModel::updateInstaller(int id,
                                     const QString &version,
                                     const QString &sourceFileUrl)
{
    if (id <= 0) {
        setStatusMessage("Data installer tidak valid.");
        return false;
    }

    const QString cleanVersion = version.trimmed();
    if (cleanVersion.isEmpty()) {
        setStatusMessage("Versi wajib diisi.");
        return false;
    }

    QJsonObject current = InstallerRepository::getInstance()->getInstallerById(id);
    if (current.isEmpty()) {
        setStatusMessage("Data installer tidak ditemukan.");
        return false;
    }

    const QString currentInstallerPath = current.value("installer_path").toString().trimmed();
    QString installerPath = currentInstallerPath;
    QString installerName = current.value("installer_name").toString().trimmed();
    const bool hasExistingInstallerFile = !currentInstallerPath.isEmpty();

    if (sourceFileUrl.trimmed().isEmpty() && !hasExistingInstallerFile) {
        setStatusMessage("File installer .exe wajib dipilih untuk data tanpa file installer.");
        return false;
    }

    if (!sourceFileUrl.trimmed().isEmpty()) {
        bool copied = false;
        const QString copiedAbsolutePath = copyExeToInstallerFolder(sourceFileUrl, &copied);
        if (!copied) {
            return false;
        }
        installerPath = relativeInstallerPath(copiedAbsolutePath);
        installerName = QFileInfo(copiedAbsolutePath).completeBaseName();
    }

    const QVariant createdAt = QVariant(QDateTime::currentSecsSinceEpoch());

    InstallerRepository::getInstance()->updateInstaller(id, installerName, installerPath, cleanVersion, createdAt);

    loadInstallers();
    setStatusMessage("Installer berhasil diperbarui.");
    return true;
}

bool InstallerModel::deleteInstaller(int id)
{
    if (id <= 0) {
        setStatusMessage("Data installer tidak valid.");
        return false;
    }

    QJsonObject current = InstallerRepository::getInstance()->getInstallerById(id);
    if (current.isEmpty()) {
        setStatusMessage("Data installer tidak ditemukan.");
        return false;
    }

    const QString relativePath = current.value("installer_path").toString();
    const QString absolutePath = QDir(InstallerRepository::projectRootPath()).filePath(relativePath);

    if (QFile::exists(absolutePath)) {
        QFile::remove(absolutePath);
    }

    InstallerRepository::getInstance()->deleteInstaller(id);

    loadInstallers();
    setStatusMessage("Installer berhasil dihapus.");
    return true;
}

bool InstallerModel::installInstaller(int id)
{
    if (id <= 0) {
        setStatusMessage("Data installer tidak valid.");
        return false;
    }

    QJsonObject current = InstallerRepository::getInstance()->getInstallerById(id);
    if (current.isEmpty()) {
        setStatusMessage("Data installer tidak ditemukan.");
        return false;
    }

    const QString relativePath = current.value("installer_path").toString().trimmed();
    if (relativePath.isEmpty()) {
        setStatusMessage("Path installer kosong.");
        return false;
    }

    const QString absolutePath = QDir(InstallerRepository::projectRootPath()).filePath(relativePath);
    QFileInfo fileInfo(absolutePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        setStatusMessage("File installer tidak ditemukan: " + absolutePath);
        return false;
    }

    if (fileInfo.suffix().compare("exe", Qt::CaseInsensitive) != 0) {
        setStatusMessage("File installer harus berekstensi .exe.");
        return false;
    }

    const bool started = QProcess::startDetached(absolutePath, {});
    if (!started) {
        setStatusMessage("Gagal menjalankan installer.");
        return false;
    }

    setStatusMessage("Installer dijalankan: " + fileInfo.fileName());
    return true;
}

void InstallerModel::loadInstallers()
{
    QJsonArray data = InstallerRepository::getInstance()->getAllInstallers();

    m_installers.clear();
    for (const QJsonValue &value : data) {
        QJsonObject obj = value.toObject();
        QVariantMap map = obj.toVariantMap();

        const qint64 createdAt = obj.value("created_at").toVariant().toLongLong();
        map.insert("created_at_text", createdAt > 0
                                          ? QDateTime::fromSecsSinceEpoch(createdAt).toString("yyyy-MM-dd HH:mm:ss")
                                          : QString("-"));

        m_installers.append(map);
    }

    emit installersChanged();
}

QString InstallerModel::copyExeToInstallerFolder(const QString &sourceFileUrl, bool *ok)
{
    if (ok != nullptr) {
        *ok = false;
    }

    const QString sourcePath = localPathFromUrl(sourceFileUrl);
    QFileInfo sourceInfo(sourcePath);

    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        setStatusMessage("File installer tidak ditemukan.");
        return QString();
    }

    if (sourceInfo.suffix().compare("exe", Qt::CaseInsensitive) != 0) {
        setStatusMessage("Hanya file .exe yang diizinkan.");
        return QString();
    }

    QDir installerDir(InstallerRepository::installerFolderPath());
    if (!installerDir.exists()) {
        installerDir.mkpath(".");
    }

    QString targetPath = installerDir.filePath(sourceInfo.fileName());
    if (QFileInfo(targetPath).absoluteFilePath() == sourceInfo.absoluteFilePath()) {
        if (ok != nullptr) {
            *ok = true;
        }
        return targetPath;
    }

    if (QFile::exists(targetPath)) {
        const QString baseName = sourceInfo.completeBaseName();
        const QString extension = sourceInfo.suffix();
        int index = 1;
        do {
            targetPath = installerDir.filePath(
                QString("%1_%2.%3").arg(baseName).arg(index).arg(extension));
            index++;
        } while (QFile::exists(targetPath));
    }

    if (!QFile::copy(sourceInfo.absoluteFilePath(), targetPath)) {
        setStatusMessage("Gagal menyalin file installer ke folder project.");
        return QString();
    }

    if (ok != nullptr) {
        *ok = true;
    }

    return targetPath;
}

QString InstallerModel::localPathFromUrl(const QString &sourceFileUrl) const
{
    const QUrl url(sourceFileUrl);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }

    return sourceFileUrl;
}

QString InstallerModel::relativeInstallerPath(const QString &absolutePath) const
{
    QDir projectRoot(InstallerRepository::projectRootPath());
    return projectRoot.relativeFilePath(absolutePath);
}

void InstallerModel::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}
