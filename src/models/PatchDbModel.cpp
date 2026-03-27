#include "PatchDbModel.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QUrl>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSet>
#include <QGuiApplication>
#include <QClipboard>

// ════════════════════════════════════════════════════════════
// Constructor / Destructor
// ════════════════════════════════════════════════════════════

PatchDbModel::PatchDbModel(QObject *parent)
    : QObject(parent)
    , m_connNew("patch_new_" + QString::number(quintptr(this)))
    , m_connOld("patch_old_" + QString::number(quintptr(this)))
{
}

PatchDbModel::~PatchDbModel()
{
    cleanup();
}

// ════════════════════════════════════════════════════════════
// Properties
// ════════════════════════════════════════════════════════════

bool PatchDbModel::comparing() const { return m_comparing; }
bool PatchDbModel::loaded() const { return m_loaded; }
QString PatchDbModel::statusMessage() const { return m_statusMessage; }
QStringList PatchDbModel::diffTableList() const { return m_diffTableList; }
QStringList PatchDbModel::allTableList() const { return m_allTableList; }
QVariantMap PatchDbModel::shipDetails() const { return m_shipDetails; }
int PatchDbModel::dataVersion() const { return m_dataVersion; }
QString PatchDbModel::newDbFileName() const { return m_newDbFileName; }

void PatchDbModel::setComparing(bool v) {
    if (m_comparing != v) { m_comparing = v; emit comparingChanged(); }
}
void PatchDbModel::setStatusMessage(const QString &msg) {
    m_statusMessage = msg; emit statusMessageChanged();
    qDebug() << "[PatchDbModel]" << msg;
}
void PatchDbModel::setLoaded(bool v) {
    if (m_loaded != v) { m_loaded = v; emit loadedChanged(); }
}
void PatchDbModel::bumpDataVersion() {
    m_dataVersion++;
    emit dataVersionChanged();
}

// ════════════════════════════════════════════════════════════
// Helpers
// ════════════════════════════════════════════════════════════

QString PatchDbModel::toLocalPath(const QString &urlPath) const {
    QUrl url(urlPath);
    return url.isLocalFile() ? url.toLocalFile() : urlPath;
}

QString PatchDbModel::workDir() const {
    return QDir::homePath() + "/iStowV2/";
}

void PatchDbModel::closeConnections() {
    {
        if (QSqlDatabase::contains(m_connNew))
            QSqlDatabase::database(m_connNew).close();
        if (QSqlDatabase::contains(m_connOld))
            QSqlDatabase::database(m_connOld).close();
    }
    QSqlDatabase::removeDatabase(m_connNew);
    QSqlDatabase::removeDatabase(m_connOld);
}

void PatchDbModel::cleanup() {
    closeConnections();
    setLoaded(false);
    m_diffTableList.clear();
    m_allTableList.clear();
    m_shipDetails.clear();
    m_newTableSizes.clear();
    m_oldTableSizes.clear();
    emit diffTableListChanged();

    emit allTableListChanged();
    emit shipDetailsChanged();
}

void PatchDbModel::copyToClipboard(const QString &text) {
    QGuiApplication::clipboard()->setText(text);
}

// ════════════════════════════════════════════════════════════
// .istow helpers (mirror dari IstowImporter)
// ════════════════════════════════════════════════════════════

bool PatchDbModel::readDetailsFromIstow(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QDataStream ds(&file);
    while (!ds.atEnd()) {
        QString fileName; QByteArray data;
        ds >> fileName >> data;
        if (ds.status() != QDataStream::Ok) break;

        if (fileName.endsWith(".details")) {
            QByteArray uncompressed = qUncompress(data);
            QJsonDocument doc = QJsonDocument::fromJson(uncompressed);
            if (doc.isNull() || !doc.isObject()) { file.close(); return false; }
            m_shipDetails = doc.object().toVariantMap();
            emit shipDetailsChanged();
            file.close();
            return true;
        }
    }
    file.close();
    return false;
}

