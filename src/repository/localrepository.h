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

private:
    static LocalRepository *localRepository;
};

#endif // LOCALREPOSITORY_H