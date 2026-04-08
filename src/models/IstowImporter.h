#ifndef ISTOWIMPORTER_H
#define ISTOWIMPORTER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QJsonObject>
#include <QStringList>
#include <QtQml/qqmlregistration.h>

/*!
 * \brief Class untuk mengimport file .istow
 *
 * Menangani:
 * - Membaca metadata (.details) dari arsip .istow
 * - Mengekstrak file asset (skip .db) ke working directory
 * - Mengekstrak file .db ke folder temporary
 * - Overwrite database (DISABLED — akan diaktifkan setelah compare)
 * - Backup otomatis sebelum overwrite
 */
class IstowImporter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantMap shipDetails READ shipDetails NOTIFY detailsChanged)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QStringList importLogs READ importLogs NOTIFY importLogsChanged)
    Q_PROPERTY(QStringList dbCompareLogs READ dbCompareLogs NOTIFY dbCompareLogsChanged)

public:
    explicit IstowImporter(QObject *parent = nullptr);

    // ── Properties ──────────────────────────────────────
    QVariantMap shipDetails() const;
    bool importing() const;
    QString statusMessage() const;
    QStringList importLogs() const;
    QStringList dbCompareLogs() const;

    // ── Q_INVOKABLE — dipanggil dari QML ────────────────
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void clearDbLogs();

    /*!
     * \brief Compare schema DB lama (di iStowV2) dengan DB baru (dari temp).
     * Hanya menambahkan: tabel baru, kolom baru, trigger baru.
     * TIDAK menghapus tabel, kolom, atau trigger yang sudah ada.
     * \param tempDbPath path ke DB baru di folder temporary
     * \return true jika compare & migrate berhasil
     */
    Q_INVOKABLE bool compareAndMigrateDb(const QString &tempDbPath);

    /*!
     * \brief Baca metadata .details dari arsip .istow
     * \param filePath path absolut ke file .istow (dari FileDialog, format file:///)
     * \return true jika .details ditemukan dan berhasil di-parse
     */
    Q_INVOKABLE bool readDetails(const QString &filePath);

    /*!
     * \brief Generate file .details manual dan load ke m_shipDetails
     * \param details QVariantMap berisi data kapal
     * \return true jika berhasil generate
     */
    Q_INVOKABLE bool generateDetailsAndLoad(const QVariantMap &details);

    /*!
     * \brief Ekstrak semua asset dari .istow KECUALI file .db
     * \param filePath path absolut ke file .istow
     * \return true jika ekstraksi berhasil
     *
     * Sebelum overwrite file yang sudah ada, otomatis buat backup.
     */
    Q_INVOKABLE bool importAssets(const QString &filePath);

    /*!
     * \brief Ekstrak hanya file .db dari .istow ke folder temporary
     * \param filePath path absolut ke file .istow
     * \return path ke file .db di folder temporary, atau "" jika gagal
     */
    Q_INVOKABLE QString extractDbToTemp(const QString &filePath);

    /*!
     * \brief Register data kapal ke LocalDB (shiptbl)
     * \return true jika berhasil
     *
     * Menggunakan metadata yang sudah dibaca dari readDetails().
     * Menghapus data lama (deleteShip) lalu insert baru (insertShip).
     */
    Q_INVOKABLE bool registerToLocalDb();

    // ── Overwrite Database (DISABLED) ───────────────────
    // Kode ini dinonaktifkan karena database harus di-compare
    // dengan data lama terlebih dahulu sebelum di-overwrite.
    //
    // Q_INVOKABLE bool overwriteDatabase(const QString &tempDbPath,
    //                                     const QString &targetDbPath);

signals:
    void detailsChanged();
    void importingChanged();
    void statusMessageChanged();
    void importLogsChanged();
    void dbCompareLogsChanged();
    void importFinished(bool success, const QString &message);

private:
    QVariantMap m_shipDetails;
    bool m_importing = false;
    QString m_statusMessage;
    QStringList m_importLogs;
    QStringList m_dbCompareLogs;
    QString m_currentBackupFolder;
    int m_currentBackupSessionId = -1;

    // ── Helper ──────────────────────────────────────────
    void appendLog(const QString &msg);
    void appendDbLog(const QString &msg);

    /*!
     * \brief Dapatkan working directory (homePath) untuk file asset kapal
     * \return path absolut ke folder iStowV2 di AppDataLocation
     */
    QString workDir() const;

    /*!
     * \brief Backup file lama sebelum overwrite
     * \param filePath path absolut ke file yang akan di-backup
     * \return true jika backup berhasil atau file belum ada (tidak perlu backup)
     *
     * File di-copy ke subfolder backup/ dengan suffix timestamp.
     */
    bool backupFile(const QString &filePath);

    /*!
     * \brief Convert URL dari FileDialog ke local path
     * \param urlPath path dari FileDialog (bisa format file:///)
     * \return path lokal yang bisa dipakai QFile
     */
    QString toLocalPath(const QString &urlPath) const;

    void setImporting(bool value);
    void setStatusMessage(const QString &msg);
};

#endif // ISTOWIMPORTER_H
