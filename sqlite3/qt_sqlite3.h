//
// MiniSql to use on QT  sqlite3 database 
// Author: Peter Hohl <pehohlva@gmail.com>,    1.12.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

// last update 6.12.2013

#ifndef QT_SQLITE3_H
#define	QT_SQLITE3_H

#include<time.h> // for clock
#include <math.h> // for fmod
#include <cstdlib> //for system
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "sqlite3.h"
#include <ctype.h>
#include <stdarg.h>
#include <QCoreApplication>
#include <QVariant>
#include <QByteArray>
#include <QString>
#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QDebug>
#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QList>
#include <QDir>
#include <readline/readline.h>
#include <readline/history.h>
#include "console_table.h"
#include <readline/readline.h>
#include <readline/history.h>

static int line_have_semicolon(const char *z, int N) {
    int i;
    for (i = 0; i < N; i++) {
        if (z[i] == ';') return 1;
    }
    return 0;
}

static QString Fx(int fo) {
    QString str;
    QString leading("0");
    str.setNum(fo);
    if ( str.size() == 1 ) {
        return leading + str;
    } else {
       return str; 
    }
}

static QString CronoStop(const int Selapsed) {
    int ddays = 0;
    int dhh = 0;
    int dmm = 0;
    int dss = 0;
    if (Selapsed < (60 * 60 * 24)) {
        dss = fmod(Selapsed, 60);
        int minutes = Selapsed / 60;
        dmm = fmod(minutes, 60);
        int hours = minutes / 60;
        dhh = hours;
        return QString("El.%1:%2:%3").arg(Fx(dhh)).arg(Fx(dmm)).arg(Fx(dss));
    } else {
        return QString("One day.. stop it...");
    }
}

struct LongTimer {
    uint fstart;
    uint lastpit;

    void start() {
        fstart = time_nowLong();
    }

    void start_mid() {
        lastpit = time_nowLong();
    }

    /* time null unix time long nummer */
    uint time_nowLong() {
        QDateTime timer1(QDateTime::currentDateTime());
        return timer1.toTime_t();
    }

    QString elapsedFromStart() {
        uint current = time_nowLong();
        int Selapsed = current - fstart;
        return CronoStop(Selapsed);
    }

    QString elapsedFromLast() {
        uint current = time_nowLong();
        int Selapsed = current - lastpit;
        return CronoStop(Selapsed);
    }
};





//// -help" || rmake == "-f1" || rmake == "-h" 

static char zHelpDb[] =
        "-table  or -t          Show current Table\n"
        "-dump ?TABLE? ...      Dump the database in an SQL text format\n"
        "                         If TABLE specified, only dump tables matching\n"
        "                         LIKE pattern TABLE.\n"
        "-sp                    Update country if having table geoipcountrywhois + geolocation\n"
        "-vacum                 VACUUM to db long time by 100Mb file\n"
        "-lib                   Display sqlite3 version on this app.\n"
        "-help or -f1 or -h     Show this message\n"
        "-exit or -quit or -q   Exit this modus\n"
        ;

namespace SqlConsole {
    
    #define MINISQLVERSION 100040
    #define __MiniSql_Version__ \
              QString("Ve.1.0.4")

    class MiniSqlPrivate;

    class SqlMini {
    public:
        SqlMini();
        void interactive(); /// like original sqlite3 
        bool isOpen();
        //// set file = "ram"  to work on :memory: db
        bool open_DB(const QString & sqlitefile, int modus = 3);
        //// fast check if file is a dbsqliteformat 3
        bool check_file_DB(const QString & sqlitefile);
        //// to update or other no result output ... 
        bool simple_query(const QString query);
        int rowCountTable( const QString tablename );
        QStringList fieldnamesTable(const QString tablename);
        void dump_table( const QString table , QString tofile = QString() );
        SqlResult resultFrom(const QString query);
        int currentnumberRow();
        int currentnumberColumn();
        void clearSqlResult(); // cache /dev/null
        qint64 lastInserID();
        void _consolelog(const QString line, int modus = 0);
        void displayTable();
        void vacumdb(); /// long time if db is big rebuild and order data to bee faster...
        void handle_sp(); /// special query .. 
        ~SqlMini();
    private:
        
        ///// sqlite3 * _db; /// instance of current database...
        //// bool dirty; /// dirty false = db not having VACUUM comand. / dirty true is compact and faster

        QString opendbfilecurrent; /// file dbsqlite3 running
        qint64 LastInsertID;
        QString LastErrorMessage;
        MiniSqlPrivate *d;
    };


}


#endif	/* QT_SQLITE3_H */

