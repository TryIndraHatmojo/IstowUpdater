#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <QString>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>

class QueryResult
{
public:
    explicit QueryResult(QSqlQuery query);

    QJsonArray getRawMany();
    QJsonObject getRawOne();

private:
    QSqlQuery m_query;
};

class Repository
{
public:
    Repository(QString pathdb, QString conndb);
    virtual ~Repository();

protected:
    QueryResult *executeQuery(const QString &sql);

    QString sqlQuery;

private:
    QSqlDatabase m_db;
    QString m_connName;
};

#endif // REPOSITORY_H
