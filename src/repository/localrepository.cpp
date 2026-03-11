#include "localrepository.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

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

    // Seed data dummy untuk development (hanya jika tabel masih kosong)
    QueryResult *countResult = this->executeQuery("SELECT COUNT(*) as total FROM shiptbl");
    QJsonObject countRow = countResult->getRawOne();
    int total = countRow.value("total").toInt();
    qDebug() << "[LocalRepository] Jumlah data di shiptbl:" << total;

    if (total == 0) {
        qDebug() << "[LocalRepository] Tabel kosong, insert data dummy...";
        this->executeQuery("INSERT INTO shiptbl (idship, tipe, nama, company, dbid, fileprefix, version) VALUES "
                           "(1, 'Container', 'KM NUSANTARA 1', 'PT Pelni', 'NUS01', 'nus1', 1),"
                           "(2, 'Tanker',    'MT BAHARI JAYA', 'PT Pertamina', 'BAH02', 'bah2', 1),"
                           "(3, 'Bulk',      'MV SINAR ASIA',  'PT SPIL', 'SIN03', 'sin3', 1)");
        this->executeQuery("INSERT INTO voyagetbl (shiptbl_id, novoyage, current_voyage_id) VALUES "
                           "(1, 'V001/2026', 1),"
                           "(2, 'V002/2026', 2),"
                           "(3, 'V003/2026', 3)");
        qDebug() << "[LocalRepository] Data dummy berhasil di-insert.";
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

// ──────────────────────────────────────────────
// deleteShip — Hapus data kapal dari shiptbl dan voyagetbl
// ──────────────────────────────────────────────

void LocalRepository::deleteShip(int idship)
{
    qDebug() << "[LocalRepository] deleteShip: idship =" << idship;

    QString query = QString("DELETE FROM voyagetbl WHERE shiptbl_id = %1").arg(idship);
    this->executeQuery(query);

    query = QString("DELETE FROM shiptbl WHERE idship = %1").arg(idship);
    this->executeQuery(query);

    qDebug() << "[LocalRepository] deleteShip selesai untuk idship:" << idship;
}

// ──────────────────────────────────────────────
// insertShip — Insert data kapal baru ke shiptbl
// ──────────────────────────────────────────────

void LocalRepository::insertShip(int idship, const QString &tipe, const QString &nama,
                                  const QString &dbid, const QString &fileprefix,
                                  const QString &company, int version)
{
    qDebug() << "[LocalRepository] insertShip:" << nama << "| idship:" << idship;

    QString query = QString(
        "INSERT INTO shiptbl(idship, tipe, nama, company, dbid, fileprefix, lastupdate, version) "
        "VALUES(%1, '%2', '%3', '%4', '%5', '%6', strftime('%%s','now'), %7)")
        .arg(idship)
        .arg(tipe)
        .arg(nama)
        .arg(company)
        .arg(dbid)
        .arg(fileprefix)
        .arg(version);

    this->executeQuery(query);
    qDebug() << "[LocalRepository] insertShip selesai:" << nama;
}

// ──────────────────────────────────────────────
// checkShipExists — Cek apakah kapal sudah ada
// ──────────────────────────────────────────────

bool LocalRepository::checkShipExists(int idship)
{
    QString query = QString("SELECT COUNT(*) as count FROM shiptbl WHERE idship = %1").arg(idship);
    QueryResult *result = this->executeQuery(query);
    QJsonObject row = result->getRawOne();
    return row.value("count").toInt() > 0;
}

// ──────────────────────────────────────────────
// updateShip — Update data kapal tanpa menghapus voyage
// ──────────────────────────────────────────────

void LocalRepository::updateShip(int idship, const QString &tipe, const QString &nama,
                                  const QString &dbid, const QString &fileprefix,
                                  const QString &company, int version)
{
    qDebug() << "[LocalRepository] updateShip:" << nama << "| idship:" << idship;

    QString query = QString(
        "UPDATE shiptbl SET "
        "tipe = '%1', nama = '%2', company = '%3', dbid = '%4', fileprefix = '%5', lastupdate = strftime('%%s','now'), version = %6 "
        "WHERE idship = %7")
        .arg(tipe)
        .arg(nama)
        .arg(company)
        .arg(dbid)
        .arg(fileprefix)
        .arg(version)
        .arg(idship);

    this->executeQuery(query);
    qDebug() << "[LocalRepository] updateShip selesai:" << nama;
}