QString PatchDbModel::extractDbFromIstow(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return "";

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                      + "/IstowUpdater_patch_temp";
    QDir().mkpath(tempDir);

    QString targetDbId = m_shipDetails.value("dbid", "").toString();

    QDataStream ds(&file);
    while (!ds.atEnd()) {
        QString fileName; QByteArray data;
        ds >> fileName >> data;
        if (ds.status() != QDataStream::Ok) break;
        qDebug() << "[PatchDbModel] Memeriksa file dalam .istow:" << fileName;

        if (fileName.endsWith(".db")) {
            // Jika ada '/' di depan, abaikan untuk sementara saat pengecekan folder
            QString cleanName = fileName.startsWith("/") ? fileName.mid(1) : fileName;

            // Hanya proses file yang ada di root directory .istow (tidak mengandung '/' lagi setelah dibersihkan)
            if (cleanName.contains('/')) {
                continue;
            }

            QString dbBaseName = QFileInfo(fileName).fileName();
            
            // Verifikasi bahwa file yang diekstrak secara persis sama dengan id db target ".db"
            QString expectedName = targetDbId.endsWith(".db") ? targetDbId : (targetDbId + ".db");
            if (!targetDbId.isEmpty() && dbBaseName != expectedName) {
                continue;
            }
            qDebug() << "[PatchDbModel] Menemukan file DB dalam .istow:" << dbBaseName;
            qDebug() << "[PatchDbModel] Target DB ID dari metadata:" << targetDbId;
            qDebug() << "[PatchDbModel] Verifikasi nama file DB:" << filePath << "->" << dbBaseName << " (expected:" << expectedName << ")";

            QString tempDbPath = tempDir + "/" + dbBaseName;
            if (QFile::exists(tempDbPath)) QFile::remove(tempDbPath);

            QFile outFile(tempDbPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(qUncompress(data));
                outFile.close();
                file.close();
                
                m_newDbFileName = dbBaseName;
                emit newDbFileNameChanged();
                
                return tempDbPath;
            }
        }
    }
    file.close();
    return "";
}

// ════════════════════════════════════════════════════════════
// Primary Key Detection
// ════════════════════════════════════════════════════════════

QString PatchDbModel::detectPrimaryKey(const QString &tableName, const QString &connName) {
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery q(db);
    q.exec(QString("PRAGMA table_info(\"%1\")").arg(tableName));

    QStringList pkCols;
    while (q.next()) {
        if (q.value(5).toInt() > 0) {
            pkCols.append(q.value(1).toString());
        }
    }

    if (pkCols.isEmpty()) return "rowid";
    return pkCols.first(); // Gunakan PK pertama
}

// ════════════════════════════════════════════════════════════
// loadAndCompare — Main entry point
// ════════════════════════════════════════════════════════════

