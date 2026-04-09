#include "LogModel.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QUrl>
#include <QDesktopServices>

LogModel::LogModel(QObject *parent)
    : QAbstractTableModel(parent)
{
    loadLogs();
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[TimestampRole] = "timestamp";
    roles[ActionRole] = "action";
    roles[TargetShipRole] = "target_ship";
    roles[StatusRole] = "status";
    roles[StatusMessageRole] = "status_message";
    roles[LogPathRole] = "log_path";
    return roles;
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_logs.size();
}

int LogModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_logs.size())
        return QVariant();

    QJsonObject obj = m_logs[index.row()].toObject();

    switch (role) {
    case TimestampRole:
        return obj.value("timestamp").toString();
    case ActionRole:
        return obj.value("action").toString();
    case TargetShipRole:
        return obj.value("target_ship").toString();
    case StatusRole:
        return obj.value("status").toString();
    case StatusMessageRole:
        return obj.value("status_message").toString();
    case LogPathRole:
        return obj.value("log_path").toString();
    default:
        return QVariant();
    }
}

void LogModel::loadLogs()
{
    beginResetModel();
    m_logs = LogRepository::getInstance()->fetchLogs();
    endResetModel();
}

bool LogModel::exportToCsv(const QString &filePath)
{
    QString path = filePath;
    // Handle file URL
    if (path.startsWith("file:///")) {
        path = QUrl(path).toLocalFile();
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << path;
        return false;
    }

    QTextStream out(&file);
    
    // Write headers
    out << "Timestamp,Action,Target/Ship,Status,Status Message/Details\n";

    // Write data
    for (int i = 0; i < m_logs.size(); ++i) {
        QJsonObject obj = m_logs[i].toObject();
        QString timestamp = obj.value("timestamp").toString();
        QString action = obj.value("action").toString();
        QString targetShip = obj.value("target_ship").toString();
        QString status = obj.value("status").toString();
        QString statusMsg = obj.value("status_message").toString();

        // Escape CSV commas
        if (action.contains(",")) action = "\"" + action + "\"";
        if (targetShip.contains(",")) targetShip = "\"" + targetShip + "\"";
        if (status.contains(",")) status = "\"" + status + "\"";
        if (statusMsg.contains(",")) statusMsg = "\"" + statusMsg + "\"";

        out << timestamp << ","
            << action << ","
            << targetShip << ","
            << status << ","
            << statusMsg << "\n";
    }

    file.close();
    qDebug() << "Successfully exported logs to CSV format at:" << path;
    return true;
}

bool LogModel::exportStringListTxt(const QStringList &logs, const QString &filePath)
{
    QString path = filePath;
    // Handle file URL
    if (path.startsWith("file:///")) {
        path = QUrl(path).toLocalFile();
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing text logs:" << path;
        return false;
    }

    QTextStream out(&file);
    for (const QString &line : logs) {
        out << line << "\n";
    }

    file.close();
    qDebug() << "Successfully exported text logs to:" << path;
    return true;
}

void LogModel::openLogPath(const QString &logPath)
{
    if (logPath.isEmpty()) {
        qWarning() << "Log path is empty, cannot open";
        return;
    }
    
    QUrl url;
    if (logPath.startsWith("http://") || logPath.startsWith("https://") || logPath.startsWith("file://")) {
        url = QUrl(logPath);
    } else {
        url = QUrl::fromLocalFile(logPath);
    }
    
    QDesktopServices::openUrl(url);
}