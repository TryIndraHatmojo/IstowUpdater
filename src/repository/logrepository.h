#ifndef LOGREPOSITORY_H
#define LOGREPOSITORY_H

#include <QObject>
#include <QJsonArray>
#include "repository.h"
#include <QDateTime>

class LogRepository : public Repository
{
public:
    LogRepository(QString pathdb, QString conndb);
    static LogRepository *getInstance();

    void init();
    
    // insertLog uses the schema: Timestamp, Action, Target/Ship, Status, Status Message/Details, Log Path
    void insertLog(const QString &action, const QString &targetShip, const QString &status, const QString &statusMessage, const QString &logPath = "");

    // Helper to generate a .txt log file and return its path
    QString saveLogToFile(const QString &prefix, const QString &targetShip, const QStringList &logs);
    
    QJsonArray fetchLogs();

private:
    static LogRepository *logRepository;
};

#endif // LOGREPOSITORY_H