bool PatchDbModel::loadAndCompare(const QString &istowFilePath) {
    QString localPath = toLocalPath(istowFilePath);
    setComparing(true);
    setStatusMessage("Memulai proses Patch DB Data...");

    // 1. Read .details
    if (!readDetailsFromIstow(localPath)) {
        setStatusMessage("❌ Gagal membaca metadata dari .istow");
        setComparing(false);
        return false;
    }

    QString dbid = m_shipDetails.value("dbid", "").toString();
    if (dbid.isEmpty()) {
        setStatusMessage("❌ dbid tidak ditemukan di metadata");
        setComparing(false);
        return false;
    }

    // 2. Extract DB ke temp
    m_newDbPath = extractDbFromIstow(localPath);
    if (m_newDbPath.isEmpty()) {
        setStatusMessage("❌ Tidak ada file .db dalam .istow");
        setComparing(false);
        return false;
    }

    // 3. Target DB path
    m_oldDbPath = workDir() + dbid;
    if (!QFile::exists(m_oldDbPath)) {
        setStatusMessage("❌ Database target tidak ditemukan: " + m_oldDbPath);
        setComparing(false);
        return false;
    }

    // 4. Close previous connections
    closeConnections();

    // 5. Open both databases
    {
        QSqlDatabase dbNew = QSqlDatabase::addDatabase("QSQLITE", m_connNew);
        dbNew.setDatabaseName(m_newDbPath);
        if (!dbNew.open()) {
            setStatusMessage("❌ Gagal buka DB baru: " + dbNew.lastError().text());
            setComparing(false);
            return false;
        }

        QSqlDatabase dbOld = QSqlDatabase::addDatabase("QSQLITE", m_connOld);
        dbOld.setDatabaseName(m_oldDbPath);
        if (!dbOld.open()) {
            setStatusMessage("❌ Gagal buka DB target: " + dbOld.lastError().text());
            setComparing(false);
            return false;
        }
    }

    // 6. Get table lists
    QSet<QString> newTables, oldTables;
    {
        QSqlQuery q(QSqlDatabase::database(m_connNew));
        q.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
        while (q.next()) newTables.insert(q.value(0).toString());
    }
    {
        QSqlQuery q(QSqlDatabase::database(m_connOld));
        q.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
        while (q.next()) oldTables.insert(q.value(0).toString());
    }

    QSet<QString> allTables = newTables | oldTables;
    QSet<QString> commonTables = newTables & oldTables;

    QStringList initialTableList = allTables.values();
    initialTableList.sort();

    // 7. Compare data untuk tabel-tabel yang ada di kedua DB
    QStringList diffTables;
    QStringList finalAllTables;

    for (const QString &table : std::as_const(initialTableList)) {
        QSqlDatabase dbNew = QSqlDatabase::database(m_connNew);
        QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);

        // Cek jumlah row di masing-masing DB
        int newCount = 0;
        if (newTables.contains(table)) {
            QSqlQuery q(dbNew);
            q.exec(QString("SELECT count(*) FROM \"%1\"").arg(table));
            if (q.next()) newCount = q.value(0).toInt();
        }

        int oldCount = 0;
        if (oldTables.contains(table)) {
            QSqlQuery q(dbOld);
            q.exec(QString("SELECT count(*) FROM \"%1\"").arg(table));
            if (q.next()) oldCount = q.value(0).toInt();
        }

        m_newTableSizes[table] = newCount;
        m_oldTableSizes[table] = oldCount;

        // Jika row di kedua DB sama-sama kosong (atau tidak ada), lewati dan jangan list tabel ini
        if (newCount == 0 && oldCount == 0) {
            continue;
        }

        finalAllTables.append(table);

        // Tabel hanya ada di satu DB menanggapinya sebagai tabel yang berbeda (karena di sisi lain tidak ada dan row count > 0)
        if (!newTables.contains(table) || !oldTables.contains(table)) {
            diffTables.append(table);
            continue;
        }

        QString pk = detectPrimaryKey(table, m_connNew);

        // Ambil semua data dari kedua DB dan bandingkan

        // Ambil rows dari new DB
        QMap<QString, QVariantMap> newRows;
        {
            QSqlQuery q(dbNew);
            if (pk == "rowid")
                q.exec(QString("SELECT rowid AS _pk_rowid, * FROM \"%1\"").arg(table));
            else
                q.exec(QString("SELECT * FROM \"%1\"").arg(table));

            QSqlRecord rec = q.record();
            while (q.next()) {
                QVariantMap row;
                QString pkVal;
                for (int i = 0; i < rec.count(); i++) {
                    QString col = rec.fieldName(i);
                    row[col] = q.value(i);
                    if (col == pk || (pk == "rowid" && col == "_pk_rowid"))
                        pkVal = q.value(i).toString();
                }
                newRows[pkVal] = row;
            }
        }

        // Ambil rows dari old DB
        QMap<QString, QVariantMap> oldRows;
        {
            QSqlQuery q(dbOld);
            if (pk == "rowid")
                q.exec(QString("SELECT rowid AS _pk_rowid, * FROM \"%1\"").arg(table));
            else
                q.exec(QString("SELECT * FROM \"%1\"").arg(table));

            QSqlRecord rec = q.record();
            while (q.next()) {
                QVariantMap row;
                QString pkVal;
                for (int i = 0; i < rec.count(); i++) {
                    QString col = rec.fieldName(i);
                    row[col] = q.value(i);
                    if (col == pk || (pk == "rowid" && col == "_pk_rowid"))
                        pkVal = q.value(i).toString();
                }
                oldRows[pkVal] = row;
            }
        }

        // Bandingkan
        bool hasDiff = false;

        // Cek row baru yang tidak ada di old
        for (auto it = newRows.constBegin(); it != newRows.constEnd(); ++it) {
            if (!oldRows.contains(it.key())) { hasDiff = true; break; }
            // Cek value per kolom
            const QVariantMap &nr = it.value();
            const QVariantMap &or_ = oldRows[it.key()];
            for (auto ci = nr.constBegin(); ci != nr.constEnd(); ++ci) {
                if (ci.key() == "_pk_rowid") continue;
                if (ci.key() == "created_at" || ci.key() == "updated_at") continue;
                if (ci.value().toString() != or_.value(ci.key()).toString()) {
                    hasDiff = true; break;
                }
            }
            if (hasDiff) break;
        }

        if (!hasDiff) {
            // Cek row lama yang tidak ada di new
            for (auto it = oldRows.constBegin(); it != oldRows.constEnd(); ++it) {
                if (!newRows.contains(it.key())) { hasDiff = true; break; }
            }
        }

        if (hasDiff) diffTables.append(table);
    }

    diffTables.sort();
    m_diffTableList = diffTables;
    m_allTableList = finalAllTables;

    emit allTableListChanged();
    emit diffTableListChanged();

    setStatusMessage(QString("✅ Compare selesai: %1 tabel total, %2 tabel memiliki perbedaan data")
                         .arg(m_allTableList.size()).arg(m_diffTableList.size()));
    setComparing(false);
    setLoaded(true);
    return true;
}

