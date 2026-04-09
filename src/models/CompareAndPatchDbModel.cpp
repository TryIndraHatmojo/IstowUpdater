#include "CompareAndPatchDbModel.h"
#include "../repository/logrepository.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
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

CompareAndPatchDbModel::CompareAndPatchDbModel(QObject *parent)
    : QObject(parent)
    , m_connNew("cpatch_new_" + QString::number(quintptr(this)))
    , m_connOld("cpatch_old_" + QString::number(quintptr(this)))
{
}

CompareAndPatchDbModel::~CompareAndPatchDbModel()
{
    cleanup();
}

// ════════════════════════════════════════════════════════════
// Property Accessors
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::comparing() const { return m_comparing; }
bool CompareAndPatchDbModel::loaded() const { return m_loaded; }
bool CompareAndPatchDbModel::structureSynced() const { return m_structureSynced; }
bool CompareAndPatchDbModel::structurePreviewed() const { return m_structurePreviewed; }
QString CompareAndPatchDbModel::statusMessage() const { return m_statusMessage; }
QString CompareAndPatchDbModel::newDbPath() const { return m_newDbPath; }
QString CompareAndPatchDbModel::targetDbPath() const { return m_targetDbPath; }
QStringList CompareAndPatchDbModel::structureLogs() const { return m_structureLogs; }
QStringList CompareAndPatchDbModel::diffTableList() const { return m_diffTableList; }
QStringList CompareAndPatchDbModel::allTableList() const { return m_allTableList; }
int CompareAndPatchDbModel::dataVersion() const { return m_dataVersion; }

// ════════════════════════════════════════════════════════════
// Property Setters (private)
// ════════════════════════════════════════════════════════════

void CompareAndPatchDbModel::setComparing(bool v) {
    if (m_comparing != v) { m_comparing = v; emit comparingChanged(); }
}

void CompareAndPatchDbModel::setStatusMessage(const QString &msg) {
    m_statusMessage = msg;
    emit statusMessageChanged();
    qDebug() << "[CompareAndPatchDbModel]" << msg;
}

void CompareAndPatchDbModel::setLoaded(bool v) {
    if (m_loaded != v) { m_loaded = v; emit loadedChanged(); }
}

void CompareAndPatchDbModel::setStructureSynced(bool v) {
    if (m_structureSynced != v) { m_structureSynced = v; emit structureSyncedChanged(); }
}

void CompareAndPatchDbModel::setStructurePreviewed(bool v) {
    if (m_structurePreviewed != v) { m_structurePreviewed = v; emit structurePreviewedChanged(); }
}

void CompareAndPatchDbModel::bumpDataVersion() {
    m_dataVersion++;
    emit dataVersionChanged();
}

// ════════════════════════════════════════════════════════════
// Helpers
// ════════════════════════════════════════════════════════════

QString CompareAndPatchDbModel::toLocalPath(const QString &urlPath) const {
    QUrl url(urlPath);
    return url.isLocalFile() ? url.toLocalFile() : urlPath;
}

void CompareAndPatchDbModel::closeConnections() {
    {
        if (QSqlDatabase::contains(m_connNew))
            QSqlDatabase::database(m_connNew).close();
        if (QSqlDatabase::contains(m_connOld))
            QSqlDatabase::database(m_connOld).close();
    }
    QSqlDatabase::removeDatabase(m_connNew);
    QSqlDatabase::removeDatabase(m_connOld);
}

void CompareAndPatchDbModel::appendStructureLog(const QString &msg) {
    m_structureLogs.append(msg);
    emit structureLogsChanged();
    qDebug() << "[CompareAndPatchDbModel StructLog]" << msg;
}

void CompareAndPatchDbModel::cleanup() {
    closeConnections();
    setLoaded(false);
    setStructureSynced(false);
    setStructurePreviewed(false);
    m_diffTableList.clear();
    m_allTableList.clear();
    m_structureLogs.clear();
    m_newTableSizes.clear();
    m_oldTableSizes.clear();
    emit diffTableListChanged();
    emit allTableListChanged();
    emit structureLogsChanged();
}

