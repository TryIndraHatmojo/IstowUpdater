#ifndef BACKUPREPOSITORY_H
#define BACKUPREPOSITORY_H

#include <QObject>
#include <QJsonArray>
#include "repository.h"

class BackupRepository : public Repository
{
public:
    BackupRepository(QString pathdb, QString conndb);
    static BackupRepository *getInstance();

    void init();

    // ── Backup Sessions ──────────────────────────────
    int insertSession(const QString &folderName, const QString &shipName, int idship);
    void updateSessionStatus(int sessionId, const QString &status);
    QJsonArray getAllSessions();
    QJsonObject getSessionByFolder(const QString &folderName);
    QJsonArray getSessionsByShip(int idship);

    // ── Backup Records ──────────────────────────────
    void insertRecord(int sessionId, const QString &filePath, const QString &backupPath,
                      const QString &action);
    QJsonArray getAllRecords();
    QJsonArray getRecordsBySession(int sessionId);

private:
    static BackupRepository *backupRepository;
};

#endif // BACKUPREPOSITORY_H
