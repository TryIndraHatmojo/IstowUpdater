# Dokumentasi Database BackupRepository

Dokumen ini menjelaskan database SQLite yang dipakai oleh BackupRepository, termasuk lokasi file database, struktur tabel, relasi, dan fungsi akses datanya.

## Ringkasan

- Engine: SQLite
- Nama koneksi Qt: BACKUPDB
- Nama file database: backup_records.sqlite
- Lokasi database saat runtime:
  - Home user + /iStowV2/IstowUpdater/
  - Contoh pola path: <home>/iStowV2/IstowUpdater/backup_records.sqlite
- Inisialisasi database dilakukan di fungsi getInstance dan init pada BackupRepository.

Referensi implementasi:

- src/repository/backuprepository.h
- src/repository/backuprepository.cpp

## Tabel 1: backup_sessions

Tabel ini menyimpan informasi sesi backup (satu folder backup per sesi).

Skema SQL:

```sql
CREATE TABLE IF NOT EXISTS backup_sessions(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  folder_name TEXT UNIQUE,
  ship_name TEXT,
  idship INT,
  status TEXT DEFAULT 'completed',
  created_at INTEGER DEFAULT (strftime('%s','now'))
);
```

Keterangan kolom:

- id: primary key autoincrement.
- folder_name: nama folder backup, harus unik.
- ship_name: nama kapal yang terkait sesi backup.
- idship: id kapal.
- status: status sesi backup, default completed.
- created_at: timestamp Unix (detik) saat data dibuat.

Operasi yang tersedia:

- insertSession(folderName, shipName, idship)
- updateSessionStatus(sessionId, status)
- getAllSessions()
- getSessionByFolder(folderName)
- getSessionsByShip(idship)

## Tabel 2: backup_records

Tabel ini menyimpan detail file yang dibackup per sesi.

Skema SQL:

```sql
CREATE TABLE IF NOT EXISTS backup_records(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  session_id INT,
  file_path TEXT,
  backup_path TEXT,
  action TEXT,
  created_at INTEGER DEFAULT (strftime('%s','now')),
  FOREIGN KEY(session_id) REFERENCES backup_sessions(id)
);
```

Keterangan kolom:

- id: primary key autoincrement.
- session_id: relasi ke backup_sessions.id.
- file_path: path file sumber.
- backup_path: path file hasil backup.
- action: aksi yang dilakukan pada file (misalnya copy, update, rollback, sesuai implementasi pemanggil).
- created_at: timestamp Unix (detik) saat record dibuat.

Operasi yang tersedia:

- insertRecord(sessionId, filePath, backupPath, action)
- getAllRecords()
- getRecordsBySession(sessionId)

## Relasi Data

- Satu baris di backup_sessions dapat memiliki banyak baris di backup_records.
- Relasi: backup_records.session_id -> backup_sessions.id.

Diagram relasi sederhana:

```text
backup_sessions (1) ---- (N) backup_records
```

## Urutan Alur Umum

1. Panggil insertSession untuk membuat sesi backup baru.
2. Simpan id sesi dari hasil insert.
3. Untuk setiap file, panggil insertRecord dengan session_id yang sama.
4. Opsional, update status sesi menggunakan updateSessionStatus.

## Catatan Implementasi

- Query dibangun dengan QString::arg dan string interpolation SQL.
- created_at disimpan sebagai Unix timestamp dalam detik.
- folder_name dibatasi UNIQUE untuk mencegah duplikasi nama folder sesi.
