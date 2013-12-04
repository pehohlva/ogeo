/* 
 * File:   qt_sqlite3.cpp
 * Author: dev
 * 
 * Created on 4. dicembre 2013, 08:45
 */
#include "sqlite3.h"
#include "qt_sqlite3.h"
#include "console_table.h"


namespace SqlConsole {

    SqlMini::SqlMini() : is_open(false), dirty(true), is_notcommit(false),LastInsertID(0) {
        //// init class from sqlite3 console namenspace...
    }
    ConsoleTable SqlMini::query_2Table(const QString query) {
        int rec = 0;
        ConsoleTable dummy;
        dummy.clean();
        sqlite3_stmt *sqlStatement;
        LastErrorMessage = QString();
        if (!isOpen()) {
            LastErrorMessage = QString("Warining! DB is not open!");
            this->_consolelog(LastErrorMessage);
            return dummy;
        }
        rec = sqlite3_prepare(_db, qPrintable(query), -1, &sqlStatement, NULL);
        if (rec == SQLITE_OK) {
            return Table(sqlStatement);
        } else {
            send_error(rec);
            return dummy;
        }
    }

    bool SqlMini::simple_query(const QString query) {
        int rec = 0;
        bool mayberowcount = false;
        bool currentAction = false;
        LastQueryIncomming = query;
        QString controller = LastQueryIncomming.toLower();
        if ( controller.indexOf("select") !=-1 || controller.indexOf("distinct") !=-1  ) {
            //// If you want the row count in advance, then there's no easy way. ... lol.
            mayberowcount = true;
        }
        sqlite3_stmt *sqlStatement;
        LastErrorMessage.clear();
        if (!isOpen()) {
            LastErrorMessage = QString(i18n("Warning! DB is not open!..."));
            this->_consolelog(LastErrorMessage);
            return false;
        }
        rec = sqlite3_prepare(_db, qPrintable(query), -1, &sqlStatement, NULL);
        if (rec == SQLITE_OK) {
            qint64 MaybeLastInsertID = sqlite3_last_insert_rowid(_db);
            if (lastinsID > 0) {
                LastInsertID = MaybeLastInsertID;
                currentAction = true;
            }
            if (sqlite3_step(sqlStatement) == SQLITE_ROW) {
                int coolcheck = sqlite3_data_count(sqlStatement);
                ConsoleTable Result = Table(sqlStatement);
                const int rowf = Result.rowCount();
                QStringList header_name = Result.header();
                this->_consolelog(header_name.join(" | "), 2);
                for (int i = 0; i < rowf; ++i) {
                    QStringList line = Result.data_row(i);
                    this->_consolelog(line.join(" | "));
                }
                this->_consolelog(header_name.join(" | "), 2);
                QString sumtot = QString("Total row:%1").arg(rowf);
                this->_consolelog(sumtot);
                this->_consolelog(QString(), 3);
                currentAction = true;
            }
        } else {
            send_error(rec);
        }
        return currentAction;
    }

    void SqlMini::query_console(const QString query) {
        this->simple_query(query);
    }

    void SqlMini::send_error(const int rec) {
        LastErrorMessage = QString(i18n("SqlLite3 Error: ")) + QString::fromUtf8(sqlite3_errmsg(_db));
        LastErrorMessage.append(i18n("  Error MSG INT:"));
        LastErrorMessage.append(QString::number(rec));
        this->_consolelog(LastErrorMessage);
    }

    QString SqlMini::dump_header(const QString table) {
        QString recalc("select sql sqlite_master where name='%1' limit 1;").arg(table);
        ConsoleTable query = SqlMini::query_2Table(recalc);
        
        return QString();
    }

    void SqlMini::interactive() {
        QTextStream out(stdout);
        //// const QString historyfileram = QString("%1/.history_db3").arg(QDir::homePath());
        /////const char *zhistorifile = qPrintable(historyfileram);
        //// read_history(zhistorifile);
        using_history();
        bool ok = false;
        int lastelement = 100;
        QString str("*");
        QString zPrompt = QString("sql>"); ///  ;
        QString cmd;
        out << str.fill('*', fillconsole) << "\n";
        out << "Comand or query:\n";
        out.flush();
        char* input, shell_prompt[300];
        snprintf(shell_prompt, sizeof (shell_prompt), qPrintable(zPrompt));
        fflush(stdout);
        input = readline(shell_prompt);
        cmd = QString(input).simplified();
        if (!cmd.isEmpty()) {
            add_history(input);
            ///// write_history(zhistorifile);
            out << "Last cmd:" << cmd << "\n";
            out.flush();
            if (cmd.endsWith(";")) {
                ok = this->simple_query(cmd);
            } else if (cmd.startsWith(".")) {
                const QString rmake = cmd.simplified();
                if (rmake == ".table") {
                    displayTable();
                } else if (rmake == ".sp") {
                    handle_sp();
                } else if (rmake == ".vacum") {
                    vacumdb();
                } else if (rmake == ".help") {
                    this->_consolelog(QString(zHelpDb));
                } else if (rmake == ".quit") {
                    this->_consolelog("By ......");
                    exit(1);
                } else if (rmake == ".exit") {
                    this->_consolelog("By ......");
                    exit(1);
                } else {
                    interactive(); //// zHelpDb
                }
            }
        }
        interactive();

    }

