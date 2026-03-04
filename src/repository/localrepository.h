#ifndef LOCALREPOSITORY_H
#define LOCALREPOSITORY_H

#include <QObject>
#include <QJsonArray>
#include "repository.h"

class LocalRepository : public Repository
{
public:
    LocalRepository(QString pathdb, QString conndb);
    static LocalRepository *getInstance();

    void init();
    QJsonArray getAllShip();

    void insertShip(int idship, const QString &tipe, const QString &nama,
                    const QString &dbid, const QString &fileprefix,
                    const QString &company, int version);
    void deleteShip(int idship);

private:
    static LocalRepository *localRepository;
};

#endif // LOCALREPOSITORY_H