#include "repository.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

// ──────────────────────────────────────────────
// QueryResult
// ──────────────────────────────────────────────

QueryResult::QueryResult(QSqlQuery query)
    : m_query(std::move(query))
{
}

QJsonArray QueryResult::getRawMany()
{
    QJsonArray result;
    while (m_query.next()) {
        QJsonObject obj;
        QSqlRecord record = m_query.record();
        for (int i = 0; i < record.count(); ++i) {
            obj.insert(record.fieldName(i), QJsonValue::fromVariant(record.value(i)));
        }
        result.append(obj);
    }
    qDebug() << "[QueryResult] getRawMany: total baris diterima:" << result.size();
    return result;
}

QJsonObject QueryResult::getRawOne()
{
    QJsonObject obj;
    if (m_query.next()) {
        QSqlRecord record = m_query.record();
        for (int i = 0; i < record.count(); ++i) {
            obj.insert(record.fieldName(i), QJsonValue::fromVariant(record.value(i)));
        }
    }
    return obj;
}

// ──────────────────────────────────────────────
// Repository
// ──────────────────────────────────────────────

Repository::Repository(QString pathdb, QString conndb)
    : m_connName(conndb)
{
    if (QSqlDatabase::contains(conndb)) {
        m_db = QSqlDatabase::database(conndb);
        qDebug() << "[Repository] Reuse koneksi:" << conndb;
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", conndb);
        m_db.setDatabaseName(pathdb);
        qDebug() << "[Repository] Buka DB:" << pathdb;
        if (!m_db.open()) {
            qWarning() << "[Repository] Gagal membuka database:" << m_db.lastError().text();
        } else {
            qDebug() << "[Repository] DB berhasil dibuka.";
        }
    }
}

Repository::~Repository()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(m_connName);
}

QueryResult *Repository::executeQuery(const QString &sql)
{
    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        qWarning() << "[Repository] Query GAGAL:" << query.lastError().text();
        qWarning() << "[Repository] SQL:" << sql;
    } else {
        qDebug() << "[Repository] Query OK:" << sql.left(80)
                 << (sql.length() > 80 ? "..." : "");
    }
    return new QueryResult(std::move(query));
}