    void SqlMini::handle_sp() {
        QString recalc("select distinct tab_4,tab_5 from geoipcountrywhois order by tab_4;");
        ConsoleTable query = SqlMini::query_2Table(recalc);
        const int rowf = query.rowCount();
        if (rowf == 0) {
            return;
        }
        const int columm = query.columnCount();
        cell_list listquery = query.fullData();

        for (int i = 0; i < rowf; ++i) {
            const QString longer = query.pos(i, 1).cellvalue();
            const QString small = query.pos(i, 0).cellvalue();
            ///// DBTELL() << " ABR:" << small << " - " << longer;
            QString updd = QString("update geolocation set fucountry='%2' where country='%1';").arg(small).arg(longer);
            if (this->simple_query(updd)) {
                QString status = QString("%1) tot:%2| Ok:").arg(i).arg(rowf);
                this->_consolelog(status + updd);
            } else {
                QString bad = QString("%1) tot:%2| Fail:").arg(i).arg(rowf);
                this->_consolelog(bad + updd);
            }
            //// DBTELL() << " sql:" << updd;
            // select * from geolocation where country='';
            /// delete from geolocation where country='';
            //  DROP TABLE  IF EXISTS name

        }
    }

    /* SELECT sql FROM sqlite_master WHERE name='geolocation';  */
    //// CREATE TABLE IF NOT EXISTS geoblocks ( startIpNum NUMERIC UNIQUE, endIpNum NUMERIC UNIQUE, locId INTEGER REFERENCES geolocation(locID) ); 

    void SqlMini::displayTable() {
        LastErrorMessage = QString();
        QString table_display = "SELECT * " //// name , sql 
                "FROM sqlite_master "
                "WHERE type='table' "
                "ORDER BY name;";
        this->query_console(table_display);
        interactive();
    }

    ConsoleTable SqlMini::Table(sqlite3_stmt *sqlStatement) {
        int rowsrc = -1;
        ConsoleTable table;
        table.clean();
        const int colums = sqlite3_data_count(sqlStatement);
        //// DBTELL() << " reg on  Table " << rowsrc << " x " << colums;
        table.setCapacity(colums);
        while (sqlite3_step(sqlStatement) == SQLITE_ROW) {
            rowsrc++;

            //// DBTELL() << " reg on  rowsrc " << rowsrc << " x " << colums;

            for (int e = 0; e < colums; e++) {
                ConsoleCell field;
                const QByteArray column_Name = QByteArray((const char *) sqlite3_column_name(sqlStatement, e));
                const QByteArray column_Value = QByteArray((const char *) sqlite3_column_text(sqlStatement, e));
                const int column_Type = sqlite3_column_type(sqlStatement, e);
                field.setData(column_Name, QVariant(column_Value), column_Type);
                field.setCell(rowsrc, e);
                table.insert(field);
            }
            //// DBTELL() << " by end rowsrc " << rowsrc << " x " << colums;
        }
        table.end();
        return table;
    }

    /* open the file or the memory db  */
    bool SqlMini::open_DB(const QString & sqlitefile, DBopenMode VARIANTOPEN) {
        bool ok = false;
        const QString memory = QString(":memory:");
        /* open db or file */
        if (sqlitefile == "ram") {
            dberror = sqlite3_open(qPrintable(memory), &_db);
        } else if (VARIANTOPEN == UTF_8) {
            dberror = sqlite3_open(qPrintable(sqlitefile), &_db);
        } else if (VARIANTOPEN == UTF_16) {
            dberror = sqlite3_open16(qPrintable(sqlitefile), &_db);
        }
        if (dberror) {
            LastErrorMessage = QString::fromUtf8(sqlite3_errmsg(_db));
            sqlite3_close(_db);
            _db = 0;
            this->_consolelog(LastErrorMessage);
            return ok;
        }

        if (_db) {
            if (SQLITE_OK == sqlite3_exec(_db, "PRAGMA empty_result_callbacks = ON;", NULL, NULL, NULL)) {
                if (SQLITE_OK == sqlite3_exec(_db, "PRAGMA show_datatypes = ON;", NULL, NULL, NULL)) {
                    ok = true;
                    //// this->vacumdb();
                    this->displayTable();
                }
            }

        }
        opendbfilecurrent = sqlitefile;
        is_open = ok;
        return ok;
    }

    /* check the file db if is sqlite3 format....  */
    bool SqlMini::check_file_DB(const QString & sqlitefile) {
        if (sqlitefile == "ram") {
            return true;
        }
        QByteArray type;
        bool sqlfilefinder = false;
        QFile file(sqlitefile);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray header = file.peek(16); ///         
            const int gzip_magic = QChar(header[1]).unicode(); //// header[1] = 0x8b; // gzip-magic[1] 
            const int unicodeNumber = QChar(header[14]).unicode(); /// is 3 = unicode 51 from SQLite version 3
            header.resize(6);
            if (header == "SQLite" && unicodeNumber == 51) {
                sqlfilefinder = true;
                sversion = Vsql3;
            }
            file.reset();
            file.close();
        }
        return sqlfilefinder;
    }

    /* Log action controller */
    void SqlMini::_consolelog(const QString line, int modus) {
        QString str("*");
        QTextStream out(stdout);
        if (modus == 2) {
            out << str.fill('*', fillconsole) << "\n";
        }
        if (modus != 3) {
            out << line << "\n";
        }
        if (modus == 1 || modus == 2 || modus == 3) {
            out << str.fill('*', fillconsole) << "\n";
        }
        out.flush();
    }

    bool SqlMini::vacumdb() {
        char *errmsg;
        bool ok = false;

        if (!isOpen()) return false;

        if (_db) {
            if (SQLITE_OK == sqlite3_exec(_db, "VACUUM;", NULL, NULL, &errmsg)) {
                ok = true;
                dirty = false;
            }
        }

        if (!ok) {
            LastErrorMessage = QString(errmsg);
            this->_consolelog(LastErrorMessage);
            return false;
        } else {
            return true;
        }
    }

    SqlMini::~SqlMini() {
        _db = NULL;
    }






}






