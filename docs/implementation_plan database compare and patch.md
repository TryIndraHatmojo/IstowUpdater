# Compare and Patch DB Feature

Fitur ini memungkinkan user membandingkan dua file database SQLite secara langsung (tanpa melalui file `.istow`), lalu melakukan **Sync DB Structure** dan **Sync DB Data** dari New DB ke Target Patch DB.

## User Review Required

> [!IMPORTANT]
> Fitur ini **berbeda** dari PatchDbPage yang sudah ada. PatchDbPage membaca dari file `.istow`, sedangkan **CompareAndPatchDbPage** menerima 2 path file `.db` langsung via FileDialog.

> [!WARNING]
> Model baru `CompareAndPatchDbModel` akan dibuat terpisah dari `PatchDbModel` agar tidak saling mengganggu. Model ini menggabungkan logika `compareAndMigrateDb` dari `IstowImporter` (untuk Sync Structure) dan seluruh fitur data compare/patch dari `PatchDbModel` (untuk Sync Data).

## Proposed Changes

### Model C++ — Backend Logic

#### [NEW] [CompareAndPatchDbModel.h](file:///c:/Pranala/IstowUpdater/src/models/CompareAndPatchDbModel.h)

Header file untuk model C++ baru. Properties dan methods:

**Properties (Q_PROPERTY):**
- `comparing` (bool) — sedang proses compare
- `loaded` (bool) — data sudah di-load
- `statusMessage` (QString) — pesan status
- `structureLogs` (QStringList) — log dari Sync DB Structure (mirip `dbCompareLogs` di IstowImporter)
- `diffTableList` (QStringList) — tabel yang memiliki perbedaan data
- `allTableList` (QStringList) — semua tabel yang ada
- `dataVersion` (int) — version bump untuk reactive refresh
- `newDbPath` (QString) — path DB baru yang dipilih
- `targetDbPath` (QString) — path DB target yang dipilih

**Q_INVOKABLE Methods:**
- `setNewDbPath(path)` — set path DB baru dari FileDialog
- `setTargetDbPath(path)` — set path DB target dari FileDialog
- `syncStructure()` — jalankan compareAndMigrateDb (dari IstowImporter logic) + populate structureLogs
- `syncData()` — buka kedua DB, compare data, populate table list (mirip `loadAndCompare` di PatchDbModel, tapi tanpa `.istow`)
- `cleanup()` — reset state
- `getColumns(tableName)` — sama dengan PatchDbModel
- `getPrimaryKey(tableName)` — sama dengan PatchDbModel
- `getNewDbRows(tableName)` — annotated rows dengan diff info
- `getOldDbRows(tableName)` — annotated rows dengan diff info
- `getTableStats(tableName)` — statistik diff per tabel
- `getNewRowCount(tableName)` / `getOldRowCount(tableName)`
- `updateCell(tableName, pkValue, column, newValue)` — edit cell di target DB
- `replaceRowById(tableName, pkValue)` — replace 1 row
- `addRowToOldDb(tableName, pkValue)` — insert 1 row ke target
- `addAllNewRows(tableName)` — bulk insert
- `replaceAllDiffRows(tableName)` — bulk replace
- `deleteOldRowById(tableName, pkValue)` — delete 1 row
- `deleteAllOldOnlyRows(tableName)` — bulk delete
- `savePendingChanges(tableName, changes)` — batch update cells
- `copyToClipboard(text)` — clipboard helper

---

#### [NEW] [CompareAndPatchDbModel.cpp](file:///c:/Pranala/IstowUpdater/src/models/CompareAndPatchDbModel.cpp)

Implementasi model. Poin-poin kunci:

1. **`syncStructure()`** — Adaptasi dari `IstowImporter::compareAndMigrateDb()`:
   - Buka kedua DB (`newDbPath` dan `targetDbPath`)
   - Compare tables: tambah tabel baru ke target DB
   - Compare columns: `ALTER TABLE ADD COLUMN` untuk kolom baru
   - Compare triggers: tambah trigger baru
   - Compare views: tambah view baru
   - Populate `structureLogs` dengan log tiap aksi (emoji + warna di QML)
   - Append summary di akhir

2. **`syncData()`** — Adaptasi dari `PatchDbModel::loadAndCompare()`:
   - Buka kedua DB (langsung dari path, tanpa `.istow`)
   - Enumerate tables, count rows
   - Compare data row-by-row menggunakan primary key
   - Populate `diffTableList` dan `allTableList`

3. **Semua method data modification** — Identik dengan `PatchDbModel`, karena logikanya sama (operate pada 2 koneksi SQLite open).

---

### QML Pages — Frontend UI

#### [MODIFY] [CompareAndPatchDbPage.qml](file:///c:/Pranala/IstowUpdater/src/pages/CompareAndPatchDbPage.qml)

Halaman utama fitur. Layout keseluruhan:

```
┌─────────────────────────────────────────────────────┐
│ Header: "🔀 Compare and Patch DB"                   │
├─────────────────────────────────────────────────────┤
│ [Browse New DB]  [path...]                          │
│ [Browse Target]  [path...]                          │
├─────────────────────────────────────────────────────┤
│ [⚡ Sync DB Structure] [📊 Sync DB Data]            │
├─────────────────────────────────────────────────────┤
│ Structure Logs Card (mirip dbLogsCard di            │
│ BrowseDotIstow.qml — muncul setelah Sync Structure)│
├─────────────────────────────────────────────────────┤
│ Status Bar (comparing indicator)                    │
├────────┬────────────────────────────────────────────┤
│Sidebar │  Two-panel view (New DB | Target DB)       │
│Table   │  ┌──────────────┬──────────────┐           │
│List    │  │ PatchDbTable │ PatchDbTable │           │
│(Δ/All) │  │ View         │ View         │           │
│        │  │ (readonly)   │ (editable)   │           │
│        │  └──────────────┴──────────────┘           │
└────────┴────────────────────────────────────────────┘
```

Komponen detail:

1. **Header** — Warna `#1A3A5C`, judul "🔀 Compare and Patch DB", legend warna (Beda/Baru/Old Only/Pending) — sama persis dengan PatchDbPage

2. **File Picker Section** — Dua baris FileDialog input:
   - Row 1: Button "📁 Browse New DB" + path display
   - Row 2: Button "📁 Browse Target Patch DB" + path display
   - Menggunakan `FileDialog` dengan `nameFilters: ["Database Files (*.db)"]`
   - Kedua input bersifat **mandatory** — tombol aksi disabled sampai keduanya terisi

3. **Action Buttons Row**:
   - **"⚡ Sync DB Structure"** — memanggil `compareModel.syncStructure()`
   - **"📊 Sync DB Data"** — memanggil `compareModel.syncData()`
   - Disabled jika salah satu path belum dipilih
   - Tampilan: warna hijau untuk Structure, biru untuk Data

4. **Structure Logs Card** — Identik dengan `dbLogsCard` di BrowseDotIstow.qml:
   - Visible saat `compareModel.structureLogs.length > 0`
   - ListView dengan delegate berwarna (emoji-based coloring)
   - Font `Consolas`, ukuran 11
   - Warna: ❌ merah, 🆕/✅ hijau, 📎/⚡/👁 biru, 📊 ungu

5. **Loading/Comparing indicator** — BusyIndicator saat `comparing === true`

6. **Main Content (Sidebar + 2-panel)** — **100% identik** dengan PatchDbPage:
   - **Left Sidebar**: table list, filter Δ/All, dot indicator, row count badges
   - **Stats Bar**: tabel name, PK info, count badges (baru/beda/old only/match)
   - **Bulk Action Bars**: Add All, Replace All, Delete All Old Only
   - **Left Panel**: `PatchDbTableView` mode `readonly` (DB Baru)
   - **Right Panel**: `PatchDbTableView` mode `editable` (DB Target)
   - Karena kita reuse `PatchDbTableView.qml` langsung, tidak perlu buat komponen baru

7. **Welcome State** — Saat belum ada file dipilih (emoji 🔀 besar + instruksi)

---

#### Reuse: [PatchDbTableView.qml](file:///c:/Pranala/IstowUpdater/src/pages/PatchDbTableView.qml)

Tidak perlu dimodifikasi. Komponen ini sudah generic (menerima `patchModel` via property). `CompareAndPatchDbPage` akan pass `compareModel` sebagai `patchModel` karena interface-nya identik (method names sama).

---

### Build Config

#### [MODIFY] [CMakeLists.txt](file:///c:/Pranala/IstowUpdater/CMakeLists.txt)

Tambahkan file baru ke build:

```diff
 # C++ sources
+    src/models/CompareAndPatchDbModel.h
+    src/models/CompareAndPatchDbModel.cpp

 # QML files
+    src/pages/CompareAndPatchDbPage.qml
```

> [!NOTE]  
> `CompareAndPatchDbPage.qml` sudah terdaftar saat ini. Hanya perlu menambah file C++ model baru.

## Open Questions

1. **Apakah naming `CompareAndPatchDbModel` sudah OK?** Atau ada nama yang lebih disukai?
2. **Apakah Sync Structure harus dilakukan dulu sebelum Sync Data?** (saat ini plan-nya independen — user bisa klik mana saja duluan). Opsi lain: enforce urutan (Structure → Data).

## Verification Plan

### Automated Tests
- Build project dengan `cmake --build .` untuk memastikan tidak ada compile error
- Buka aplikasi, navigasi ke tab "Compare and Patch DB"
- Coba browse 2 file `.db`, klik Sync Structure dan Sync Data
- Verifikasi log panel muncul dan data tabel terloading

### Manual Verification
- Verifikasi bahwa semua fitur PatchDbPage (sidebar, table view, edit cell, replace row, add row, delete row, bulk actions, pending changes) bekerja identik di CompareAndPatchDbPage