// ════════════════════════════════════════════════════════════
// getColumns — Ambil nama kolom dari tabel
// ════════════════════════════════════════════════════════════

QVariantList PatchDbModel::getColumns(const QString &tableName) {
    QVariantList result;
    // Prioritas: ambil dari DB baru, fallback ke DB lama
    QString conn = QSqlDatabase::contains(m_connNew) ? m_connNew : m_connOld;
    if (!QSqlDatabase::contains(conn)) return result;

    QSqlDatabase db = QSqlDatabase::database(conn);
    QSqlQuery q(db);
    q.exec(QString("PRAGMA table_info(\"%1\")").arg(tableName));

    while (q.next()) {
        QVariantMap col;
        col["name"] = q.value(1).toString();
        col["type"] = q.value(2).toString();
        col["pk"] = q.value(5).toInt() > 0;
        result.append(col);
    }
    return result;
}

QString PatchDbModel::getPrimaryKey(const QString &tableName) {
    QString conn = QSqlDatabase::contains(m_connNew) ? m_connNew : m_connOld;
    return detectPrimaryKey(tableName, conn);
}

// ════════════════════════════════════════════════════════════
// getNewDbRows / getOldDbRows — dengan anotasi diff
// ════════════════════════════════════════════════════════════

static QVariantList fetchAllRows(const QString &tableName, const QString &pk,
                                  const QString &connName) {
    QVariantList result;
    if (!QSqlDatabase::contains(connName)) return result;

    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery q(db);

    if (pk == "rowid")
        q.exec(QString("SELECT rowid AS _pk_rowid, * FROM \"%1\"").arg(tableName));
    else
        q.exec(QString("SELECT * FROM \"%1\"").arg(tableName));

    QSqlRecord rec = q.record();
    while (q.next()) {
        QVariantMap row;
        for (int i = 0; i < rec.count(); i++) {
            row[rec.fieldName(i)] = q.value(i);
        }
        // Set _pk
        if (pk == "rowid")
            row["_pk"] = row.value("_pk_rowid");
        else
            row["_pk"] = row.value(pk);
        result.append(row);
    }
    return result;
}

static QMap<QString, QVariantMap> indexByPk(const QVariantList &rows) {
    QMap<QString, QVariantMap> map;
    for (const QVariant &v : rows) {
        QVariantMap row = v.toMap();
        map[row.value("_pk").toString()] = row;
    }
    return map;
}

