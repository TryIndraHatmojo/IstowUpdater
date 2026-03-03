#include "ShipModel.h"
#include "localrepository.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

ShipModel::ShipModel(QObject *parent)
    : QObject(parent)
{
    loadShips();
}

QVariantList ShipModel::ships() const
{
    return m_ships;
}

QVariantMap ShipModel::getShipAt(int index) const
{
    if (index >= 0 && index < m_shipEntities.size()) {
        qDebug() << "[ShipModel] getShipAt(" << index << "):"
                 << m_shipEntities.at(index).nama;
        return m_shipEntities.at(index).toVariantMap();
    }
    qWarning() << "[ShipModel] getShipAt: index" << index << "di luar range (size:" << m_shipEntities.size() << ")";
    return {};
}

int ShipModel::count() const
{
    return m_shipEntities.size();
}

void ShipModel::loadShips()
{
    qDebug() << "[ShipModel] loadShips: mulai mengambil data kapal...";

    QJsonArray data = LocalRepository::getInstance()->getAllShip();

    qDebug() << "[ShipModel] data diterima dari repository:" << data.size() << "item";

    m_shipEntities.clear();
    m_ships.clear();

    for (const auto &item : data) {
        Ship ship = Ship::fromJsonObject(item.toObject());
        m_shipEntities.append(ship);
        m_ships.append(ship.toVariantMap());
        qDebug() << "[ShipModel] loaded:" << ship.nama << "| idship:" << ship.idship;
    }

    qDebug() << "[ShipModel] loadShips: total" << m_shipEntities.size() << "kapal dimuat.";
    emit shipsChanged();
}
