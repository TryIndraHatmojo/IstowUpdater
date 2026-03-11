#include "IstowImporter.h"
#include "localrepository.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDateTime>
#include <QUrl>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSet>

// ════════════════════════════════════════════════════════════
// Constructor
// ════════════════════════════════════════════════════════════

IstowImporter::IstowImporter(QObject *parent)
    : QObject(parent)
{
}

// ════════════════════════════════════════════════════════════
// Properties
// ════════════════════════════════════════════════════════════

QVariantMap IstowImporter::shipDetails() const { return m_shipDetails; }
bool IstowImporter::importing() const { return m_importing; }
QString IstowImporter::statusMessage() const { return m_statusMessage; }
QStringList IstowImporter::importLogs() const { return m_importLogs; }
QStringList IstowImporter::dbCompareLogs() const { return m_dbCompareLogs; }

void IstowImporter::clearLogs()
{
    m_importLogs.clear();
    emit importLogsChanged();
}

void IstowImporter::clearDbLogs()
{
    m_dbCompareLogs.clear();
    emit dbCompareLogsChanged();
}

void IstowImporter::appendLog(const QString &msg)
{
    m_importLogs.append(msg);
    emit importLogsChanged();
    qDebug() << "[IstowImporter Log]" << msg;
}

void IstowImporter::appendDbLog(const QString &msg)
{
    m_dbCompareLogs.append(msg);
    emit dbCompareLogsChanged();
    qDebug() << "[IstowImporter DbLog]" << msg;
}

void IstowImporter::setImporting(bool value)
{
    if (m_importing != value) {
        m_importing = value;
        emit importingChanged();
    }
}

void IstowImporter::setStatusMessage(const QString &msg)
{
    m_statusMessage = msg;
    emit statusMessageChanged();
    qDebug() << "[IstowImporter]" << msg;
}

// ════════════════════════════════════════════════════════════
// Helper: workDir
// ════════════════════════════════════════════════════════════

QString IstowImporter::workDir() const
{
    // Konvensi iStowV2: [home]/iStowV2/
    // Contoh: C:/Users/T480/iStowV2/
    return QDir::homePath() + "/iStowV2/";
}

// ════════════════════════════════════════════════════════════
// Helper: toLocalPath — konversi URL FileDialog ke local path
// ════════════════════════════════════════════════════════════

QString IstowImporter::toLocalPath(const QString &urlPath) const
{
    QUrl url(urlPath);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    return urlPath;
}

// ════════════════════════════════════════════════════════════
// Helper: backupFile — backup sebelum overwrite
// ════════════════════════════════════════════════════════════

bool IstowImporter::backupFile(const QString &filePath)
{
    QFile existing(filePath);
    if (!existing.exists()) {
        // Tidak perlu backup, file belum ada
        return true;
    }

    QFileInfo fi(filePath);
    QString wDir = workDir();

    // Hitung relative path dari workDir
    // misal filePath = "C:/.../iStow/assets/images/ship.png"
    //       wDir     = "C:/.../iStow/"
    //       relPath  = "assets/images/ship.png"
    QString relPath = filePath;
    if (relPath.startsWith(wDir)) {
        relPath = relPath.mid(wDir.length());
    }

    // Backup disimpan di iStowV2/IstowUpdater/backup_timestamp/ dengan struktur folder yang sama
    // misal: C:/Users/T480/iStowV2/IstowUpdater/backup_20260304_125300/assets/images/ship.png
    QFileInfo relFi(relPath);
    
    // Jika tidak diset, buat fallback timestamp
    QString folderName = m_currentBackupFolder.isEmpty() 
        ? "backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") 
        : m_currentBackupFolder;

    QString backupDir = wDir + "IstowUpdater/" + folderName + "/" + relFi.path();
    QDir().mkpath(backupDir);

    // Nama backup sama seperti nama file aslinya
    QString backupPath = backupDir + "/" + fi.fileName();

    if (QFile::copy(filePath, backupPath)) {
        qDebug() << "[IstowImporter] Backup dibuat:" << backupPath;
        return true;
    } else {
        qWarning() << "[IstowImporter] GAGAL membuat backup:" << filePath << "->" << backupPath;
        return false;
    }
}