void CompareAndPatchDbModel::copyToClipboard(const QString &text) {
    QGuiApplication::clipboard()->setText(text);
}

// ════════════════════════════════════════════════════════════
// Path Setters
// ════════════════════════════════════════════════════════════

void CompareAndPatchDbModel::setNewDbPath(const QString &path) {
    QString localPath = toLocalPath(path);
    if (m_newDbPath != localPath) {
        m_newDbPath = localPath;
        emit newDbPathChanged();
        // Reset state ketika path berubah
        cleanup();
        setStatusMessage("New DB: " + localPath);
    }
}

void CompareAndPatchDbModel::setTargetDbPath(const QString &path) {
    QString localPath = toLocalPath(path);
    if (m_targetDbPath != localPath) {
        m_targetDbPath = localPath;
        emit targetDbPathChanged();
        // Reset state ketika path berubah
        cleanup();
        setStatusMessage("Target DB: " + localPath);
    }
}

// ════════════════════════════════════════════════════════════
// Primary Key Detection
// ════════════════════════════════════════════════════════════

QString CompareAndPatchDbModel::detectPrimaryKey(const QString &tableName, const QString &connName) {
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
    return pkCols.first();
}

// ════════════════════════════════════════════════════════════
// previewStructure — Preview schema differences (no apply)
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::previewStructure() {
    m_structureLogs.clear();
    emit structureLogsChanged();
    setStructurePreviewed(false);

    if (m_newDbPath.isEmpty() || m_targetDbPath.isEmpty()) {
        appendStructureLog("❌ ERROR: Kedua path DB harus dipilih terlebih dahulu");
        return false;
    }

    if (!QFile::exists(m_newDbPath)) {
        appendStructureLog("❌ ERROR: File New DB tidak ditemukan: " + m_newDbPath);
        return false;
    }

    if (!QFile::exists(m_targetDbPath)) {
        appendStructureLog("❌ ERROR: File Target DB tidak ditemukan: " + m_targetDbPath);
        return false;
    }

    // Close any previous connections
    closeConnections();

    appendStructureLog("📂 New DB: " + m_newDbPath);
    appendStructureLog("📂 Target DB: " + m_targetDbPath);
    appendStructureLog("");

    // Open both databases
    {
        QSqlDatabase dbNew = QSqlDatabase::addDatabase("QSQLITE", m_connNew);
        dbNew.setDatabaseName(m_newDbPath);
        if (!dbNew.open()) {
            appendStructureLog("❌ GAGAL membuka New DB: " + dbNew.lastError().text());
            return false;
        }

        QSqlDatabase dbOld = QSqlDatabase::addDatabase("QSQLITE", m_connOld);
        dbOld.setDatabaseName(m_targetDbPath);
        if (!dbOld.open()) {
            appendStructureLog("❌ GAGAL membuka Target DB: " + dbOld.lastError().text());
            QSqlDatabase::database(m_connNew).close();
            QSqlDatabase::removeDatabase(m_connNew);
            return false;
        }

        appendStructureLog("🔍 Memulai perbandingan schema (PREVIEW)...");
        appendStructureLog("═══════════════════════════════════════════");

        int tablesNew = 0;
        int columnsNew = 0;
        int triggersNew = 0;
        int viewsNew = 0;

        // ═══════════════════════════════════════════
        // 1. COMPARE TABLES
        // ═══════════════════════════════════════════

        QSet<QString> oldTables;
        {
            QSqlQuery q(dbOld);
            q.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
            while (q.next()) {
                oldTables.insert(q.value(0).toString());
            }
        }

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
            if (!oldTables.contains(tableName)) {
                appendStructureLog("🆕 [PREVIEW] Tabel baru akan ditambahkan: " + tableName);
                tablesNew++;
            }
        }

        // ═══════════════════════════════════════════
        // 2. COMPARE COLUMNS
        // ═══════════════════════════════════════════

        for (const QString &tableName : oldTables) {
            if (!newTablesDDL.contains(tableName)) continue;

            QSet<QString> oldColumns;
            {
                QSqlQuery q(dbOld);
                q.exec(QString("PRAGMA table_info('%1')").arg(tableName));
                while (q.next()) {
                    oldColumns.insert(q.value(1).toString().toLower());
                }
            }

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
                    appendStructureLog("📎 [PREVIEW] Kolom baru: " + tableName + "." + col.name + " (" + col.type + ")");
                    columnsNew++;
                }
            }
        }

        // ═══════════════════════════════════════════
        // 3. COMPARE TRIGGERS
        // ═══════════════════════════════════════════

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
                appendStructureLog("⚡ [PREVIEW] Trigger baru: " + it.key());
                triggersNew++;
            }
        }

        // ═══════════════════════════════════════════
        // 4. COMPARE VIEWS
        // ═══════════════════════════════════════════

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

        for (auto it = newViewsDDL.constBegin(); it != newViewsDDL.constEnd(); ++it) {
            if (!oldViews.contains(it.key())) {
                appendStructureLog("👁 [PREVIEW] View baru: " + it.key());
                viewsNew++;
            }
        }

        // ═══════════════════════════════════════════
        // Summary
        // ═══════════════════════════════════════════

        appendStructureLog("═══════════════════════════════════════════");
        appendStructureLog(QString("📊 Preview Ringkasan: %1 tabel, %2 kolom, %3 trigger, %4 view akan ditambahkan")
                               .arg(tablesNew).arg(columnsNew).arg(triggersNew).arg(viewsNew));

        if (tablesNew == 0 && columnsNew == 0 && triggersNew == 0 && viewsNew == 0) {
            appendStructureLog("✅ Schema sudah identik — tidak ada perubahan yang diperlukan");
        } else {
            appendStructureLog("ℹ️ Klik 'Apply Sync Structure' untuk menerapkan perubahan di atas.");
        }

        // Close connections after preview (will re-open on apply)
        dbOld.close();
        dbNew.close();
    }

    QSqlDatabase::removeDatabase(m_connNew);
    QSqlDatabase::removeDatabase(m_connOld);

    setStructurePreviewed(true);
    setStatusMessage("✅ Preview structure selesai");
    return true;
}

