#ifndef PATCHDBMODEL_H
#define PATCHDBMODEL_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

/*!
 * \brief Model untuk fitur Patch DB Data.
 *
 * Menangani:
 * - Membaca .istow dan mengekstrak database
 * - Compare DATA (value) antara DB baru (.istow) dan DB lama (iStowV2)
 * - Menyediakan akses data per tabel ke QML
 * - Update, replace, dan insert row di DB target
 */
class PatchDbModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool comparing READ comparing NOTIFY comparingChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QStringList diffTableList READ diffTableList NOTIFY diffTableListChanged)
    Q_PROPERTY(QStringList allTableList READ allTableList NOTIFY allTableListChanged)
    Q_PROPERTY(QVariantMap shipDetails READ shipDetails NOTIFY shipDetailsChanged)
    Q_PROPERTY(int dataVersion READ dataVersion NOTIFY dataVersionChanged)
    Q_PROPERTY(QString newDbFileName READ newDbFileName NOTIFY newDbFileNameChanged)

public:
    explicit PatchDbModel(QObject *parent = nullptr);
    ~PatchDbModel();

    // ── Property Accessors ──────────────────────────
    bool comparing() const;
    bool loaded() const;
    QString statusMessage() const;
    QStringList diffTableList() const;
    QStringList allTableList() const;
    QVariantMap shipDetails() const;
    int dataVersion() const;
    QString newDbFileName() const;

    // ── Main Operations ─────────────────────────────

    /*!
     * \brief Load file .istow, ekstrak DB, compare data dengan DB target.
     * \param istowFilePath path ke file .istow (bisa format file:///)
     * \return true jika berhasil
     */
    Q_INVOKABLE bool loadAndCompare(const QString &istowFilePath);

    /*!
     * \brief Tutup koneksi DB dan reset state.
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

    /*!
     * \brief Update satu cell di DB target.
     */
    Q_INVOKABLE bool updateCell(const QString &tableName, const QVariant &pkValue,
                                 const QString &column, const QVariant &newValue);

    /*!
     * \brief Replace seluruh row di DB target dengan data dari DB baru (by PK).
     */
    Q_INVOKABLE bool replaceRowById(const QString &tableName, const QVariant &pkValue);

    /*!
     * \brief Copy row dari DB baru ke DB target (insert).
     */
    Q_INVOKABLE bool addRowToOldDb(const QString &tableName, const QVariant &pkValue);

    /*!
     * \brief Batch update multiple cells: items = [{pk, column, value}, ...]
     */
    Q_INVOKABLE bool savePendingChanges(const QString &tableName, const QVariantList &changes);

    /*!
     * \brief Copy text ke system clipboard.
     */
    Q_INVOKABLE void copyToClipboard(const QString &text);

signals:
    void comparingChanged();
    void loadedChanged();
    void statusMessageChanged();
    void diffTableListChanged();
    void allTableListChanged();
    void shipDetailsChanged();
    void dataVersionChanged();
    void newDbFileNameChanged();

private:
    bool m_comparing = false;
    bool m_loaded = false;
    QString m_statusMessage;
    QStringList m_diffTableList;
    QStringList m_allTableList;
    QVariantMap m_shipDetails;
    int m_dataVersion = 0;
    QString m_newDbFileName;

    QString m_newDbPath;
    QString m_oldDbPath;
    QString m_connNew;
    QString m_connOld;
    
    QMap<QString, int> m_newTableSizes;
    QMap<QString, int> m_oldTableSizes;

    // ── Helpers ─────────────────────────────────────
    void setComparing(bool v);
    void setStatusMessage(const QString &msg);
    void setLoaded(bool v);
    void bumpDataVersion();

    bool readDetailsFromIstow(const QString &filePath);
    QString extractDbFromIstow(const QString &filePath);
    QString toLocalPath(const QString &urlPath) const;
    QString workDir() const;
    QString detectPrimaryKey(const QString &tableName, const QString &connName);
    void closeConnections();

    // Helper: get row data from a specific DB connection
    QVariantMap fetchRow(const QString &tableName, const QVariant &pkValue,
                         const QString &connName);
};

#endif // PATCHDBMODEL_H