QVariantList PatchDbModel::getNewDbRows(const QString &tableName) {
    QString pk = getPrimaryKey(tableName);
    QVariantList newRows = fetchAllRows(tableName, pk, m_connNew);
    QVariantList oldRows = fetchAllRows(tableName, pk, m_connOld);

    auto oldMap = indexByPk(oldRows);
    QVariantList filteredRows;

    // Annotate each new row with diff info
    for (int i = 0; i < newRows.size(); i++) {
        QVariantMap row = newRows[i].toMap();
        QString pkVal = row.value("_pk").toString();

        if (!oldMap.contains(pkVal)) {
            row["_status"] = "new_only";
            row["_diffCols"] = "";
            filteredRows.append(row);
        } else {
            QVariantMap oldRow = oldMap[pkVal];
            QStringList diffCols;
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                if (it.key().startsWith("_")) continue;
                if (it.key() == "created_at" || it.key() == "updated_at") continue;
                if (it.value().toString() != oldRow.value(it.key()).toString())
                    diffCols.append(it.key());
            }
            if (!diffCols.isEmpty()) {
                row["_status"] = "diff";
                row["_diffCols"] = diffCols.join(",");
                filteredRows.append(row);
            }
        }
    }
    return filteredRows;
}

QVariantList PatchDbModel::getOldDbRows(const QString &tableName) {
    QString pk = getPrimaryKey(tableName);
    QVariantList oldRows = fetchAllRows(tableName, pk, m_connOld);
    QVariantList newRows = fetchAllRows(tableName, pk, m_connNew);

    auto newMap = indexByPk(newRows);
    QVariantList filteredRows;

    for (int i = 0; i < oldRows.size(); i++) {
        QVariantMap row = oldRows[i].toMap();
        QString pkVal = row.value("_pk").toString();

        if (!newMap.contains(pkVal)) {
            row["_status"] = "old_only";
            row["_diffCols"] = "";
            filteredRows.append(row);
        } else {
            QVariantMap newRow = newMap[pkVal];
            QStringList diffCols;
            for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
                if (it.key().startsWith("_")) continue;
                if (it.key() == "created_at" || it.key() == "updated_at") continue;
                if (it.value().toString() != newRow.value(it.key()).toString())
                    diffCols.append(it.key());
            }
            if (!diffCols.isEmpty()) {
                row["_status"] = "diff";
                row["_diffCols"] = diffCols.join(",");
                filteredRows.append(row);
            }
        }
    }
    return filteredRows;
}

// ════════════════════════════════════════════════════════════
// getTableStats
// ════════════════════════════════════════════════════════════

QVariantMap PatchDbModel::getTableStats(const QString &tableName) {
    QVariantMap stats;
    QString pk = getPrimaryKey(tableName);

    QVariantList newRows = fetchAllRows(tableName, pk, m_connNew);
    QVariantList oldRows = fetchAllRows(tableName, pk, m_connOld);

    auto newMap = indexByPk(newRows);
    auto oldMap = indexByPk(oldRows);

    int newOnly = 0, oldOnly = 0, diffCount = 0, matchCount = 0;

    for (auto it = newMap.constBegin(); it != newMap.constEnd(); ++it) {
        if (!oldMap.contains(it.key())) { newOnly++; continue; }
        const QVariantMap &nr = it.value();
        const QVariantMap &or_ = oldMap[it.key()];
        bool same = true;
        for (auto ci = nr.constBegin(); ci != nr.constEnd(); ++ci) {
            if (ci.key().startsWith("_")) continue;
            if (ci.key() == "created_at" || ci.key() == "updated_at") continue;
            if (ci.value().toString() != or_.value(ci.key()).toString()) {
                same = false; break;
            }
        }
        if (same) matchCount++; else diffCount++;
    }

    for (auto it = oldMap.constBegin(); it != oldMap.constEnd(); ++it) {
        if (!newMap.contains(it.key())) oldOnly++;
    }

    stats["totalNew"] = newRows.size();
    stats["totalOld"] = oldRows.size();
    stats["newOnly"] = newOnly;
    stats["oldOnly"] = oldOnly;
    stats["diffCount"] = diffCount;
    stats["matchCount"] = matchCount;
    return stats;
}

int PatchDbModel::getNewRowCount(const QString &tableName) {
    return m_newTableSizes.value(tableName, 0);
}

int PatchDbModel::getOldRowCount(const QString &tableName) {
    return m_oldTableSizes.value(tableName, 0);
}

// ════════════════════════════════════════════════════════════
// fetchRow — helper ambil 1 row dari DB tertentu
// ════════════════════════════════════════════════════════════

