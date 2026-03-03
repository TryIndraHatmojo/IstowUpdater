#include "localrepository.h"
#include <QStandardPaths>
#include <QDir>

LocalRepository* LocalRepository::localRepository = nullptr;

LocalRepository::LocalRepository(QString pathdb, QString conndb)
    : Repository(pathdb, conndb)
{
}

LocalRepository *LocalRepository::getInstance()
{
    if (localRepository == nullptr) {
        QString fileName = "/578b896b3560265ace75015a4f77a672.sqlite";
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
        QString path = "QML/OfflineStorage/Databases";
        dir.mkpath(path);

        localRepository = new LocalRepository(dir.absolutePath() + "/" + path + fileName, "LOCALDB");
        localRepository->init();
    }
    return localRepository;
}

void LocalRepository::init()
{
    // Buat tabel shiptbl jika belum ada
    QString query = QString("CREATE TABLE IF NOT EXISTS shiptbl("
                            "id INTEGER PRIMARY KEY, "
                            "idship INT, "
                            "tipe TEXT, "
                            "nama TEXT, "
                            "company TEXT, "
                            "dbid TEXT, "
                            "fileprefix TEXT, "
                            "lastupdate INT, "
                            "version INT)");
    this->executeQuery(query);

    // Buat tabel voyagetbl jika belum ada
    query = QString("CREATE TABLE IF NOT EXISTS voyagetbl("
                    "id INTEGER PRIMARY KEY, "
                    "shiptbl_id INT, "
                    "novoyage TEXT, "
                    "lastupdate INT, "
                    "current_voyage_id INT DEFAULT -1)");
    this->executeQuery(query);

    // Cek dan tambahkan kolom 'version' di shiptbl jika belum ada
    query = QString("PRAGMA table_info(shiptbl);");
    QJsonArray shipTableInfo = this->executeQuery(query)->getRawMany();
    bool hasVersion = false;
    for (auto _obj : shipTableInfo) {
        QJsonObject obj = _obj.toObject();
        if (obj.value("name").toVariant().toString() == "version") {
            hasVersion = true;
            break;
        }
    }
    if (!hasVersion) {
        query = QString("ALTER TABLE shiptbl ADD COLUMN version INT;");
        this->executeQuery(query);
    }

    // Cek dan tambahkan kolom 'current_voyage_id' di voyagetbl jika belum ada
    query = QString("PRAGMA table_info(voyagetbl);");
    QJsonArray voyageTableInfo = this->executeQuery(query)->getRawMany();
    bool hasCurrentVoyageId = false;
    for (auto _obj : voyageTableInfo) {
        QJsonObject obj = _obj.toObject();
        if (obj.value("name").toVariant().toString() == "current_voyage_id") {
            hasCurrentVoyageId = true;
            break;
        }
    }
    if (!hasCurrentVoyageId) {
        query = QString("ALTER TABLE voyagetbl ADD COLUMN current_voyage_id INT DEFAULT -1;");
        this->executeQuery(query);
    }
}

/*!
 * \brief LocalRepository::getAllShip
 * \return QJsonArray berisi semua data kapal beserta voyage terakhirnya
 *
 * Mengambil semua data kapal dari shiptbl dan
 * melakukan LEFT JOIN dengan voyagetbl untuk mendapatkan
 * informasi voyage terakhir setiap kapal.
 */
QJsonArray LocalRepository::getAllShip()
{
    QString query = QString("SELECT "
                            "  shiptbl.idship, "
                            "  shiptbl.tipe, "
                            "  shiptbl.nama, "
                            "  shiptbl.company, "
                            "  shiptbl.dbid, "
                            "  shiptbl.fileprefix, "
                            "  shiptbl.version, "
                            "  voyagetbl.novoyage, "
                            "  voyagetbl.current_voyage_id "
                            "FROM shiptbl "
                            "LEFT JOIN voyagetbl ON shiptbl.idship = voyagetbl.shiptbl_id");
    return this->executeQuery(query)->getRawMany();
}