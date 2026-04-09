## Plan: Activity Log & CSV Export Feature

**Penyelesaian Singkat**
Kita akan membuat entitas `LogRepository` (C++) untuk mencatat operasi (Action, Status, dll) ke dalam tabel database SQLite. Selain itu, dibuat model `LogModel` untuk menyediakan riwayat ini ke antarmuka pengguna (`MainPage.qml` -> Tab Logs) sekaligus menyediakan fungsi _generate_ CSV (`.csv`) yang berfungsi layaknya dokumen Excel. Pencatatan (`LogRepository::addLog()`) akan disisipkan di saat-saat krusial dalam impor dan patch DB.

**Langkah-langkah**

1. **Implement Database & Repository**
   - Buat tabel `activity_logs` di database lokal (SQLite) pada saat inisialisasi.
   - Buat _Class_ `LogRepository` (C++) untuk mengurus method `insertLog` dan `fetchLogs`.
2. **Implement LogModel (C++)**
   - Buat kelas `LogModel` turunan dari `QAbstractTableModel`.
   - Menyediakan List API untuk UI.
   - Tambahkan _method_ Q_INVOKABLE `exportToCsv(QString filePath)` untuk menulis baris data yang ada ke dalam text file CSV (kolom _Timestamp, Action, Target/Ship, Status, Status Message_).
3. **Log Integration on Existing Features**
   - Inject/Tambahkan pemanggilan penulisan DB `LogRepository->insertLog(...)` ke dalam operasi di:
     - `IstowImporter.cpp` _("Create/Update Assets and Sync DB Structure")_ saat file di-import.
     - `PatchDbModel.cpp` _("Patch DB Data")_ saat proses patching.
     - `CompareAndPatchDbModel.cpp` _("Compare and Patch DB")_ saat proses compare dan sync berhasil/gagal.
4. **Create Logs Page UI & View**
   - Buat file `src/pages/LogsPage.qml`.
   - Implementasikan `TableView` atau kumpulan baris untuk menampilkan isi dari `LogModel`.
   - Tambahkan tombol "Export to CSV/Excel", yang jika diklik akan memunculkan _File Dialog_ lalu memanggil fungsi export di `LogModel`.
5. **Update MainPage.qml**
   - Menambahkan satu item navigasi (`TabButton` bertuliskan "Logs") pada bagian `TabBar` di `MainPage.qml`.
   - Menambahkan `LogsPage` pada bagian `StackLayout`.
6. **Register Types**
   - Masukkan `logrepository.cpp` dan `LogModel.cpp` ke dalam `CMakeLists.txt`.

**File Terkait yang akan Ditambahkan / Dimodifikasi**

- `src/repository/logrepository.h` & `.cpp` (File baru)
- `src/models/LogModel.h` & `.cpp` (File baru)
- `src/models/IstowImporter.cpp` — Registrasi pencatatan import.
- `src/models/PatchDbModel.cpp` — Registrasi pencatatan patch DB.
- `src/models/CompareAndPatchDbModel.cpp` — Registrasi pencatatan compare & patch.
- `src/pages/LogsPage.qml` (File baru) — Layout UI untuk log.
- `src/pages/MainPage.qml` — Menambahkan Tab Log.
- `CMakeLists.txt` — Update build files C++.

**Verifikasi**

1. **Pencatatan Berjalan:** Melakukan uji coba import `.istow` di masing-masing tab terkait untuk mensimulasikan pencatatan masuk ke log database.
2. **Tab Interface Real-time:** Membuka tab baru (Logs) lalu melihat apakah logs yang berhasil dan gagal dapat terlihat dengan _Header_ yang sudah disepakati (Timestamp, Action, Target/Ship, Status, Status Message).
3. **Validasi XLSX/CSV Ekspor:** Mengeklik ekspor log dan membuka file bersangkutan (`.csv`) via Microsoft Excel, pastikan semua kolom terdistribusi secara rapi.