QVariantMap PatchDbModel::fetchRow(const QString &tableName, const QVariant &pkValue,
                                    const QString &connName) {
    QVariantMap result;
    if (!QSqlDatabase::contains(connName)) return result;

    QString pk = detectPrimaryKey(tableName, connName);
    QSqlDatabase db = QSqlDatabase::database(connName);
    QSqlQuery q(db);

    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);
    q.prepare(QString("SELECT * FROM \"%1\" WHERE %2 = :pk").arg(tableName, pkCol));
    q.bindValue(":pk", pkValue);

    if (!q.exec() || !q.next()) return result;

    QSqlRecord rec = q.record();
    for (int i = 0; i < rec.count(); i++)
        result[rec.fieldName(i)] = q.value(i);

    return result;
}

// ════════════════════════════════════════════════════════════
// updateCell
// ════════════════════════════════════════════════════════════

bool PatchDbModel::updateCell(const QString &tableName, const QVariant &pkValue,
                               const QString &column, const QVariant &newValue) {
    if (!QSqlDatabase::contains(m_connOld)) return false;

    QString pk = detectPrimaryKey(tableName, m_connOld);
    QSqlDatabase db = QSqlDatabase::database(m_connOld);
    QSqlQuery q(db);

    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);
    q.prepare(QString("UPDATE \"%1\" SET \"%2\" = :val WHERE %3 = :pk")
                  .arg(tableName, column, pkCol));
    q.bindValue(":val", newValue);
    q.bindValue(":pk", pkValue);

    if (!q.exec()) {
        setStatusMessage("❌ Update gagal: " + q.lastError().text());
        return false;
    }

    bumpDataVersion();
    return true;
}

// ════════════════════════════════════════════════════════════
// replaceRowById
// ════════════════════════════════════════════════════════════

bool PatchDbModel::replaceRowById(const QString &tableName, const QVariant &pkValue) {
    if (!QSqlDatabase::contains(m_connNew) || !QSqlDatabase::contains(m_connOld))
        return false;

    QString pk = detectPrimaryKey(tableName, m_connNew);

    // Ambil row dari DB baru
    QVariantMap newRow = fetchRow(tableName, pkValue, m_connNew);
    if (newRow.isEmpty()) {
        setStatusMessage("❌ Row tidak ditemukan di DB baru (PK: " + pkValue.toString() + ")");
        return false;
    }

    // Cek apakah row ada di DB lama
    QVariantMap oldRow = fetchRow(tableName, pkValue, m_connOld);

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);

    if (!oldRow.isEmpty()) {
        // UPDATE existing row
        QStringList setParts;
        QSqlQuery q(dbOld);
        int idx = 0;

        for (auto it = newRow.constBegin(); it != newRow.constEnd(); ++it) {
            if (it.key() == pk) continue;
            setParts << QString("\"%1\" = :v%2").arg(it.key()).arg(idx);
            idx++;
        }

        QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);
        q.prepare(QString("UPDATE \"%1\" SET %2 WHERE %3 = :pk")
                      .arg(tableName, setParts.join(", "), pkCol));

        idx = 0;
        for (auto it = newRow.constBegin(); it != newRow.constEnd(); ++it) {
            if (it.key() == pk) continue;
            q.bindValue(QString(":v%1").arg(idx), it.value());
            idx++;
        }
        q.bindValue(":pk", pkValue);

        if (!q.exec()) {
            setStatusMessage("❌ Replace gagal: " + q.lastError().text());
            return false;
        }
    } else {
        // INSERT new row
        return addRowToOldDb(tableName, pkValue);
    }

    bumpDataVersion();
    setStatusMessage("✅ Row berhasil di-replace (PK: " + pkValue.toString() + ")");
    return true;
}

// ════════════════════════════════════════════════════════════
// addRowToOldDb
// ════════════════════════════════════════════════════════════

