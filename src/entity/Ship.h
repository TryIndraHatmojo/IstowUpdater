#ifndef SHIP_H
#define SHIP_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QJsonObject>

class Ship
{
    Q_GADGET

    Q_PROPERTY(int     idship          MEMBER idship)
    Q_PROPERTY(QString tipe            MEMBER tipe)
    Q_PROPERTY(QString nama            MEMBER nama)
    Q_PROPERTY(QString company         MEMBER company)
    Q_PROPERTY(QString dbid            MEMBER dbid)
    Q_PROPERTY(QString fileprefix      MEMBER fileprefix)
    Q_PROPERTY(int     version         MEMBER version)
    Q_PROPERTY(QString novoyage        MEMBER novoyage)
    Q_PROPERTY(int     currentVoyageId MEMBER currentVoyageId)

public:
    int     idship          = -1;
    QString tipe;
    QString nama;
    QString company;
    QString dbid;
    QString fileprefix;
    int     version         =  0;
    QString novoyage;
    int     currentVoyageId = -1;

    static Ship fromVariantMap(const QVariantMap &map)
    {
        Ship s;
        s.idship          = map.value("idship",            -1).toInt();
        s.tipe            = map.value("tipe",              "").toString();
        s.nama            = map.value("nama",              "").toString();
        s.company         = map.value("company",           "").toString();
        s.dbid            = map.value("dbid",              "").toString();
        s.fileprefix      = map.value("fileprefix",        "").toString();
        s.version         = map.value("version",            0).toInt();
        s.novoyage        = map.value("novoyage",          "").toString();
        s.currentVoyageId = map.value("current_voyage_id", -1).toInt();
        return s;
    }

    static Ship fromJsonObject(const QJsonObject &obj)
    {
        return fromVariantMap(obj.toVariantMap());
    }

    QVariantMap toVariantMap() const
    {
        return {
            { "idship",            idship          },
            { "tipe",              tipe            },
            { "nama",              nama            },
            { "company",           company         },
            { "dbid",              dbid            },
            { "fileprefix",        fileprefix      },
            { "version",           version         },
            { "novoyage",          novoyage        },
            { "current_voyage_id", currentVoyageId },
        };
    }

    bool isValid() const { return idship >= 0; }
};

Q_DECLARE_METATYPE(Ship)

#endif // SHIP_H