// ════════════════════════════════════════════════════════════
// applySyncStructure — Apply schema sync after preview
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::applySyncStructure() {
    if (!m_structurePreviewed) {
        setStatusMessage("❌ Lakukan Preview Structure terlebih dahulu");
        return false;
    }

    if (m_newDbPath.isEmpty() || m_targetDbPath.isEmpty()) {
        setStatusMessage("❌ Kedua path DB harus dipilih");
        return false;
    }

    // Clear and re-populate logs for apply phase
    m_structureLogs.clear();
    emit structureLogsChanged();

    appendStructureLog("📂 New DB: " + m_newDbPath);
    appendStructureLog("📂 Target DB: " + m_targetDbPath);
    appendStructureLog("");

    // Close previous connections
    closeConnections();

    {
        QSqlDatabase dbNew = QSqlDatabase::addDatabase("QSQLITE", m_connNew);
        dbNew.setDatabaseName(m_newDbPath);
        if (!dbNew.open()) {
            appendStructureLog("❌ GAGAL membuka New DB: " + dbNew.lastError().text());
            return false;
        }

        QSqlDatabase dbOld = QSqlDatabase::addDatabase("QSQLITE", m_connOld);
        dbOld.setDatabaseName(m_targetDbPath);
        if (!dbOld.open()) {
            appendStructureLog("❌ GAGAL membuka Target DB: " + dbOld.lastError().text());
            QSqlDatabase::database(m_connNew).close();
            QSqlDatabase::removeDatabase(m_connNew);
            return false;
        }

        appendStructureLog("🔧 Menerapkan perubahan schema...");
        appendStructureLog("═══════════════════════════════════════════");

        int tablesAdded = 0;
        int columnsAdded = 0;
        int triggersAdded = 0;
        int viewsAdded = 0;

        // ═══════════════════════════════════════════
        // 1. ADD NEW TABLES
        // ═══════════════════════════════════════════

        QSet<QString> oldTables;
        {
            QSqlQuery q(dbOld);
            q.exec("SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'");
            while (q.next()) {
                oldTables.insert(q.value(0).toString());
            }
        }

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
                appendStructureLog("🆕 Tambah tabel: " + tableName);
                QSqlQuery q(dbOld);
                if (q.exec(createSql)) {
                    appendStructureLog("   ✅ Berhasil dibuat");
                    tablesAdded++;
                } else {
                    appendStructureLog("   ❌ Gagal: " + q.lastError().text());
                }
            }
        }

        // ═══════════════════════════════════════════
        // 2. ADD NEW COLUMNS
        // ═══════════════════════════════════════════

        for (const QString &tableName : oldTables) {
            if (!newTablesDDL.contains(tableName)) continue;

            QSet<QString> oldColumns;
            {
                QSqlQuery q(dbOld);
                q.exec(QString("PRAGMA table_info('%1')").arg(tableName));
                while (q.next()) {
                    oldColumns.insert(q.value(1).toString().toLower());
                }
            }

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
                    QString alterSql = QString("ALTER TABLE \"%1\" ADD COLUMN \"%2\" %3")
                                          .arg(tableName, col.name, col.type);
                    if (!col.dfltValue.isEmpty()) {
                        alterSql += " DEFAULT " + col.dfltValue;
                    }

                    appendStructureLog("📎 Tambah kolom: " + tableName + "." + col.name + " (" + col.type + ")");
                    QSqlQuery q(dbOld);
                    if (q.exec(alterSql)) {
                        appendStructureLog("   ✅ Berhasil");
                        columnsAdded++;
                    } else {
                        appendStructureLog("   ❌ Gagal: " + q.lastError().text());
                    }
                }
            }
        }

        // ═══════════════════════════════════════════
        // 3. ADD NEW TRIGGERS
        // ═══════════════════════════════════════════

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
                appendStructureLog("⚡ Tambah trigger: " + it.key());
                QSqlQuery q(dbOld);
                if (q.exec(it.value())) {
                    appendStructureLog("   ✅ Berhasil");
                    triggersAdded++;
                } else {
                    appendStructureLog("   ❌ Gagal: " + q.lastError().text());
                }
            }
        }

        // ═══════════════════════════════════════════
        // 4. ADD NEW VIEWS
        // ═══════════════════════════════════════════

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

        for (auto it = newViewsDDL.constBegin(); it != newViewsDDL.constEnd(); ++it) {
            if (!oldViews.contains(it.key())) {
                appendStructureLog("👁 Tambah view: " + it.key());
                QSqlQuery q(dbOld);
                if (q.exec(it.value())) {
                    appendStructureLog("   ✅ Berhasil");
                    viewsAdded++;
                } else {
                    appendStructureLog("   ❌ Gagal: " + q.lastError().text());
                }
            }
        }

        // ═══════════════════════════════════════════
        // Summary
        // ═══════════════════════════════════════════

        appendStructureLog("═══════════════════════════════════════════");
        appendStructureLog(QString("📊 Ringkasan: %1 tabel, %2 kolom, %3 trigger, %4 view ditambahkan")
                               .arg(tablesAdded).arg(columnsAdded).arg(triggersAdded).arg(viewsAdded));

        if (tablesAdded == 0 && columnsAdded == 0 && triggersAdded == 0 && viewsAdded == 0) {
            appendStructureLog("✅ Schema DB sudah identik — tidak ada perubahan");
        }

        // Close connections
        dbOld.close();
        dbNew.close();
    }

    QSqlDatabase::removeDatabase(m_connNew);
    QSqlDatabase::removeDatabase(m_connOld);

    setStructureSynced(true);
    setStatusMessage("✅ Sync Structure selesai. Anda sekarang dapat melakukan Sync Data.");
    return true;
}