bool PatchDbModel::addRowToOldDb(const QString &tableName, const QVariant &pkValue) {
    if (!QSqlDatabase::contains(m_connNew) || !QSqlDatabase::contains(m_connOld))
        return false;

    QVariantMap newRow = fetchRow(tableName, pkValue, m_connNew);
    if (newRow.isEmpty()) {
        setStatusMessage("❌ Row tidak ditemukan di DB baru");
        return false;
    }

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);
    QSqlQuery q(dbOld);

    QStringList cols, placeholders;
    int idx = 0;
    for (auto it = newRow.constBegin(); it != newRow.constEnd(); ++it) {
        cols << QString("\"%1\"").arg(it.key());
        placeholders << QString(":v%1").arg(idx);
        idx++;
    }

    q.prepare(QString("INSERT INTO \"%1\" (%2) VALUES (%3)")
                  .arg(tableName, cols.join(", "), placeholders.join(", ")));

    idx = 0;
    for (auto it = newRow.constBegin(); it != newRow.constEnd(); ++it) {
        q.bindValue(QString(":v%1").arg(idx), it.value());
        idx++;
    }

    if (!q.exec()) {
        setStatusMessage("❌ Add row gagal: " + q.lastError().text());
        return false;
    }

    bumpDataVersion();
    setStatusMessage("✅ Row berhasil ditambahkan (PK: " + pkValue.toString() + ")");
    return true;
}

// ════════════════════════════════════════════════════════════
// addAllNewRows
// ════════════════════════════════════════════════════════════

bool PatchDbModel::addAllNewRows(const QString &tableName) {
    if (!QSqlDatabase::contains(m_connOld) || !QSqlDatabase::contains(m_connNew)) return false;

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);
    dbOld.transaction();

    QVariantList newRows = getNewDbRows(tableName);
    int successCount = 0;
    for (const QVariant &v : newRows) {
        QVariantMap row = v.toMap();
        if (row.value("_status").toString() == "new_only") {
            QVariant pkValue = row.value("_pk");
            QVariantMap rawNewRow = fetchRow(tableName, pkValue, m_connNew);
            if (!rawNewRow.isEmpty()) {
                QStringList cols, placeholders;
                int idx = 0;
                for (auto it = rawNewRow.constBegin(); it != rawNewRow.constEnd(); ++it) {
                    cols << QString("\"%1\"").arg(it.key());
                    placeholders << QString(":v%1").arg(idx);
                    idx++;
                }

                QSqlQuery q(dbOld);
                q.prepare(QString("INSERT INTO \"%1\" (%2) VALUES (%3)")
                              .arg(tableName, cols.join(", "), placeholders.join(", ")));
                
                idx = 0;
                for (auto it = rawNewRow.constBegin(); it != rawNewRow.constEnd(); ++it) {
                    q.bindValue(QString(":v%1").arg(idx), it.value());
                    idx++;
                }
                
                if (q.exec()) {
                    successCount++;
                }
            }
        }
    }

    dbOld.commit();
    if (successCount > 0) {
        m_oldTableSizes[tableName] += successCount;
        bumpDataVersion();
        setStatusMessage(QString("✅ Add All selesai: %1 row ditambahkan ke %2").arg(successCount).arg(tableName));
        return true;
    } else {
        setStatusMessage("Tidak ada row baru untuk ditambahkan.");
        return false;
    }
}

// ════════════════════════════════════════════════════════════
// replaceAllDiffRows
// ════════════════════════════════════════════════════════════

bool PatchDbModel::replaceAllDiffRows(const QString &tableName) {
    if (!QSqlDatabase::contains(m_connOld) || !QSqlDatabase::contains(m_connNew)) return false;

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);
    dbOld.transaction();

    QString pk = detectPrimaryKey(tableName, m_connNew);
    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);

    QVariantList newRows = getNewDbRows(tableName);
    int successCount = 0;
    for (const QVariant &v : newRows) {
        QVariantMap row = v.toMap();
        if (row.value("_status").toString() == "diff") {
            QVariant pkValue = row.value("_pk");
            QVariantMap rawNewRow = fetchRow(tableName, pkValue, m_connNew);
            if (!rawNewRow.isEmpty()) {
                QStringList setParts;
                int idx = 0;
                for (auto it = rawNewRow.constBegin(); it != rawNewRow.constEnd(); ++it) {
                    if (it.key() == pk) continue;
                    setParts << QString("\"%1\" = :v%2").arg(it.key()).arg(idx);
                    idx++;
                }

                QSqlQuery q(dbOld);
                q.prepare(QString("UPDATE \"%1\" SET %2 WHERE %3 = :pk")
                              .arg(tableName, setParts.join(", "), pkCol));

                idx = 0;
                for (auto it = rawNewRow.constBegin(); it != rawNewRow.constEnd(); ++it) {
                    if (it.key() == pk) continue;
                    q.bindValue(QString(":v%1").arg(idx), it.value());
                    idx++;
                }
                q.bindValue(":pk", pkValue);

                if (q.exec()) {
                    successCount++;
                }
            }
        }
    }

    dbOld.commit();
    if (successCount > 0) {
        bumpDataVersion();
        setStatusMessage(QString("✅ Replace All selesai: %1 row di-replace di %2").arg(successCount).arg(tableName));
        return true;
    } else {
        setStatusMessage("Tidak ada row dengan data beda untuk di-replace.");
        return false;
    }
}

