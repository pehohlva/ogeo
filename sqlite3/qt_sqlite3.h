/* 
 * File:   qt_sqlite3.h
 * Author: dev
 *  4.12.13 untested work progress ..
 * Created on 4. dicembre 2013, 08:45
 */

#ifndef QT_SQLITE3_H
#define	QT_SQLITE3_H

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


static char zHelpDb[] =
        ".backup                Backup DB as timename.db FILE\n"
        ".table                 Show current Table\n"
        ".sp                    Update country if having table geoipcountrywhois + geolocation\n"
        ".vacum                 VACUUM to db long time by 100Mb file\n"
        ".help                  Show this message\n"
        ".exit                  Exit this program\n"
        ".quit                  Exit this program\n"
        ;

namespace SqlConsole {

    enum DBopenMode {
        UTF_8,
        UTF_16,
        VFS
    };

    enum Fileversion {
        Vsql2,
        Vsql3,
        Vsql1
    };

    class SqlMini {
    public:
        SqlMini();
        //// set file = "ram"  to work on :memory: db
        bool open_DB(const QString & sqlitefile, DBopenMode VARIANTOPEN = UTF_8);
        //// fast check if file is a dbsqliteformat 3
        bool check_file_DB(const QString & sqlitefile);
        //// display result
        void query_console(const QString query);
        //// to update or other no result output ... 
        bool simple_query(const QString query);
        
        qint64 lastInserID() {
            return LastInsertID;
        }
        ConsoleTable query_2Table(const QString query);

        bool isOpen() {
            return is_open;
        }
        void _consolelog(const QString line, int modus = 0);
        void displayTable();
        bool vacumdb(); /// long time if db is big rebuild and order data to bee faster...
        void handle_sp(); /// special query .. 
        ~SqlMini();
    private:
        void send_error(const int rec);
        void interactive(); /// take comand from user and exec...
        
        QString dump_header(const QString table);
        sqlite3 * _db; /// instance of current database...
        bool dirty; /// dirty false = db not having VACUUM comand. / dirty true is compact and faster
 
        QString opendbfilecurrent; /// file dbsqlite3 running
        Fileversion sversion;
        bool is_open;
        bool is_notcommit;
        
        
              //// hot variable to clean each time & query!:
        ConsoleTable Table(sqlite3_stmt *sqlStatement);
        void set_Rows_Columns(sqlite3_stmt *sqlStatement);
        void clean_query(); /// call to clean..
        int NumRowIn;
        int NumColumnsIn;
        qint64 LastInsertID;
        QString LastErrorMessage;
        int dberror; /// record last error 
               

    };


}


#endif	/* QT_SQLITE3_H */

