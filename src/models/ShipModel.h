#ifndef SHIPMODEL_H
#define SHIPMODEL_H

#include <QObject>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include "Ship.h"

class ShipModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList ships READ ships NOTIFY shipsChanged)

public:
    explicit ShipModel(QObject *parent = nullptr);

    QVariantList ships() const;

    Q_INVOKABLE QVariantMap getShipAt(int index) const;
    Q_INVOKABLE int count() const;

signals:
    void shipsChanged();

private:
    QList<Ship>  m_shipEntities;
    QVariantList m_ships;

    void loadShips();
};

#endif // SHIPMODEL_H