// ════════════════════════════════════════════════════════════
// syncData — Compare data (requires structureSynced)
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::syncData() {
    if (!m_structureSynced) {
        setStatusMessage("❌ Lakukan Sync Structure terlebih dahulu sebelum Sync Data");
        return false;
    }

    if (m_newDbPath.isEmpty() || m_targetDbPath.isEmpty()) {
        setStatusMessage("❌ Kedua path DB harus dipilih");
        return false;
    }

    setComparing(true);
    setStatusMessage("Memulai perbandingan data...");

    // Close previous connections
    closeConnections();

    // Open both databases
    {
        QSqlDatabase dbNew = QSqlDatabase::addDatabase("QSQLITE", m_connNew);
        dbNew.setDatabaseName(m_newDbPath);
        if (!dbNew.open()) {
            setStatusMessage("❌ Gagal buka New DB: " + dbNew.lastError().text());
            setComparing(false);
            return false;
        }

        QSqlDatabase dbOld = QSqlDatabase::addDatabase("QSQLITE", m_connOld);
        dbOld.setDatabaseName(m_targetDbPath);
        if (!dbOld.open()) {
            setStatusMessage("❌ Gagal buka Target DB: " + dbOld.lastError().text());
            setComparing(false);
            return false;
        }
    }

    // Get table lists
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

    QSet<QString> allTablesSet = newTables | oldTables;

    QStringList initialTableList = allTablesSet.values();
    initialTableList.sort();

    // Compare data per table
    QStringList diffTables;
    QStringList finalAllTables;

    for (const QString &table : std::as_const(initialTableList)) {
        QSqlDatabase dbNew = QSqlDatabase::database(m_connNew);
        QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);

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

        // Skip empty tables from both sides
        if (newCount == 0 && oldCount == 0) continue;

        finalAllTables.append(table);

        // Table only in one side = diff
        if (!newTables.contains(table) || !oldTables.contains(table)) {
            diffTables.append(table);
            continue;
        }

        QString pk = detectPrimaryKey(table, m_connNew);

        // Build row maps for comparison
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

        // Compare
        bool hasDiff = false;

        for (auto it = newRows.constBegin(); it != newRows.constEnd(); ++it) {
            if (!oldRows.contains(it.key())) { hasDiff = true; break; }
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
            for (auto it = oldRows.constBegin(); it != oldRows.constEnd(); ++it) {
                if (!newRows.contains(it.key())) { hasDiff = true; break; }
            }
        }

        if (hasDiff) diffTables.append(table);
    }

    diffTables.sort();
    m_diffTableList = diffTables;
    m_allTableList = finalAllTables;

    QStringList compareLogs;
    compareLogs << "==== [DATABASE DATA COMPARE LOGS] ====";
    compareLogs << QString("Total Tabel Dievaluasi : %1").arg(m_allTableList.size());
    compareLogs << QString("Total Tabel Berbeda    : %1\n").arg(m_diffTableList.size());

    if (!m_diffTableList.isEmpty()) {
        compareLogs << "Rincian Tabel yang Memiliki Perbedaan Data:";
        for (const QString &table : m_diffTableList) {
            int nCount = m_newTableSizes.value(table, 0);
            int oCount = m_oldTableSizes.value(table, 0);
            compareLogs << QString("  - %1 (New DB: %2 baris | Target DB: %3 baris)").arg(table).arg(nCount).arg(oCount);
        }
    } else {
        compareLogs << "Kedua database identik (tidak ada perbedaan data).";
    }

    emit allTableListChanged();
    emit diffTableListChanged();

    QString targetShip = QFileInfo(m_targetDbPath).fileName();
    QString logPathData = LogRepository::getInstance()->saveLogToFile("CompareDB_Data", targetShip, compareLogs);

    QString summaryMsg = QString("Compare data selesai: Ditemukan perbedaan pada %1 dari total %2 tabel.").arg(m_diffTableList.size()).arg(m_allTableList.size());
    setStatusMessage(summaryMsg);
    
    // Log result
    QString logPathStruct = LogRepository::getInstance()->saveLogToFile("CompareDB_Structure", targetShip, m_structureLogs);
    LogRepository::getInstance()->insertLog("Compare DB (Sync Data)", targetShip, "Success", summaryMsg, logPathData);

    setComparing(false);
    setLoaded(true);
    return true;
}

