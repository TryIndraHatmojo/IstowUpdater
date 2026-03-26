#ifndef INSTALLERREPOSITORY_H
#define INSTALLERREPOSITORY_H

#include <QJsonArray>
#include <QJsonObject>
#include "repository.h"

class InstallerRepository : public Repository
{
public:
    InstallerRepository(QString pathdb, QString conndb);
    static InstallerRepository *getInstance();
    static QString projectRootPath();
    static QString installerFolderPath();

    void init();
    void insertInstaller(const QString &installerName,
                         const QString &installerPath,
                         const QString &version,
                         const QVariant &createdAt = QVariant());
    void updateInstaller(int id,
                         const QString &installerName,
                         const QString &installerPath,
                         const QString &version,
                         const QVariant &createdAt = QVariant());
    void deleteInstaller(int id);
    QJsonArray getAllInstallers();
    QJsonObject getInstallerById(int id);

private:
    static InstallerRepository *installerRepository;
    static QString escapeSql(const QString &value);
};

#endif // INSTALLERREPOSITORY_H
