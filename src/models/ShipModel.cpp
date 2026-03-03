#include "ShipModel.h"
#include "localrepository.h"
#include <QJsonArray>
#include <QJsonObject>

ShipModel::ShipModel(QObject *parent)
    : QObject(parent)
{
    reload();
}

QVariantList ShipModel::ships() const
{
    return m_ships;
}

bool ShipModel::loading() const
{
    return m_loading;
}

void ShipModel::setLoading(bool value)
{
    if (m_loading == value)
        return;
    m_loading = value;
    emit loadingChanged();
}

void ShipModel::reload()
{
    setLoading(true);

    QJsonArray data = LocalRepository::getInstance()->getAllShip();

    m_ships.clear();
    for (const auto &item : data) {
        m_ships.append(item.toObject().toVariantMap());
    }

    emit shipsChanged();
    setLoading(false);
}
