#ifndef INSTALLERMODEL_H
#define INSTALLERMODEL_H

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class InstallerModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList installers READ installers NOTIFY installersChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit InstallerModel(QObject *parent = nullptr);

    QVariantList installers() const;
    QString statusMessage() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool createInstaller(const QString &version,
                                     const QString &sourceFileUrl);
    Q_INVOKABLE bool updateInstaller(int id,
                                     const QString &version,
                                     const QString &sourceFileUrl);
    Q_INVOKABLE bool deleteInstaller(int id);
    Q_INVOKABLE bool installInstaller(int id);

signals:
    void installersChanged();
    void statusMessageChanged();

private:
    QVariantList m_installers;
    QString m_statusMessage;

    void loadInstallers();
    QString copyExeToInstallerFolder(const QString &sourceFileUrl, bool *ok);
    QString localPathFromUrl(const QString &sourceFileUrl) const;
    QString relativeInstallerPath(const QString &absolutePath) const;
    void setStatusMessage(const QString &message);
};

#endif // INSTALLERMODEL_H
