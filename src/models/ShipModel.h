#ifndef SHIPMODEL_H
#define SHIPMODEL_H

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class ShipModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList ships READ ships NOTIFY shipsChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)

public:
    explicit ShipModel(QObject *parent = nullptr);

    QVariantList ships() const;
    bool loading() const;

    Q_INVOKABLE void reload();

signals:
    void shipsChanged();
    void loadingChanged();

private:
    QVariantList m_ships;
    bool m_loading = false;

    void setLoading(bool value);
};

#endif // SHIPMODEL_H
