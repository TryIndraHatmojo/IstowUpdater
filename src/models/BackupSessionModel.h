#ifndef BACKUPSESSIONMODEL_H
#define BACKUPSESSIONMODEL_H

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class BackupSessionModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList sessions READ sessions NOTIFY sessionsChanged)
    Q_PROPERTY(QVariantList appSessions READ appSessions NOTIFY appSessionsChanged)
    Q_PROPERTY(bool rollingBack READ rollingBack NOTIFY rollingBackChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QStringList rollbackLogs READ rollbackLogs NOTIFY rollbackLogsChanged)

public:
    explicit BackupSessionModel(QObject *parent = nullptr);

    QVariantList sessions() const;
    QVariantList appSessions() const;
    bool rollingBack() const;
    QString statusMessage() const;
    QStringList rollbackLogs() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE bool rollbackSession(int sessionId);
    Q_INVOKABLE bool deleteSession(int sessionId);
    Q_INVOKABLE bool rollbackAppSession(int sessionId, const QString &targetDirUrl);
    Q_INVOKABLE bool backupOldIstow(const QString &directoryUrl);
    Q_INVOKABLE bool uninstallOldIstow(const QString &directoryUrl);

signals:
    void sessionsChanged();
    void appSessionsChanged();
    void rollingBackChanged();
    void statusMessageChanged();
    void rollbackLogsChanged();

private:
    QVariantList m_sessions;
    QVariantList m_appSessions;
    bool m_rollingBack = false;
    QString m_statusMessage;
    QStringList m_rollbackLogs;

    void loadSessions();
    void appendLog(const QString &message);
    void setRollingBack(bool value);
    void setStatusMessage(const QString &message);
};

#endif // BACKUPSESSIONMODEL_H