// ════════════════════════════════════════════════════════════
// readDetails — Baca metadata .details dari arsip .istow
// ════════════════════════════════════════════════════════════

bool IstowImporter::readDetails(const QString &filePath)
{
    QString localPath = toLocalPath(filePath);
    setStatusMessage("Membaca metadata dari: " + localPath);

    QFile file(localPath);
    if (!file.exists()) {
        setStatusMessage("ERROR: File tidak ditemukan: " + localPath);
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        setStatusMessage("ERROR: Gagal membuka file: " + localPath);
        return false;
    }

    QDataStream dataStream(&file);

    while (!dataStream.atEnd()) {
        QString fileName;
        QByteArray data;
        dataStream >> fileName >> data;

        if (dataStream.status() != QDataStream::Ok) {
            setStatusMessage("ERROR: Format file .istow tidak valid");
            file.close();
            return false;
        }

        // Cari file .details
        if (fileName.endsWith(".details")) {
            QByteArray uncompressed = qUncompress(data);
            QJsonDocument doc = QJsonDocument::fromJson(uncompressed);

            if (doc.isNull() || !doc.isObject()) {
                setStatusMessage("ERROR: Gagal parse .details JSON");
                file.close();
                return false;
            }

            QJsonObject obj = doc.object();
            m_shipDetails = obj.toVariantMap();
            emit detailsChanged();

            setStatusMessage("Metadata berhasil dibaca: " +
                             obj.value("nama").toString());
            file.close();
            return true;
        }
    }

    setStatusMessage("WARNING: File .details tidak ditemukan dalam arsip .istow");
    file.close();
    return false;
}

// ════════════════════════════════════════════════════════════
// importAssets — Ekstrak semua asset KECUALI file .db
// ════════════════════════════════════════════════════════════

bool IstowImporter::importAssets(const QString &filePath)
{
    QString localPath = toLocalPath(filePath);
    setImporting(true);
    setStatusMessage("Mulai import assets dari: " + localPath);

    // Set folder backup untuk sesi import kali ini
    m_currentBackupFolder = "backup_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    QString wDir = workDir();

    QFile file(localPath);
    if (!file.exists()) {
        setStatusMessage("ERROR: File tidak ditemukan: " + localPath);
        setImporting(false);
        emit importFinished(false, "File tidak ditemukan");
        return false;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        setStatusMessage("ERROR: Gagal membuka file: " + localPath);
        setImporting(false);
        emit importFinished(false, "Gagal membuka file");
        return false;
    }

    // Pastikan workDir ada
    QDir().mkpath(wDir);

    QDataStream dataStream(&file);
    int assetCount = 0;
    int skippedDb = 0;

    while (!dataStream.atEnd()) {
        QString fileName;
        QByteArray data;
        dataStream >> fileName >> data;

        if (dataStream.status() != QDataStream::Ok) {
            setStatusMessage("ERROR: Data stream error saat membaca arsip");
            break;
        }

        // ─── SKIP file .db ───────────────────────────────
        if (fileName.endsWith(".db")) {
            qDebug() << "[IstowImporter] SKIP file DB:" << fileName;
            skippedDb++;
            continue;
        }

        // ─── Buat subfolder jika perlu ───────────────────
        for (int i = fileName.length() - 1; i > 0; i--) {
            if (fileName.at(i) == '\\' || fileName.at(i) == '/') {
                QString subfolder = fileName.left(i);
                QDir().mkpath(wDir + subfolder);
                break;
            }
        }

        // ─── Backup file lama jika sudah ada ─────────────
        QString outPath = wDir + fileName;
        if (QFile::exists(outPath)) {
            if (!backupFile(outPath)) {
                qWarning() << "[IstowImporter] Gagal backup, skip file:" << fileName;
                appendLog("❌ LEWATI: " + fileName + " (Gagal backup)");
                continue;
            }
            // Hapus file lama agar bisa ditulis ulang
            QFile::remove(outPath);
            appendLog("🔄 REPLACE: " + fileName);
        } else {
            appendLog("✅ ADD: " + fileName);
        }

        // ─── Tulis file asset ────────────────────────────
        QFile outFile(outPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            qWarning() << "[IstowImporter] Gagal menulis:" << outPath;
            continue;
        }
        outFile.write(qUncompress(data));
        outFile.close();
        assetCount++;

        qDebug() << "[IstowImporter] Asset diekstrak:" << fileName;
    }

    file.close();
    setImporting(false);

    QString msg = QString("Import selesai: %1 asset diekstrak, %2 file DB di-skip")
                      .arg(assetCount).arg(skippedDb);
    setStatusMessage(msg);
    emit importFinished(true, msg);
    return true;
}

