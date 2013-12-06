//
// GeoIp handler database build a sqlite3 database 
// download database as csv compose sqlite3 and ready to query ip
// Author: Peter Hohl <pehohlva@gmail.com>,    1.12.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution
// last update 6.12.2013
/// pwd /Users/dev/project/ogeo

#include <QCoreApplication>
#include "geohandler.h"
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include "decompress.h"

//// endresult sqlite3   /Users/dev/.GeoIp/geoipDB.db3
//// http://www.splitbrain.org/blog/2011-02/12-maxmind_geoip_db_and_sqlite
/// http://dev.maxmind.com/geoip/legacy/geolite/
/// download cvs format from http://dev.maxmind.com/geoip/legacy/geolite/ and build sqlite3 db & table
/// unicode table number http://unicode-table.com/en/
//// http://www.splitbrain.org/blog/2011-02/12-maxmind_geoip_db_and_sqlite
/// http://dev.maxmind.com/geoip/legacy/geolite/
/// download cvs format from http://dev.maxmind.com/geoip/legacy/geolite/ and build sqlite3 db & table

//// geohandler.cpp line 110 ist the list from remote csv file 
///  geohandler.cpp read csv zip or gz file from decompress.h

static void usagethisapp(const char *name, int size) {
    printf("Usage: %s (Options)  \n", name);
    printf(" {%d} Options:\n", size);
    printf("\t--download or -d : take all csv from geoip location and info\n");
    printf("\t--handle or -r : read and convert table to sqlite3 db. +150mb data\n");
    printf("\t-ip 174.36.207.186 : query db sqlite3 \n");
    printf("\t--interactive or -c : start interactive modus on sqlite3 db. and play chat your query\n");
    printf("\t--version or -V: show the version of conversion used\n");
    printf("\t--help or -h: display this text.\n");
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QTextStream out(stdout);
    QString str("x");
    int i;
    if (argc <= 1) {
        usagethisapp(argv[0], argc);
        return (1);
    }
    
    //// qDebug() << qApp->libraryPaths(); /// sqlite3 plugin from qt not need.. 
    /// qFatal("divide: cannot divide by zero");
    
    QDateTime timer0(QDateTime::currentDateTime());
    const QString nametime = timer0.toString("dd.MM.yyyy.HH.mm.ss.zzz");
    int modus = 0;
    QString ipaddo;
    QStringList ogeoarg = a.arguments();

    for (i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-V")) ||
                (!strcmp(argv[i], "--version")) ||
                (!strcmp(argv[i], "-v"))) {
            out << str.fill('*', pointo) << "\n";
            out << __APPNAME__ << ":" << __DOCVERSION__ << "\n";
            out << "GEOIPCACHE Dir on:" << GEOIPCACHE << "\n";
            out << "FreeSpace on Home dir:" << SystemSecure::freespaceonHome() << "\n";
            out << "Action name:" << nametime << "\n";
            out << str.fill('*', pointo) << "\n";
            out.flush();
            return (1); 
        } else if ((!strcmp(argv[i], "--help")) || !strcmp(argv[i], "-h")) {
            usagethisapp(argv[0], argc);
            return (1);
        } else if ((!strcmp(argv[i], "--interactive")) || !strcmp(argv[i], "-c")) {
            modus = 4; 
        } else if ((!strcmp(argv[i], "--download")) || !strcmp(argv[i], "-d")) {
            modus = 1; //// first action 
        } else if ((!strcmp(argv[i], "--handle")) || !strcmp(argv[i], "-r")) {
            modus = 2;   //// second 
        } else if ((!strcmp(argv[i], "-ip"))) {
            /// play results
            modus = 3;
            int namer = i + 2;
            
            if (ogeoarg.size() >= namer ) {
                ipaddo = ogeoarg.at(namer - 1);
            } else {
                printf("Write the ip after -ip *** , unable to find your string..\n");
                return (1);
            }
        } 
    }
    if (modus == 0) {
        return (1);
    }
    
    GeoHandler *job = new GeoHandler(0, modus);
    if (!ipaddo.isEmpty() && modus == 3) {
        job->setIp(ipaddo);
    }
    QTimer::singleShot(0, job, SLOT(execute()));
    return a.exec();
}
