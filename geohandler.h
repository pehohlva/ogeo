//
// GeoIp handler database build a sqlite3 database 
// Author: Peter Hohl <pehohlva@gmail.com>,    1.12.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution


#ifndef GEOHANDLER_H
#define GEOHANDLER_H

//// endresult sqlite3   /Users/dev/.GeoIp/geoipDB.db3
//// http://www.splitbrain.org/blog/2011-02/12-maxmind_geoip_db_and_sqlite
/// http://dev.maxmind.com/geoip/legacy/geolite/
/// download cvs format from http://dev.maxmind.com/geoip/legacy/geolite/ and build sqlite3 db & table
/// unicode table number http://unicode-table.com/en/

#include <QCoreApplication>
#include <QFile>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QDate>
#include <QDateTime>
#include <QFileInfo>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QTextStream>
#include <QMetaType>
#include <QPair>
#include <QtCore>
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QHostAddress>

#include "sqlite3/qt_sqlite3.h"
#include "decompress.h"



#if 1 //// 1 or 0 
#define SQLBEEP qDebug
#else
#define SQLBEEP if (0) qDebug
#endif

const int pointo = 76;

#define __CARET44SP_ \
              QString("##44##")

#define __DOCVERSION__ \
              QString("Ve.0.2.3")

#define __APPNAME__ \
              QString("GeoIp Handler")
//// endresult sqlite3   /Users/dev/.GeoIp/geoipDB.db3
#define GEOIPCACHE \
             QString("%1/.GeoIp/").arg(QDir::homePath())

#define WORKSQLITE3 \
              QString("geoipDB.db3")

#if 1 //// 1 or 0 
#define GEOBEEB qDebug
#else
#define GEOBEEB if (0) qDebug
#endif




class GeoHandler : public QObject
{
    Q_OBJECT
public:
    GeoHandler(QObject *parent = 0 , int modus = 1 ); /// modus 1 = downlaod
    void setIp( const QString ip ) {
        ipadress = ip;
    }
signals:
     /// quit();
public slots:
    void execute();
    void downloadProgress(qint64 r, qint64 tot);
    void doDownload(const QUrl &url);
    void downloadFinished(QNetworkReply *reply);

private:
        int actionmake;
        bool saveToDisk(const QString &filename, QIODevice *data);
        QString saveFileName(const QUrl &url );
        bool openFileCompressed();  /// action 2 start
        void csvread(); // action 2 next steep
        bool recTable(  const QString csvfile  );
        int  createSql( QString line, const QString t, QString &in  );
        bool _sql( const QString query );
        QString longlineSql(QString line, const QString t );
        void quit();
        QDataStream writtel;
        int fcursor;
        QNetworkAccessManager manager;
        QList<QNetworkReply *> currentDownloads;
        QStringList url_list_take;
        QStringList fileincomming;
        QDir dir;
        QString ipadress;
        LongTimer radoswisse;
        bool OnService;
        SqlConsole::SqlMini *miniDB;
};

#endif // GEOHANDLER_H
