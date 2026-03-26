#include "installerrepository.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QVariant>

InstallerRepository *InstallerRepository::installerRepository = nullptr;

InstallerRepository::InstallerRepository(QString pathdb, QString conndb)
    : Repository(pathdb, conndb)
{
}

InstallerRepository *InstallerRepository::getInstance()
{
    if (installerRepository == nullptr) {
        QDir projectDir(projectRootPath());
        projectDir.mkpath("data");
        projectDir.mkpath("istow_installer");

        const QString dbPath = projectDir.filePath("data/istow_installer.sqlite");
        installerRepository = new InstallerRepository(dbPath, "INSTALLERDB");
        installerRepository->init();
    }

    return installerRepository;
}

QString InstallerRepository::projectRootPath()
{
    QDir projectDir(QCoreApplication::applicationDirPath());

    // Saat run dari build/.../Desktop_Qt_... naik ke root project.
    if (projectDir.dirName().contains("Desktop_Qt_")) {
        projectDir.cdUp();
        projectDir.cdUp();
    }

    return projectDir.absolutePath();
}

QString InstallerRepository::installerFolderPath()
{
    return QDir(projectRootPath()).filePath("istow_installer");
}

QString InstallerRepository::escapeSql(const QString &value)
{
    QString escaped = value;
    escaped.replace("'", "''");
    return escaped;
}

void InstallerRepository::init()
{
    const QString query = QString(
        "CREATE TABLE IF NOT EXISTS installer_files("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "installer_name TEXT NOT NULL, "
        "installer_path TEXT NOT NULL, "
        "version TEXT NOT NULL, "
        "created_at INTEGER NULL)");

    this->executeQuery(query);
}

void InstallerRepository::insertInstaller(const QString &installerName,
                                          const QString &installerPath,
                                          const QString &version,
                                          const QVariant &createdAt)
{
    QString query;

    if (createdAt.isNull()) {
        query = QString(
            "INSERT INTO installer_files(installer_name, installer_path, version, created_at) "
            "VALUES('%1', '%2', '%3', NULL)")
                    .arg(escapeSql(installerName))
                    .arg(escapeSql(installerPath))
                    .arg(escapeSql(version));
    } else {
        query = QString(
            "INSERT INTO installer_files(installer_name, installer_path, version, created_at) "
            "VALUES('%1', '%2', '%3', %4)")
                    .arg(escapeSql(installerName))
                    .arg(escapeSql(installerPath))
                    .arg(escapeSql(version))
                    .arg(createdAt.toLongLong());
    }

    this->executeQuery(query);
}

void InstallerRepository::updateInstaller(int id,
                                          const QString &installerName,
                                          const QString &installerPath,
                                          const QString &version,
                                          const QVariant &createdAt)
{
    QString query;

    if (createdAt.isNull()) {
        query = QString(
            "UPDATE installer_files "
            "SET installer_name = '%1', installer_path = '%2', version = '%3', created_at = NULL "
            "WHERE id = %4")
                    .arg(escapeSql(installerName))
                    .arg(escapeSql(installerPath))
                    .arg(escapeSql(version))
                    .arg(id);
    } else {
        query = QString(
            "UPDATE installer_files "
            "SET installer_name = '%1', installer_path = '%2', version = '%3', created_at = %4 "
            "WHERE id = %5")
                    .arg(escapeSql(installerName))
                    .arg(escapeSql(installerPath))
                    .arg(escapeSql(version))
                    .arg(createdAt.toLongLong())
                    .arg(id);
    }

    this->executeQuery(query);
}

void InstallerRepository::deleteInstaller(int id)
{
    const QString query = QString("DELETE FROM installer_files WHERE id = %1").arg(id);
    this->executeQuery(query);
}

QJsonArray InstallerRepository::getAllInstallers()
{
    const QString query = "SELECT * FROM installer_files ORDER BY id DESC";
    return this->executeQuery(query)->getRawMany();
}

QJsonObject InstallerRepository::getInstallerById(int id)
{
    const QString query = QString("SELECT * FROM installer_files WHERE id = %1 LIMIT 1").arg(id);
    return this->executeQuery(query)->getRawOne();
}
