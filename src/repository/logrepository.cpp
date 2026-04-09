#include "logrepository.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QVariant>
#include <QSqlError>
#include <QSqlQuery>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>

LogRepository* LogRepository::logRepository = nullptr;

LogRepository::LogRepository(QString pathdb, QString conndb)
    : Repository(pathdb, conndb)
{
}

LogRepository *LogRepository::getInstance()
{
    if (logRepository == nullptr) {
        // Menggunakan database sqlite yang sama dengan BackupRepository
        QString wDir = QDir::homePath() + "/iStowV2/";
        QString dbDir = wDir + "IstowUpdater/";
        QDir().mkpath(dbDir);

        QString dbPath = dbDir + "backup_records.sqlite";
        logRepository = new LogRepository(dbPath, "LOGSDB");
        logRepository->init();
    }
    return logRepository;
}

void LogRepository::init()
{
    QString query = QString("CREATE TABLE IF NOT EXISTS activity_logs("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                            "timestamp TEXT, "
                            "action TEXT, "
                            "target_ship TEXT, "
                            "status TEXT, "
                            "status_message TEXT)");
    this->executeQuery(query);

    // Cek dan tambahkan kolom 'log_path' jika belum ada
    query = QString("PRAGMA table_info(activity_logs);");
    QJsonArray logTableInfo = this->executeQuery(query)->getRawMany();
    bool hasLogPath = false;
    for (auto _obj : logTableInfo) {
        QJsonObject obj = _obj.toObject();
        if (obj.value("name").toVariant().toString() == "log_path") {
            hasLogPath = true;
            break;
        }
    }
    if (!hasLogPath) {
        query = QString("ALTER TABLE activity_logs ADD COLUMN log_path TEXT;");
        this->executeQuery(query);
    }
}

void LogRepository::insertLog(const QString &action, const QString &targetShip, const QString &status, const QString &statusMessage, const QString &logPath)
{
    QString timestampStr = QDateTime::currentDateTime().toString(Qt::ISODate);
    QString query = QString("INSERT INTO activity_logs (timestamp, action, target_ship, status, status_message, log_path) VALUES (?, ?, ?, ?, ?, ?)");
    QSqlQuery q = QSqlDatabase::database("LOGSDB").exec();
    q.prepare(query);
    q.addBindValue(timestampStr);
    q.addBindValue(action);
    q.addBindValue(targetShip);
    q.addBindValue(status);
    q.addBindValue(statusMessage);
    q.addBindValue(logPath);
    if(!q.exec()) {
        qDebug() << "Failed to insert log" << q.lastError().text();
    }
}

QString LogRepository::saveLogToFile(const QString &prefix, const QString &targetShip, const QStringList &logs)
{
    if (logs.isEmpty()) return "";

    QString logDir = QDir::homePath() + "/iStowV2/IstowUpdater/logs";
    QDir().mkpath(logDir);

    QString safeTarget = targetShip;
    safeTarget.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_"); // Remove invalid filename chars
    QString logFileName = prefix + "_" + safeTarget + "_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt";
    QString logPath = logDir + "/" + logFileName;

    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &line : logs) {
            out << line << "\n";
        }
        file.close();
        return logPath;
    }
    
    qWarning() << "Could not write to auto-generated log file:" << logPath;
    return "";
}

QJsonArray LogRepository::fetchLogs()
{
    QString query = QString("SELECT timestamp, action, target_ship, status, status_message, log_path FROM activity_logs ORDER BY id DESC");
    QueryResult* res = this->executeQuery(query);
    return res->getRawMany();
}