// ════════════════════════════════════════════════════════════
// extractDbToTemp — Ekstrak HANYA file .db ke folder temporary
// ════════════════════════════════════════════════════════════

QString IstowImporter::extractDbToTemp(const QString &filePath)
{
    QString localPath = toLocalPath(filePath);
    setStatusMessage("Mengekstrak database ke temporary...");

    QFile file(localPath);
    if (!file.open(QIODevice::ReadOnly)) {
        setStatusMessage("ERROR: Gagal membuka file untuk ekstrak DB");
        return "";
    }

    // Buat folder temporary
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                      + "/IstowUpdater_temp";
    QDir().mkpath(tempDir);

    QDataStream dataStream(&file);

    while (!dataStream.atEnd()) {
        QString fileName;
        QByteArray data;
        dataStream >> fileName >> data;

        if (dataStream.status() != QDataStream::Ok) {
            break;
        }

        if (fileName.endsWith(".db")) {
            // Ambil nama file saja (tanpa subfolder)
            QString dbBaseName = QFileInfo(fileName).fileName();
            QString tempDbPath = tempDir + "/" + dbBaseName;

            // Backup jika sudah ada file temp sebelumnya
            if (QFile::exists(tempDbPath)) {
                QFile::remove(tempDbPath);
                appendLog("🔄 REPLACE (Temp DB): " + dbBaseName);
            } else {
                appendLog("✅ ADD (Temp DB): " + dbBaseName);
            }

            QFile outFile(tempDbPath);
            if (outFile.open(QIODevice::WriteOnly)) {
                outFile.write(qUncompress(data));
                outFile.close();
                file.close();

                setStatusMessage("DB diekstrak ke: " + tempDbPath);
                return tempDbPath;
            } else {
                qWarning() << "[IstowImporter] Gagal menulis DB temp:" << tempDbPath;
            }
        }
    }

    file.close();
    setStatusMessage("WARNING: Tidak ada file .db dalam arsip .istow");
    return "";
}

// ════════════════════════════════════════════════════════════
// registerToLocalDb — Register metadata kapal ke shiptbl
// ════════════════════════════════════════════════════════════

bool IstowImporter::registerToLocalDb()
{
    if (m_shipDetails.isEmpty()) {
        setStatusMessage("ERROR: Tidak ada metadata kapal. Jalankan readDetails() dulu.");
        return false;
    }

    int idship       = m_shipDetails.value("idship", -1).toInt();
    QString tipe     = m_shipDetails.value("tipe", "").toString();
    QString nama     = m_shipDetails.value("nama", "").toString();
    QString dbid     = m_shipDetails.value("dbid", "").toString();
    QString prefix   = m_shipDetails.value("fileprefix", "").toString();
    QString company  = m_shipDetails.value("company", "").toString();
    int version      = m_shipDetails.value("version", 0).toInt();

    if (idship < 0) {
        setStatusMessage("ERROR: idship tidak valid: " + QString::number(idship));
        return false;
    }

    setStatusMessage("Registrasi kapal ke LocalDB: " + nama + " (ID: " + QString::number(idship) + ")");

    LocalRepository *repo = LocalRepository::getInstance();

    // Cek apakah kapal sudah pernah terdaftar
    if (repo->checkShipExists(idship)) {
        appendLog("🔄 UPDATE data kapal di LocalDB");
        repo->updateShip(idship, tipe, nama, dbid, prefix, company, version);
    } else {
        appendLog("✅ INSERT data kapal baru di LocalDB");
        repo->insertShip(idship, tipe, nama, dbid, prefix, company, version);
    }

    setStatusMessage("Kapal berhasil diregistrasi: " + nama);
    return true;
}

