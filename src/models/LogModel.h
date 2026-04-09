#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <QAbstractTableModel>
#include <QJsonArray>
#include <QStringList>
#include <QJsonObject>
#include <QtQml/qqmlregistration.h>
#include "../repository/logrepository.h"

class LogModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit LogModel(QObject *parent = nullptr);

    enum LogRoles {
        TimestampRole = Qt::UserRole + 1,
        ActionRole,
        TargetShipRole,
        StatusRole,
        StatusMessageRole,
        LogPathRole
    };

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE void loadLogs();
    Q_INVOKABLE bool exportToCsv(const QString &filePath);
    Q_INVOKABLE bool exportStringListTxt(const QStringList &logs, const QString &filePath);
    Q_INVOKABLE void openLogPath(const QString &logPath);

private:
    QJsonArray m_logs;
};

#endif // LOGMODEL_H