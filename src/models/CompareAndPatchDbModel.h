#ifndef COMPAREANDPATCHDBMODEL_H
#define COMPAREANDPATCHDBMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QMap>
#include <QtQml/qqmlregistration.h>

/*!
 * \brief Model untuk fitur Compare and Patch DB.
 *
 * Menangani:
 * - Browse 2 file .db (New DB dan Target Patch DB)
 * - Preview perbandingan schema sebelum Sync Structure
 * - Sync DB Structure (tables, columns, triggers, views)  — adaptasi dari IstowImporter::compareAndMigrateDb
 * - Sync DB Data — adaptasi dari PatchDbModel::loadAndCompare
 * - CRUD per-row dan bulk operations pada Target DB
 *
 * Workflow:
 * 1. User pilih New DB dan Target DB
 * 2. User klik "Preview Structure" → muncul preview diff (belum apply)
 * 3. User klik "Apply Sync Structure" → tabel/kolom/trigger/view baru ditambahkan
 * 4. User klik "Sync DB Data" → compare data, tampilkan diff per tabel
 * 5. User patch data secara selektif atau bulk
 */
class CompareAndPatchDbModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // ── State properties ──────────────────────────────
    Q_PROPERTY(bool comparing READ comparing NOTIFY comparingChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(bool structureSynced READ structureSynced NOTIFY structureSyncedChanged)
    Q_PROPERTY(bool structurePreviewed READ structurePreviewed NOTIFY structurePreviewedChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

    // ── Path properties ───────────────────────────────
    Q_PROPERTY(QString newDbPath READ newDbPath NOTIFY newDbPathChanged)
    Q_PROPERTY(QString targetDbPath READ targetDbPath NOTIFY targetDbPathChanged)

    // ── Structure logs ────────────────────────────────
    Q_PROPERTY(QStringList structureLogs READ structureLogs NOTIFY structureLogsChanged)

    // ── Data comparison ───────────────────────────────
    Q_PROPERTY(QStringList diffTableList READ diffTableList NOTIFY diffTableListChanged)
    Q_PROPERTY(QStringList allTableList READ allTableList NOTIFY allTableListChanged)
    Q_PROPERTY(int dataVersion READ dataVersion NOTIFY dataVersionChanged)

public:
    explicit CompareAndPatchDbModel(QObject *parent = nullptr);
    ~CompareAndPatchDbModel();

    // ── Property Accessors ──────────────────────────
    bool comparing() const;
    bool loaded() const;
    bool structureSynced() const;
    bool structurePreviewed() const;
    QString statusMessage() const;
    QString newDbPath() const;
    QString targetDbPath() const;
    QStringList structureLogs() const;
    QStringList diffTableList() const;
    QStringList allTableList() const;
    int dataVersion() const;

    // ── Path Setters ────────────────────────────────
    Q_INVOKABLE void setNewDbPath(const QString &path);
    Q_INVOKABLE void setTargetDbPath(const QString &path);

    // ── Structure Operations ────────────────────────

    /*!
     * \brief Preview schema differences tanpa apply.
     * Membaca kedua DB, compare tables/columns/triggers/views,
     * dan populate structureLogs sebagai preview.
     */
    Q_INVOKABLE bool previewStructure();

    /*!
     * \brief Apply sync structure setelah preview.
     * Menjalankan ALTER TABLE, CREATE TABLE, dll di Target DB.
     */
    Q_INVOKABLE bool applySyncStructure();

    // ── Data Operations ─────────────────────────────

    /*!
     * \brief Compare data antara kedua DB.
     * Harus sudah syncStructure dulu.
     */
    Q_INVOKABLE bool syncData();

    /*!
     * \brief Reset state dan tutup koneksi.
     */
    Q_INVOKABLE void cleanup();

    // ── Data Accessors ──────────────────────────────
    Q_INVOKABLE QVariantList getColumns(const QString &tableName);
    Q_INVOKABLE QString getPrimaryKey(const QString &tableName);
    Q_INVOKABLE QVariantList getNewDbRows(const QString &tableName);
    Q_INVOKABLE QVariantList getOldDbRows(const QString &tableName);
    Q_INVOKABLE QVariantMap getTableStats(const QString &tableName);
    Q_INVOKABLE int getNewRowCount(const QString &tableName);
    Q_INVOKABLE int getOldRowCount(const QString &tableName);

    // ── Data Modification ───────────────────────────
    Q_INVOKABLE bool updateCell(const QString &tableName, const QVariant &pkValue,
                                 const QString &column, const QVariant &newValue);
    Q_INVOKABLE bool replaceRowById(const QString &tableName, const QVariant &pkValue);
    Q_INVOKABLE bool addRowToOldDb(const QString &tableName, const QVariant &pkValue);
    Q_INVOKABLE bool addAllNewRows(const QString &tableName);
    Q_INVOKABLE bool replaceAllDiffRows(const QString &tableName);
    Q_INVOKABLE bool deleteOldRowById(const QString &tableName, const QVariant &pkValue);
    Q_INVOKABLE bool deleteAllOldOnlyRows(const QString &tableName);
    Q_INVOKABLE bool savePendingChanges(const QString &tableName, const QVariantList &changes);

    // ── Utility ─────────────────────────────────────
    Q_INVOKABLE void copyToClipboard(const QString &text);

signals:
    void comparingChanged();
    void loadedChanged();
    void structureSyncedChanged();
    void structurePreviewedChanged();
    void statusMessageChanged();
    void newDbPathChanged();
    void targetDbPathChanged();
    void structureLogsChanged();
    void diffTableListChanged();
    void allTableListChanged();
    void dataVersionChanged();

private:
    // ── State ───────────────────────────────────────
    bool m_comparing = false;
    bool m_loaded = false;
    bool m_structureSynced = false;
    bool m_structurePreviewed = false;
    QString m_statusMessage;

    // ── Paths ───────────────────────────────────────
    QString m_newDbPath;
    QString m_targetDbPath;

    // ── Structure logs ──────────────────────────────
    QStringList m_structureLogs;

    // ── Data comparison ─────────────────────────────
    QStringList m_diffTableList;
    QStringList m_allTableList;
    int m_dataVersion = 0;

    // ── DB connections ──────────────────────────────
    QString m_connNew;
    QString m_connOld;

    // ── Table sizes cache ───────────────────────────
    QMap<QString, int> m_newTableSizes;
    QMap<QString, int> m_oldTableSizes;

    // ── Helpers ─────────────────────────────────────
    void setComparing(bool v);
    void setStatusMessage(const QString &msg);
    void setLoaded(bool v);
    void setStructureSynced(bool v);
    void setStructurePreviewed(bool v);
    void bumpDataVersion();

    QString toLocalPath(const QString &urlPath) const;
    QString detectPrimaryKey(const QString &tableName, const QString &connName);
    void closeConnections();
    void appendStructureLog(const QString &msg);

    QVariantMap fetchRow(const QString &tableName, const QVariant &pkValue,
                         const QString &connName);
};

#endif // COMPAREANDPATCHDBMODEL_H