// ════════════════════════════════════════════════════════════
// Data Accessors
// ════════════════════════════════════════════════════════════

QVariantList CompareAndPatchDbModel::getColumns(const QString &tableName) {
    QVariantList result;
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

QString CompareAndPatchDbModel::getPrimaryKey(const QString &tableName) {
    QString conn = QSqlDatabase::contains(m_connNew) ? m_connNew : m_connOld;
    return detectPrimaryKey(tableName, conn);
}

// ════════════════════════════════════════════════════════════
// fetchRow helper
// ════════════════════════════════════════════════════════════

QVariantMap CompareAndPatchDbModel::fetchRow(const QString &tableName, const QVariant &pkValue,
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
// fetchAllRows (static helper)
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

// ════════════════════════════════════════════════════════════
// getNewDbRows / getOldDbRows — with diff annotations
// ════════════════════════════════════════════════════════════

QVariantList CompareAndPatchDbModel::getNewDbRows(const QString &tableName) {
    QString pk = getPrimaryKey(tableName);
    QVariantList newRows = fetchAllRows(tableName, pk, m_connNew);
    QVariantList oldRows = fetchAllRows(tableName, pk, m_connOld);

    auto oldMap = indexByPk(oldRows);
    QVariantList filteredRows;

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

QVariantList CompareAndPatchDbModel::getOldDbRows(const QString &tableName) {
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

QVariantMap CompareAndPatchDbModel::getTableStats(const QString &tableName) {
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

int CompareAndPatchDbModel::getNewRowCount(const QString &tableName) {
    return m_newTableSizes.value(tableName, 0);
}

int CompareAndPatchDbModel::getOldRowCount(const QString &tableName) {
    return m_oldTableSizes.value(tableName, 0);
}

// ════════════════════════════════════════════════════════════
// updateCell
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::updateCell(const QString &tableName, const QVariant &pkValue,
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
        QString err = q.lastError().text();
        setStatusMessage("❌ Update gagal: " + err);
        LogRepository::getInstance()->insertLog("Compare DB (Cell Update)", "Custom Patch", "Failed", "PK: " + pkValue.toString() + " - " + err);
        return false;
    }

    QString msg = QString("Cell %1 updated in %2 for %3=%4").arg(column).arg(tableName).arg(pk).arg(pkValue.toString());
    LogRepository::getInstance()->insertLog("Compare DB (Cell Update)", "Custom Patch", "Success", msg);

    bumpDataVersion();
    return true;
}

// ════════════════════════════════════════════════════════════
// replaceRowById
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::replaceRowById(const QString &tableName, const QVariant &pkValue) {
    if (!QSqlDatabase::contains(m_connNew) || !QSqlDatabase::contains(m_connOld))
        return false;

    QString pk = detectPrimaryKey(tableName, m_connNew);

    QVariantMap newRow = fetchRow(tableName, pkValue, m_connNew);
    if (newRow.isEmpty()) {
        setStatusMessage("❌ Row tidak ditemukan di New DB (PK: " + pkValue.toString() + ")");
        return false;
    }

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
            QString err = q.lastError().text();
            setStatusMessage("❌ Replace gagal: " + err);
            LogRepository::getInstance()->insertLog("Compare DB (Replace Row)", "Custom Patch", "Failed", "PK: " + pkValue.toString() + " - " + err);
            return false;
        }
    } else {
        return addRowToOldDb(tableName, pkValue);
    }

    QString msg = QString("Row replaced in %1 for %2=%3").arg(tableName).arg(pk).arg(pkValue.toString());
    LogRepository::getInstance()->insertLog("Compare DB (Replace Row)", "Custom Patch", "Success", msg);

    bumpDataVersion();
    setStatusMessage("✅ Row berhasil di-replace (PK: " + pkValue.toString() + ")");
    return true;
}

// ════════════════════════════════════════════════════════════
// addRowToOldDb
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::addRowToOldDb(const QString &tableName, const QVariant &pkValue) {
    if (!QSqlDatabase::contains(m_connNew) || !QSqlDatabase::contains(m_connOld))
        return false;

    QVariantMap newRow = fetchRow(tableName, pkValue, m_connNew);
    if (newRow.isEmpty()) {
        setStatusMessage("❌ Row tidak ditemukan di New DB");
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
        QString err = q.lastError().text();
        setStatusMessage("❌ Add row gagal: " + err);
        LogRepository::getInstance()->insertLog("Compare DB (Add Row)", "Custom Patch", "Failed", "PK: " + pkValue.toString() + " - " + err);
        return false;
    }

    QString pk = detectPrimaryKey(tableName, m_connNew);
    QString msg = QString("Row added to %1 for %2=%3").arg(tableName).arg(pk).arg(pkValue.toString());
    LogRepository::getInstance()->insertLog("Compare DB (Add Row)", "Custom Patch", "Success", msg);

    bumpDataVersion();
    setStatusMessage("✅ Row berhasil ditambahkan (PK: " + pkValue.toString() + ")");
    return true;
}

// ════════════════════════════════════════════════════════════
// addAllNewRows
// ════════════════════════════════════════════════════════════

bool CompareAndPatchDbModel::addAllNewRows(const QString &tableName) {
    if (!QSqlDatabase::contains(m_connOld) || !QSqlDatabase::contains(m_connNew)) return false;

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);
    dbOld.transaction();

    QVariantList newRows = getNewDbRows(tableName);
    int successCount = 0;
    QStringList logs;
    logs << QString("==== [ADD ALL LOGS for %1] ====").arg(tableName);

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
                    logs << QString("Added Row PK: %1").arg(pkValue.toString());
                }
            }
        }
    }

    dbOld.commit();
    if (successCount > 0) {
        m_oldTableSizes[tableName] += successCount;
        bumpDataVersion();
        QString ship = QFileInfo(m_targetDbPath).fileName();
        
        logs.prepend(QString("Total rows added: %1").arg(successCount));
        QString logPath = LogRepository::getInstance()->saveLogToFile("ComparePatch_AddAll", ship, logs);
        
        LogRepository::getInstance()->insertLog("Compare & Patch DB (Add All)", ship, "Success", QString("%1 rows added to %2").arg(successCount).arg(tableName), logPath);
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

bool CompareAndPatchDbModel::replaceAllDiffRows(const QString &tableName) {
    if (!QSqlDatabase::contains(m_connOld) || !QSqlDatabase::contains(m_connNew)) return false;

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);
    dbOld.transaction();

    QString pk = detectPrimaryKey(tableName, m_connNew);
    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);

    QVariantList newRows = getNewDbRows(tableName);
    int successCount = 0;
    QStringList logs;
    logs << QString("==== [REPLACE ALL LOGS for %1] ====").arg(tableName);

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
                    logs << QString("Replaced Row PK: %1").arg(pkValue.toString());
                }
            }
        }
    }

    dbOld.commit();
    if (successCount > 0) {
        bumpDataVersion();
        QString ship = QFileInfo(m_targetDbPath).fileName();
        
        logs.prepend(QString("Total rows replaced: %1").arg(successCount));
        QString logPath = LogRepository::getInstance()->saveLogToFile("ComparePatch_ReplaceAll", ship, logs);
        
        LogRepository::getInstance()->insertLog("Compare & Patch DB (Replace All)", ship, "Success", QString("%1 rows replaced in %2").arg(successCount).arg(tableName), logPath);
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

bool CompareAndPatchDbModel::deleteOldRowById(const QString &tableName, const QVariant &pkValue) {
    if (!QSqlDatabase::contains(m_connOld)) return false;

    QString pk = detectPrimaryKey(tableName, m_connOld);
    QSqlDatabase db = QSqlDatabase::database(m_connOld);
    QSqlQuery q(db);

    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);
    q.prepare(QString("DELETE FROM \"%1\" WHERE %2 = :pk").arg(tableName, pkCol));
    q.bindValue(":pk", pkValue);

    if (!q.exec()) {
        QString err = q.lastError().text();
        setStatusMessage("❌ Delete row gagal: " + err);
        LogRepository::getInstance()->insertLog("Compare DB (Delete Row)", "Custom Patch", "Failed", "PK: " + pkValue.toString() + " - " + err);
        return false;
    }

    QString msg = QString("Row deleted from %1 where %2=%3").arg(tableName).arg(pk).arg(pkValue.toString());
    LogRepository::getInstance()->insertLog("Compare DB (Delete Row)", "Custom Patch", "Success", msg);

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

bool CompareAndPatchDbModel::deleteAllOldOnlyRows(const QString &tableName) {
    if (!QSqlDatabase::contains(m_connOld)) return false;

    QSqlDatabase dbOld = QSqlDatabase::database(m_connOld);
    dbOld.transaction();

    QString pk = detectPrimaryKey(tableName, m_connOld);
    QString pkCol = (pk == "rowid") ? "rowid" : QString("\"%1\"").arg(pk);

    QVariantList oldRows = getOldDbRows(tableName);
    int successCount = 0;
    QStringList logs;
    logs << QString("==== [DELETE ALL LOGS for %1] ====").arg(tableName);

    QSqlQuery q(dbOld);
    q.prepare(QString("DELETE FROM \"%1\" WHERE %2 = :pk").arg(tableName, pkCol));

    for (const QVariant &v : oldRows) {
        QVariantMap row = v.toMap();
        if (row.value("_status").toString() == "old_only") {
            q.bindValue(":pk", row.value("_pk"));
            if (q.exec()) {
                successCount++;
                logs << QString("Deleted Row PK: %1").arg(row.value("_pk").toString());
            }
        }
    }

    dbOld.commit();
    if (successCount > 0) {
        if (m_oldTableSizes.contains(tableName)) {
            m_oldTableSizes[tableName] -= successCount;
        }
        bumpDataVersion();
        QString ship = QFileInfo(m_targetDbPath).fileName();
        
        logs.prepend(QString("Total rows deleted: %1").arg(successCount));
        QString logPath = LogRepository::getInstance()->saveLogToFile("ComparePatch_DeleteAll", ship, logs);
        
        LogRepository::getInstance()->insertLog("Compare & Patch DB (Delete All Old)", ship, "Success", QString("%1 rows deleted from %2").arg(successCount).arg(tableName), logPath);
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

bool CompareAndPatchDbModel::savePendingChanges(const QString &tableName, const QVariantList &changes) {
    if (!QSqlDatabase::contains(m_connOld)) return false;

    int ok = 0, fail = 0;
    QStringList logs;
    logs << QString("==== [CELL UPDATE LOGS for %1] ====").arg(tableName);
    
    for (const QVariant &item : changes) {
        QVariantMap change = item.toMap();
        QVariant pk = change.value("pk");
        QString col = change.value("column").toString();
        QVariant val = change.value("value");

        if (updateCell(tableName, pk, col, val)) {
            ok++;
            logs << QString("Patched cell OK -> PK: %1, Column: %2").arg(pk.toString(), col);
        } else {
            fail++;
            logs << QString("Patched cell FAIL -> PK: %1, Column: %2").arg(pk.toString(), col);
        }
    }

    QString msg = QString("💾 Simpan selesai: %1 berhasil, %2 gagal di %3").arg(ok).arg(fail).arg(tableName);
    setStatusMessage(msg);
    if(ok > 0) {
        QString ship = QFileInfo(m_targetDbPath).fileName();
        logs.prepend(QString("Total patched: %1").arg(ok));
        QString logPath = LogRepository::getInstance()->saveLogToFile("ComparePatch_CellUpdate", ship, logs);
        
        LogRepository::getInstance()->insertLog("Compare & Patch DB (Cell Update)", ship, "Success", QString("%1 cells patched").arg(ok), logPath);
    }
    bumpDataVersion();
    return fail == 0;
}
