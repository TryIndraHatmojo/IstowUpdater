#include "backuprepository.h"
#include <QDir>
#include <QDebug>
#include <QDateTime>

BackupRepository* BackupRepository::backupRepository = nullptr;

BackupRepository::BackupRepository(QString pathdb, QString conndb)
    : Repository(pathdb, conndb)
{
}

BackupRepository *BackupRepository::getInstance()
{
    if (backupRepository == nullptr) {
        QString wDir = QDir::homePath() + "/iStowV2/";
        QString dbDir = wDir + "IstowUpdater/";
        QDir().mkpath(dbDir);

        QString dbPath = dbDir + "backup_records.sqlite";
        backupRepository = new BackupRepository(dbPath, "BACKUPDB");
        backupRepository->init();
    }
    return backupRepository;
}

void BackupRepository::init()
{
    // Tabel sessions: menyimpan folder-folder backup
    QString query = QString(
        "CREATE TABLE IF NOT EXISTS backup_sessions("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "folder_name TEXT UNIQUE, "
        "ship_name TEXT, "
        "idship INT, "
        "status TEXT DEFAULT 'completed', "
        "created_at INTEGER DEFAULT (CAST(strftime('%%s','now','localtime') AS INTEGER)))" );
    this->executeQuery(query);

    // Tabel records: menyimpan detail file per session
    query = QString(
        "CREATE TABLE IF NOT EXISTS backup_records("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "session_id INT, "
        "file_path TEXT, "
        "backup_path TEXT, "
        "action TEXT, "
        "created_at INTEGER DEFAULT (CAST(strftime('%%s','now','localtime') AS INTEGER)), "
        "FOREIGN KEY(session_id) REFERENCES backup_sessions(id))");
    this->executeQuery(query);

    // Tabel application backup sessions
    query = QString(
        "CREATE TABLE IF NOT EXISTS application_backup_sessions("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "folder_name TEXT UNIQUE, "
        "app_name TEXT, "
        "version TEXT, "
        "status TEXT DEFAULT 'completed', "
        "created_at INTEGER DEFAULT (CAST(strftime('%%s','now','localtime') AS INTEGER)))" );
    this->executeQuery(query);

    // Tabel application backup records
    query = QString(
        "CREATE TABLE IF NOT EXISTS application_backup_records("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "session_id INT, "
        "file_path TEXT, "
        "backup_path TEXT, "
        "action TEXT, "
        "created_at INTEGER DEFAULT (CAST(strftime('%%s','now','localtime') AS INTEGER)), "
        "FOREIGN KEY(session_id) REFERENCES application_backup_sessions(id))");
    this->executeQuery(query);
}

// ──────────────────────────────────────────────
// Backup Sessions
// ──────────────────────────────────────────────

int BackupRepository::insertSession(const QString &folderName, const QString &shipName, int idship)
{
    qDebug() << "[BackupRepository] insertSession:" << folderName << "ship:" << shipName;

    qint64 currentTimestamp = QDateTime::currentSecsSinceEpoch();

    QString query = QString(
        "INSERT INTO backup_sessions(folder_name, ship_name, idship, created_at) "
        "VALUES('%1', '%2', %3, %4)")
        .arg(folderName)
        .arg(shipName)
        .arg(idship)
        .arg(currentTimestamp);
    this->executeQuery(query);

    // Ambil ID session yang baru di-insert
    QueryResult *result = this->executeQuery("SELECT last_insert_rowid() as id");
    QJsonObject row = result->getRawOne();
    int sessionId = row.value("id").toInt();
    qDebug() << "[BackupRepository] Session ID:" << sessionId;
    return sessionId;
}

void BackupRepository::updateSessionStatus(int sessionId, const QString &status)
{
    QString query = QString("UPDATE backup_sessions SET status = '%1' WHERE id = %2")
        .arg(status)
        .arg(sessionId);
    this->executeQuery(query);
}

QJsonArray BackupRepository::getAllSessions()
{
    QString query = "SELECT * FROM backup_sessions ORDER BY created_at DESC";
    return this->executeQuery(query)->getRawMany();
}

QJsonObject BackupRepository::getSessionById(int sessionId)
{
    QString query = QString(
        "SELECT * FROM backup_sessions WHERE id = %1")
        .arg(sessionId);
    return this->executeQuery(query)->getRawOne();
}

QJsonObject BackupRepository::getSessionByFolder(const QString &folderName)
{
    QString query = QString(
        "SELECT * FROM backup_sessions WHERE folder_name = '%1'")
        .arg(folderName);
    return this->executeQuery(query)->getRawOne();
}

