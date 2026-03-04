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
    // Sama dengan konvensi iStowV2: AppDataLocation + "/"
    // Contoh: C:/Users/T480/AppData/Roaming/Pranala Digital Transmaritim/Stowage Planning Software iStow/
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appData + "/";
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
    QString backupDir = fi.absolutePath() + "/backup";
    QDir().mkpath(backupDir);

    // Nama backup: namafile_YYYYMMDD_HHmmss.ext
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    QString backupName = fi.baseName() + "_" + timestamp + "." + fi.suffix();
    QString backupPath = backupDir + "/" + backupName;

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
                continue;
            }
            // Hapus file lama agar bisa ditulis ulang
            QFile::remove(outPath);
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

    // Hapus data lama (jika ada) lalu insert baru
    repo->deleteShip(idship);
    repo->insertShip(idship, tipe, nama, dbid, prefix, company, version);

    setStatusMessage("Kapal berhasil didaftarkan: " + nama);
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