// ════════════════════════════════════════════════════════════
// overwriteDatabase — DISABLED
// ════════════════════════════════════════════════════════════
//
// Bagian ini dinonaktifkan karena database harus di-compare
// dengan data lama terlebih dahulu di step selanjutnya.
//
// bool IstowImporter::overwriteDatabase(const QString &tempDbPath,
//                                        const QString &targetDbPath)
// {
//     setStatusMessage("Overwrite database...");
//
//     // Validasi file sumber (DB baru dari temp)
//     if (!QFile::exists(tempDbPath)) {
//         setStatusMessage("ERROR: File DB sumber tidak ditemukan: " + tempDbPath);
//         return false;
//     }
//
//     // ─── BACKUP DB lama sebelum overwrite ─────────────
//     if (QFile::exists(targetDbPath)) {
//         if (!backupFile(targetDbPath)) {
//             setStatusMessage("ERROR: Gagal membuat backup DB lama. Overwrite dibatalkan.");
//             return false;
//         }
//         // Hapus DB lama
//         QFile::remove(targetDbPath);
//     }
//
//     // ─── Copy DB baru ke target ──────────────────────
//     if (QFile::copy(tempDbPath, targetDbPath)) {
//         setStatusMessage("Database berhasil di-overwrite: " + targetDbPath);
//         return true;
//     } else {
//         setStatusMessage("ERROR: Gagal meng-copy database ke: " + targetDbPath);
//         return false;
//     }
// }

// ════════════════════════════════════════════════════════════
// compareAndMigrateDb — Compare dan migrate schema DB (additive only)
// ════════════════════════════════════════════════════════════