QJsonArray BackupRepository::getSessionsByShip(int idship)
{
    QString query = QString(
        "SELECT * FROM backup_sessions WHERE idship = %1 ORDER BY created_at DESC")
        .arg(idship);
    return this->executeQuery(query)->getRawMany();
}

// ──────────────────────────────────────────────
// Backup Records
// ──────────────────────────────────────────────

void BackupRepository::insertRecord(int sessionId, const QString &filePath, const QString &backupPath,
                                     const QString &action)
{
    qDebug() << "[BackupRepository] insertRecord:" << action << filePath;

    qint64 currentTimestamp = QDateTime::currentSecsSinceEpoch();

    QString query = QString(
        "INSERT INTO backup_records(session_id, file_path, backup_path, action, created_at) "
        "VALUES(%1, '%2', '%3', '%4', %5)")
        .arg(sessionId)
        .arg(filePath)
        .arg(backupPath)
        .arg(action)
        .arg(currentTimestamp);

    this->executeQuery(query);
}

QJsonArray BackupRepository::getAllRecords()
{
    QString query = "SELECT r.*, s.folder_name FROM backup_records r "
                    "LEFT JOIN backup_sessions s ON r.session_id = s.id "
                    "ORDER BY r.created_at DESC";
    return this->executeQuery(query)->getRawMany();
}

QJsonArray BackupRepository::getRecordsBySession(int sessionId)
{
    QString query = QString(
        "SELECT * FROM backup_records WHERE session_id = %1 ORDER BY created_at DESC")
        .arg(sessionId);
    return this->executeQuery(query)->getRawMany();
}

// ──────────────────────────────────────────────
// Application Backup Sessions
// ──────────────────────────────────────────────

int BackupRepository::insertAppSession(const QString &folderName, const QString &appName, const QString &version)
{
    qDebug() << "[BackupRepository] insertAppSession:" << folderName << "app:" << appName;

    qint64 currentTimestamp = QDateTime::currentSecsSinceEpoch();

    QString query = QString(
        "INSERT INTO application_backup_sessions(folder_name, app_name, version, created_at) "
        "VALUES('%1', '%2', '%3', %4)")
        .arg(folderName)
        .arg(appName)
        .arg(version)
        .arg(currentTimestamp);
    this->executeQuery(query);

    QueryResult *result = this->executeQuery("SELECT last_insert_rowid() as id");
    QJsonObject row = result->getRawOne();
    int sessionId = row.value("id").toInt();
    qDebug() << "[BackupRepository] App Session ID:" << sessionId;
    return sessionId;
}

void BackupRepository::updateAppSessionStatus(int sessionId, const QString &status)
{
    QString query = QString("UPDATE application_backup_sessions SET status = '%1' WHERE id = %2")
        .arg(status)
        .arg(sessionId);
    this->executeQuery(query);
}

QJsonArray BackupRepository::getAllAppSessions()
{
    QString query = "SELECT * FROM application_backup_sessions ORDER BY created_at DESC";
    return this->executeQuery(query)->getRawMany();
}

QJsonObject BackupRepository::getAppSessionById(int sessionId)
{
    QString query = QString(
        "SELECT * FROM application_backup_sessions WHERE id = %1")
        .arg(sessionId);
    return this->executeQuery(query)->getRawOne();
}

// ──────────────────────────────────────────────
// Application Backup Records
// ──────────────────────────────────────────────

void BackupRepository::insertAppRecord(int sessionId, const QString &filePath, const QString &backupPath,
                                       const QString &action)
{
    qDebug() << "[BackupRepository] insertAppRecord:" << action << filePath;

    qint64 currentTimestamp = QDateTime::currentSecsSinceEpoch();

    QString query = QString(
        "INSERT INTO application_backup_records(session_id, file_path, backup_path, action, created_at) "
        "VALUES(%1, '%2', '%3', '%4', %5)")
        .arg(sessionId)
        .arg(filePath)
        .arg(backupPath)
        .arg(action)
        .arg(currentTimestamp);

    this->executeQuery(query);
}

QJsonArray BackupRepository::getAllAppRecords()
{
    QString query = "SELECT r.*, s.folder_name FROM application_backup_records r "
                    "LEFT JOIN application_backup_sessions s ON r.session_id = s.id "
                    "ORDER BY r.created_at DESC";
    return this->executeQuery(query)->getRawMany();
}

QJsonArray BackupRepository::getAppRecordsBySession(int sessionId)
{
    QString query = QString(
        "SELECT * FROM application_backup_records WHERE session_id = %1 ORDER BY created_at DESC")
        .arg(sessionId);
    return this->executeQuery(query)->getRawMany();
}