// ════════════════════════════════════════════════════════════
// deleteOldRowById
// ════════════════════════════════════════════════════════════

bool PatchDbModel::deleteOldRowById(const QString &tableName, const QVariant &pkValue) {
    if (!QSqlDatabase::contains(m_connOld)) return false;

    QString pk = detectPrimaryKey(tableName, m_connOld);
    QSqlDatabase db = QSqlDatabase::database(m_connOld);
    QSqlQuery q(db);

    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);
    q.prepare(QString("DELETE FROM \"%1\" WHERE %2 = :pk").arg(tableName, pkCol));
    q.bindValue(":pk", pkValue);

    if (!q.exec()) {
        setStatusMessage("❌ Delete row gagal: " + q.lastError().text());
        return false;
    }

    if (m_oldTableSizes.contains(tableName) && m_oldTableSizes[tableName] > 0) {
        m_oldTableSizes[tableName]--;
    }
    bumpDataVersion();
    setStatusMessage("✅ Row berhasil dihapus (PK: " + pkValue.toString() + ")");
    return true;
}

// ════════════════════════════════════════════════════════════
// deleteAllOldOnlyRows
// ════════════════════════════════════════════════════════════

bool PatchDbModel::deleteAllOldOnlyRows(const QString &tableName) {
    if (!QSqlDatabase::contains(m_connOld)) return false;

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);
    dbOld.transaction();

    QString pk = detectPrimaryKey(tableName, m_connOld);
    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);

    QVariantList oldRows = getOldDbRows(tableName);
    int successCount = 0;

    QSqlQuery q(dbOld);
    q.prepare(QString("DELETE FROM \"%1\" WHERE %2 = :pk").arg(tableName, pkCol));

    for (const QVariant &v : oldRows) {
        QVariantMap row = v.toMap();
        if (row.value("_status").toString() == "old_only") {
            q.bindValue(":pk", row.value("_pk"));
            if (q.exec()) {
                successCount++;
            }
        }
    }

    dbOld.commit();
    if (successCount > 0) {
        if (m_oldTableSizes.contains(tableName)) {
            m_oldTableSizes[tableName] -= successCount;
        }
        bumpDataVersion();
        setStatusMessage(QString("✅ Delete All selesai: %1 row dihapus dari %2").arg(successCount).arg(tableName));
        return true;
    } else {
        setStatusMessage("Tidak ada row lama (old_only) untuk dihapus.");
        return false;
    }
}

// ════════════════════════════════════════════════════════════
// savePendingChanges — batch update
// ════════════════════════════════════════════════════════════

bool PatchDbModel::savePendingChanges(const QString &tableName, const QVariantList &changes) {
    // changes = [{pk: val, column: "col", value: "newVal"}, ...]
    if (!QSqlDatabase::contains(m_connOld)) return false;

    int ok = 0, fail = 0;
    for (const QVariant &item : changes) {
        QVariantMap change = item.toMap();
        QVariant pk = change.value("pk");
        QString col = change.value("column").toString();
        QVariant val = change.value("value");

        if (updateCell(tableName, pk, col, val))
            ok++;
        else
            fail++;
    }

    setStatusMessage(QString("💾 Simpan selesai: %1 berhasil, %2 gagal").arg(ok).arg(fail));
    bumpDataVersion();
    return fail == 0;
}