bool IstowImporter::compareAndMigrateDb(const QString &tempDbPath)
{
    clearDbLogs();

    if (m_shipDetails.isEmpty()) {
        appendDbLog("❌ ERROR: Metadata kapal belum dibaca");
        return false;
    }

    // ─── Tentukan path DB lama di iStowV2 ─────────────
    QString dbid = m_shipDetails.value("dbid", "").toString();
    if (dbid.isEmpty()) {
        appendDbLog("❌ ERROR: dbid tidak ada di metadata");
        return false;
    }

    QString oldDbPath = workDir() + dbid;
    appendDbLog("📂 DB Lama: " + oldDbPath);
    appendDbLog("📂 DB Baru (temp): " + tempDbPath);

    // ─── Cek apakah DB lama ada ─────────────────────
    if (!QFile::exists(oldDbPath)) {
        // DB lama tidak ada, copy DB baru langsung
        appendDbLog("ℹ️ DB lama tidak ditemukan. Copy DB baru langsung...");
        
        // Pastikan folder tujuan ada
        QFileInfo fi(oldDbPath);
        QDir().mkpath(fi.absolutePath());
        
        if (QFile::copy(tempDbPath, oldDbPath)) {
            appendDbLog("✅ DB baru berhasil di-copy ke: " + oldDbPath);
            return true;
        } else {
            appendDbLog("❌ GAGAL meng-copy DB baru ke: " + oldDbPath);
            return false;
        }
    }

    if (!QFile::exists(tempDbPath)) {
        appendDbLog("❌ ERROR: File DB baru tidak ditemukan: " + tempDbPath);
        return false;
    }

    // ─── Buka kedua database ───────────────────────
    QString connOld = "compare_old_" + QString::number(quintptr(this));
    QString connNew = "compare_new_" + QString::number(quintptr(this));

    {
        QSqlDatabase dbOld = QSqlDatabase::addDatabase("QSQLITE", connOld);
        dbOld.setDatabaseName(oldDbPath);
        if (!dbOld.open()) {
            appendDbLog("❌ GAGAL membuka DB lama: " + dbOld.lastError().text());
            return false;
        }

        QSqlDatabase dbNew = QSqlDatabase::addDatabase("QSQLITE", connNew);
        dbNew.setDatabaseName(tempDbPath);
        if (!dbNew.open()) {
            appendDbLog("❌ GAGAL membuka DB baru: " + dbNew.lastError().text());
            dbOld.close();
            return false;
        }

        appendDbLog("🔍 Memulai perbandingan schema...");
        appendDbLog("═══════════════════════════════════");

        int tablesAdded = 0;
        int columnsAdded = 0;
        int triggersAdded = 0;

        // ════════════════════════════════════════════════
        // 1. COMPARE TABLES — tambah tabel baru
        // ════════════════════════════════════════════════

        // Ambil daftar tabel dari DB lama
        QSet<QString> oldTables;
        {
            QSqlQuery q(dbOld);
            q.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
            while (q.next()) {
                oldTables.insert(q.value(0).toString());
            }
        }

        // Ambil daftar tabel + CREATE statement dari DB baru
        QMap<QString, QString> newTablesDDL;
        {
            QSqlQuery q(dbNew);
            q.exec("SELECT name, sql FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
            while (q.next()) {
                newTablesDDL.insert(q.value(0).toString(), q.value(1).toString());
            }
        }

        for (auto it = newTablesDDL.constBegin(); it != newTablesDDL.constEnd(); ++it) {
            const QString &tableName = it.key();
            const QString &createSql = it.value();

            if (!oldTables.contains(tableName)) {
                // Tabel baru — buat di DB lama
                appendDbLog("🆕 Tambah tabel: " + tableName);
                QSqlQuery q(dbOld);
                if (q.exec(createSql)) {
                    appendDbLog("   ✅ Berhasil dibuat");
                    tablesAdded++;
                } else {
                    appendDbLog("   ❌ Gagal: " + q.lastError().text());
                }
            }
        }

        // ════════════════════════════════════════════════
        // 2. COMPARE COLUMNS — tambah kolom baru ke tabel yang sudah ada
        // ════════════════════════════════════════════════

        for (const QString &tableName : oldTables) {
            if (!newTablesDDL.contains(tableName)) {
                continue; // Tabel hanya ada di DB lama, skip
            }

            // Ambil kolom DB lama
            QSet<QString> oldColumns;
            {
                QSqlQuery q(dbOld);
                q.exec(QString("PRAGMA table_info('%1')").arg(tableName));
                while (q.next()) {
                    oldColumns.insert(q.value(1).toString().toLower());
                }
            }

            // Ambil kolom DB baru beserta tipe & default
            struct ColInfo {
                QString name;
                QString type;
                bool notNull;
                QString dfltValue;
            };
            QList<ColInfo> newColumns;
            {
                QSqlQuery q(dbNew);
                q.exec(QString("PRAGMA table_info('%1')").arg(tableName));
                while (q.next()) {
                    ColInfo ci;
                    ci.name = q.value(1).toString();
                    ci.type = q.value(2).toString();
                    ci.notNull = q.value(3).toBool();
                    ci.dfltValue = q.value(4).toString();
                    newColumns.append(ci);
                }
            }

            for (const ColInfo &col : newColumns) {
                if (!oldColumns.contains(col.name.toLower())) {
                    // Kolom baru — ALTER TABLE ADD COLUMN
                    QString alterSql = QString("ALTER TABLE \"%1\" ADD COLUMN \"%2\" %3")
                                          .arg(tableName, col.name, col.type);

                    // Tambahkan DEFAULT jika ada
                    if (!col.dfltValue.isEmpty()) {
                        alterSql += " DEFAULT " + col.dfltValue;
                    }

                    appendDbLog("📎 Tambah kolom: " + tableName + "." + col.name + " (" + col.type + ")");
                    QSqlQuery q(dbOld);
                    if (q.exec(alterSql)) {
                        appendDbLog("   ✅ Berhasil");
                        columnsAdded++;
                    } else {
                        appendDbLog("   ❌ Gagal: " + q.lastError().text());
                    }
                }
            }
        }

        // ════════════════════════════════════════════════
        // 3. COMPARE TRIGGERS — tambah trigger baru
        // ════════════════════════════════════════════════

        QSet<QString> oldTriggers;
        {
            QSqlQuery q(dbOld);
            q.exec("SELECT name FROM sqlite_master WHERE type='trigger'");
            while (q.next()) {
                oldTriggers.insert(q.value(0).toString());
            }
        }

        QMap<QString, QString> newTriggersDDL;
        {
            QSqlQuery q(dbNew);
            q.exec("SELECT name, sql FROM sqlite_master WHERE type='trigger'");
            while (q.next()) {
                QString name = q.value(0).toString();
                QString sql = q.value(1).toString();
                if (!sql.isEmpty()) {
                    newTriggersDDL.insert(name, sql);
                }
            }
        }

        for (auto it = newTriggersDDL.constBegin(); it != newTriggersDDL.constEnd(); ++it) {
            if (!oldTriggers.contains(it.key())) {
                appendDbLog("⚡ Tambah trigger: " + it.key());
                QSqlQuery q(dbOld);
                if (q.exec(it.value())) {
                    appendDbLog("   ✅ Berhasil");
                    triggersAdded++;
                } else {
                    appendDbLog("   ❌ Gagal: " + q.lastError().text());
                }
            }
        }

        // ════════════════════════════════════════════════
        // 4. COMPARE VIEWS — tambah view baru
        // ════════════════════════════════════════════════

        QSet<QString> oldViews;
        {
            QSqlQuery q(dbOld);
            q.exec("SELECT name FROM sqlite_master WHERE type='view'");
            while (q.next()) {
                oldViews.insert(q.value(0).toString());
            }
        }

        QMap<QString, QString> newViewsDDL;
        {
            QSqlQuery q(dbNew);
            q.exec("SELECT name, sql FROM sqlite_master WHERE type='view'");
            while (q.next()) {
                QString name = q.value(0).toString();
                QString sql = q.value(1).toString();
                if (!sql.isEmpty()) {
                    newViewsDDL.insert(name, sql);
                }
            }
        }

        int viewsAdded = 0;
        for (auto it = newViewsDDL.constBegin(); it != newViewsDDL.constEnd(); ++it) {
            if (!oldViews.contains(it.key())) {
                appendDbLog("👁 Tambah view: " + it.key());
                QSqlQuery q(dbOld);
                if (q.exec(it.value())) {
                    appendDbLog("   ✅ Berhasil");
                    viewsAdded++;
                } else {
                    appendDbLog("   ❌ Gagal: " + q.lastError().text());
                }
            }
        }

        // ════════════════════════════════════════════════
        // Summary
        // ════════════════════════════════════════════════

        appendDbLog("═══════════════════════════════════");
        appendDbLog(QString("📊 Ringkasan: %1 tabel, %2 kolom, %3 trigger, %4 view ditambahkan")
                        .arg(tablesAdded).arg(columnsAdded).arg(triggersAdded).arg(viewsAdded));

        if (tablesAdded == 0 && columnsAdded == 0 && triggersAdded == 0 && viewsAdded == 0) {
            appendDbLog("✅ Schema DB sudah identik — tidak ada perubahan");
        }

        // Close connections
        dbOld.close();
        dbNew.close();
    }

    // Remove connections outside the scope
    QSqlDatabase::removeDatabase(connOld);
    QSqlDatabase::removeDatabase(connNew);

    setStatusMessage("Compare & Migrate DB selesai");
    return true;
}
