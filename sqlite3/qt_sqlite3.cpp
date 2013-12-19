//
// MiniSql to use on QT  sqlite3 database 
// Author: Peter Hohl <pehohlva@gmail.com>,    1.12.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

// last update 6.12.2013

#include "sqlite3.h"
#include "qt_sqlite3.h"
#include "console_table.h"  
#include "decompress.h"  /// remove dump file and this file not need
#include <QVector>
#include <QVariant>
#include <QByteArray>
#include <QString>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>


#if QT_VERSION < 0x050000



#else
Q_DECLARE_OPAQUE_POINTER(sqlite3*)
Q_DECLARE_METATYPE(sqlite3*)
Q_DECLARE_OPAQUE_POINTER(sqlite3_stmt*)
Q_DECLARE_METATYPE(sqlite3_stmt*)
#endif



#if 0 //// 1 or 0 
#define DBPR qDebug
#else
#define DBPR if (0) qDebug
#endif


namespace SqlConsole {

    static inline QString Sql_mError(sqlite3 *access, QString & mydesc, int errorcode = -1, int lineexec = -1) {
        QString eMsg = QString(reinterpret_cast<const QChar *> (sqlite3_errmsg16(access)));

        QString Nrcode = QString("SQL: ErrorCode:%1.").arg(errorcode);
        QString linetellme = QString("Line:%1.").arg(lineexec);

        if (!mydesc.isEmpty()) {
            eMsg.append("\n");
            eMsg.append(mydesc);
        }
        if (errorcode != -1) {
            eMsg.append("\n");
            eMsg.append(Nrcode);
        }
        if (lineexec > 0) {
            eMsg.append("\n");
            eMsg.append(linetellme);
        }

        return eMsg;
    }

    static int getColumnTypebyName(const QString &tpName) {
        const QString typeName = tpName.toLower();

        if (typeName == QLatin1String("integer")
                || typeName == QLatin1String("int"))
            return VINTEGER;
        if (typeName == QLatin1String("double")
                || typeName == QLatin1String("float")
                || typeName == QLatin1String("real")
                || typeName.startsWith(QLatin1String("numeric")))
            return VFLOAT;
        if (typeName == QLatin1String("blob"))
            return VBLOB;
        if (typeName == QLatin1String("boolean")
                || typeName == QLatin1String("bool"))
            return VBOOLEAN;

        return VTEXT;
    }

    class MiniSqlPrivate {
    public:
        MiniSqlPrivate();
        /// mode 0 = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
        /// mode 1 = SQLITE_OPEN_READONLY
        /// mode 3  = QSQLITE_ENABLE_SHARED_CACHE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
        bool SQL_Open(const QString sqlite3dbfile, int mode = 0);
        bool prepost(const QString query, int out = 1);

        SqlResult q_result() {
            return cache;
        }
        void clearSqlResult();

        bool ISOpen() {
            return is_Open;
        }
        bool Close();

        int numRowsAffected() {
            return row;
        }

        int changeAffected() {
            return affectrow;
        }

        QString lastError() {
            return lasterr;
        }
        ~MiniSqlPrivate();
        QString lasterr;
        int row;
        int column;
        sqlite3 *access;
        sqlite3_stmt *stmt;
        SqlResult cache;
        bool is_Open;
        int lastInsertid;
        int affectrow; /// row is chanched total by query...
    private:
        bool readSelect(int mode);
        void readdata();
        void readLine(const int Row);
        void paint_OnStdout(int header);
        void registerHeader(bool first_row, const int nCols);
        void finalize(); /// call at end from row loop... or to stop the loop
    };

    MiniSqlPrivate::MiniSqlPrivate()
    : access(0), stmt(0), row(0), column(0), is_Open(false), affectrow(0) {
        cache.clear();
    }

    bool MiniSqlPrivate::SQL_Open(const QString fi, int mode) {
        if (this->ISOpen()) {
            this->Close();
        }
        int openMode = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, timeOut = 5000;
        if (mode == 3) {
            sqlite3_enable_shared_cache(true);
        }
        if (mode == 1) {
            openMode = SQLITE_OPEN_READONLY, timeOut = 5000;
        }
        if (sqlite3_open_v2(fi.toUtf8().constData(), &access, openMode, NULL) == SQLITE_OK) {
            sqlite3_busy_timeout(access, timeOut);
            is_Open = true;
            lasterr.clear();
        } else {
            if (access) {
                sqlite3_close(access);
                this->Close();
            }
            lasterr = QString(i18n("Error opening database"));
            return false;
        }
        /// parse query type ... stay open
        bool _pragma = false;
        if (is_Open) {
            //// DBPR() << "dbopen OK." << lasterr;
            if (SQLITE_OK == sqlite3_exec(access, "PRAGMA empty_result_callbacks = ON;", NULL, NULL, NULL)) {
                if (SQLITE_OK == sqlite3_exec(access, "PRAGMA show_datatypes = ON;", NULL, NULL, NULL)) {
                    _pragma = true;
                }
            }
        }

        return this->ISOpen();
    }

    bool MiniSqlPrivate::Close() {
        finalize();
        if (sqlite3_close(this->access) != SQLITE_OK) {
            lasterr = QString(i18n("Error closing database"));
        }
        if (ISOpen()) {
            this->access = 0;
            is_Open = false;
        }
        //// DBPR() << "MiniSqlPrivate::Close() call" << lasterr;
        return is_Open;
    }

    void MiniSqlPrivate::clearSqlResult() {
        cache.clear();
    }

    void MiniSqlPrivate::registerHeader(bool first_row, const int nCols) {
        cache.clear(); /// remove old data if having 
        if (nCols <= 0) {
            return;
        }
        if (!first_row) {
            return;
        }
        Q_ASSERT(nCols == column);

        if (column <= 0) {
            return;
        }
        cache.setCapacity(column, row);

        for (int i = 0; i < nCols; ++i) {
            QString colName = QString(reinterpret_cast<const QChar *> (sqlite3_column_name16(stmt, i)))
                    .remove(QLatin1Char('"'));
            const QByteArray column_Value = QByteArray((const char *) sqlite3_column_text(stmt, i));
            int reg_type = -1;
            /// depend of the query type different function work...
            const char *xtyper = sqlite3_column_decltype(stmt, i);
            const QString typeasstra(xtyper);

            QString typeasstrb = QString(reinterpret_cast<const QChar *> (sqlite3_column_decltype16(stmt, i)));
            QString typeasstrc = QString(reinterpret_cast<const QChar *> (sqlite3_column_name(stmt, i)));
            QString nv = QString();
            bool having_n = false;
            if (!typeasstra.isEmpty()) {
                nv = typeasstra; /// 
                having_n = true;
            } else if (!having_n && !typeasstrb.isEmpty()) {
                nv = typeasstrb;
                having_n = true;
            } else if (!having_n && !typeasstrc.isEmpty()) {
                nv = typeasstrc;
                having_n = true;
            }
            if (!nv.isEmpty() && !having_n) {
                reg_type = getColumnTypebyName(nv);
            } else {
                reg_type = sqlite3_column_type(stmt, i);
            }
            //// header data start 
            Field dd;
            dd.init();
            dd.setCell(-1, i);
            dd.setData(colName.toLocal8Bit(), QVariant("header"), reg_type);
            //// header data end 
            //// data line zero  start 
            Field x;
            x.init();
            x.setCell(0, i); /// this is line 1° row zero.
            x.setData(colName.toLocal8Bit(), QVariant(column_Value), reg_type);
            //// data line zero  end 
            cache.insertHead(dd);
            cache.insertCell(x);
        }
        cache.end();
    }

    void MiniSqlPrivate::readLine(const int Row) {

        if (column <= 0) {
            return;
        }
        if (Row <= 0) {
            return;
        }

        for (int c = 0; c < column; ++c) {
            //// column loop 
            const QByteArray column_Name = QByteArray((const char *) sqlite3_column_name(stmt, c));
            const QByteArray column_Value = QByteArray((const char *) sqlite3_column_text(stmt, c));
            const int column_Type = sqlite3_column_type(stmt, c);
            //// DBPR() << realrow << ")column_Value:" << column_Value;
            Field x;
            x.init();
            x.setCell(Row, c); /// this is line Row° after  zero.
            x.setData(column_Name, QVariant(column_Value), column_Type);
            cache.insertCell(x);
            cache.end(); /// close each row 
        }
    }

    bool MiniSqlPrivate::readSelect(int mode) {
        row = 0;
        int row_virtual = -1;
        column = sqlite3_data_count(stmt);
        const QByteArray First_0_column_Name = QByteArray((const char *) sqlite3_column_name(stmt, 0));
        if (!First_0_column_Name.isEmpty()) {
            row_virtual = 1;
            cache.clear();
            registerHeader(true, column); /// take header ++ line 0. one sqlite3_step used... to switch data result.
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            row_virtual++;
            const int realrow = row_virtual - 1;
            readLine(realrow);
        }
        row = (row_virtual != -1) ? row_virtual : 0;
        if (row > 0 && column > 0) {
            /// display result ... 
            paint_OnStdout(mode);
        } else {
            cache.clear();
        }
        finalize();
        return (row != 0) ? true : false;
    }

    void MiniSqlPrivate::paint_OnStdout(int modus) {
        int rowsRec = cache.rows;
        int columsrec = cache.cools;
        DBTELL() << "modus:" << modus << "r:" << rowsRec << "c:" << columsrec;
        if (modus < 1) {
            return; /// 0 not print..
        }
        QStringList topheaderDescription = cache.Headers();
        QTextStream out(stdout);
        if (modus > 0) {
            out << topheaderDescription.join("|") << "\n";
        }
        for (int i = 0; i < rowsRec; ++i) {
            QStringList iline = cache.data_row(i);
            out << iline.join("|") << "\n";
            out.flush();
        }
        out.flush();
    }

    bool MiniSqlPrivate::prepost(const QString query, int out) {
        /// sqlite3_changes(d->access); row?? 
        //// DBPR() << "MiniSqlPrivate::prepost() in query:" << query;
        //// DBPR() << "..............................................";
        const void *pzTail = NULL;
        int changes_row_tot_current_action = -1;
        affectrow = 0;
        if (this->ISOpen()) {
#if (SQLITE_VERSION_NUMBER >= 3003011) //// dev on 3008002
            int res = sqlite3_prepare16_v2(access, query.constData(), (query.size() + 1) * sizeof (QChar),
                    &stmt, &pzTail);
#else
            int res = sqlite3_prepare16(access, query.constData(), (query.size() + 1) * sizeof (QChar),
                    &stmt, &pzTail);
#endif
            if (res != SQLITE_OK) {
                lasterr.clear();
                QString MyComment(i18n("Unable to prepare query..."));
                lasterr = Sql_mError(access, MyComment, res, __LINE__);
                finalize();
                return false;
            } else if (pzTail && !QString(reinterpret_cast<const QChar *> (pzTail)).trimmed().isEmpty()) {
                lasterr.clear();
                QString MyComment(i18n("Unable to execute multiple statements at a time"));
                lasterr = Sql_mError(access, MyComment, res, __LINE__);
                finalize();
                return false;
            }



        } else {
            return false; /// db is not open
        }



        if (out != -1) {
            affectrow = 0;
            bool exec_is_select = sqlite3_step(stmt) == SQLITE_ROW; /// one slice from row zero is out
            if (exec_is_select) {
                //// inside here can stop
                return readSelect(out);
            }
            bool exec_result = sqlite3_step(stmt) == SQLITE_DONE; /// process to not continue to parse...
            if (exec_result && !exec_is_select) {
                /// count INSERT, UPDATE, or DELETE statement only... NOT SELECT here!!!
                changes_row_tot_current_action = sqlite3_changes(access); // sqlite3_total_changes(access);
                if (changes_row_tot_current_action > 0) {
                    affectrow = changes_row_tot_current_action;
                }
                int lastinsID = sqlite3_last_insert_rowid(access);
                lastInsertid = (lastinsID > 0) ? lastinsID : -1;
                if (lastInsertid != -1 && changes_row_tot_current_action != 0) {
                    finalize();
                    return true;
                }
            }
        }

        lastInsertid = -1;
        finalize();
        return true;

    }

    void MiniSqlPrivate::finalize() {
        if (!stmt) {
            return;
        }
        sqlite3_finalize(stmt);
        stmt = 0;
    }

    MiniSqlPrivate::~MiniSqlPrivate() {
        cache.clear();
        Close();
    }

    SqlMini::SqlMini() : d(new MiniSqlPrivate()) {
        //// init class from sqlite3 console namenspace...

    }

    bool SqlMini::simple_query(const QString query) {

        bool ok = d->prepost(query, 2); /// print to console header yes 
        if (!ok) {
            this->_consolelog(QString("SqlMini::Error on simple_query %1...").arg(d->lastError()));
        }
        /// free result to console
        return ok;

    }

    void SqlMini::displayTable() {
        LastErrorMessage = QString();
        QString table_display = "SELECT * " //// name , sql 
                "FROM sqlite_master "
                "WHERE type='table' "
                "ORDER BY name;";
        simple_query(table_display);
    }

    /* open the file or the memory db  */
    bool SqlMini::open_DB(const QString & sqlitefile, int modus) {
        return d->SQL_Open(sqlitefile, modus);
    }

    /* check the file db if is sqlite3 format....  */
    bool SqlMini::check_file_DB(const QString & sqlitefile) {
        if (sqlitefile == ":memory:") {
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
            DBTELL() << "header file " << unicodeNumber;
            if (unicodeNumber == 51) {
                sqlfilefinder = true;
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

    void SqlMini::vacumdb() {
        //// VACUUM;
        simple_query("VACUUM;");
    }

    void SqlMini::clearSqlResult() {
        d->clearSqlResult();
    }

    SqlMini::~SqlMini() {
        d->Close();
    }

    bool SqlMini::isOpen() {
        return d->ISOpen();
    }

    int SqlMini::currentnumberRow() {
        return d->row;
    }

    int SqlMini::currentnumberColumn() {
        return d->column;
    }

    qint64 SqlMini::lastInserID() {
        return d->lastInsertid;
    }

    void SqlMini::interactive() {
        QDateTime timer0(QDateTime::currentDateTime());
        const QString ddname = timer0.toString("dd.MM.yyyy.HH.mm");
        QTextStream out(stdout);
        QString minimalhelp = QString("Enter SQL statements multiline terminated with a \";\"\n");
        minimalhelp.append("Enter \"-help\" for instructions -q to quit\n");
        const char *zHelpNow = "-help";
        using_history();
        bool ok = false;
        int lastelement = 100;
        QString str("*");
        QString sql_continue("-->");
        QString zPrompt = QString("sql>"); ///  ;
        QString cmd;
        out << str.fill('*', fillconsole) << "\n";
        out << minimalhelp;
        out << "Comand or query:\n";
        out.flush();
        char* input, shell_prompt[500];
        snprintf(shell_prompt, sizeof (shell_prompt), qPrintable(zPrompt));
        fflush(stdout);
        QString execnow;
        while (1) {
            input = readline(shell_prompt);
            bool comand_exec = false;
            unsigned int le_input = (unsigned) strlen(input);
            int ser = line_have_semicolon(input, le_input);
            if (le_input > 0) {
                if (input[0] == '-') {
                    comand_exec = true;
                }
            } else {
                rl_insert_text(zHelpNow);
            }
            execnow.append(QString(input).simplified());
            if (ser == 0 && !comand_exec) {
                out << sql_continue;
                out.flush();
            } else if (ser == 1 || comand_exec) {
                break;
            }
        }
        cmd = execnow.simplified();
        const char *fullinput = qPrintable(cmd);
        if (!cmd.isEmpty()) {
            add_history(fullinput);
            out << "Last cmd:" << cmd << "\n";
            out.flush();
            if (cmd.endsWith(";")) {
                ok = this->simple_query(cmd);
            } else if (cmd.startsWith("-")) {
                const QString rmake = cmd.simplified(); //// rmake.startsWith("Ban");  
                if (rmake == "-table" || rmake == "-t") {
                    displayTable();
                } else if (rmake == "-sp") {
                    handle_sp();
                } else if (rmake.startsWith("-dump")) {
                    /// check after -dump 5-
                    QString say = rmake;
                    const QString tablenn = rmake.right(5).simplified();
                    QString stab = say.replace("-dump", "").simplified().trimmed();
                    if (tablenn == "-dump") {
                        this->_consolelog("To dump all table use sqlite3 console.. or download and rebuild all table..");
                    } else {
                        QStringList fsx = fieldnamesTable(stab);
                        QString disit("Search for table: ");
                        this->_consolelog(disit + stab);
                        if (fsx.size() < 1) {
                            QString sorry("Sorry table not exist: ");
                            this->_consolelog(sorry + stab);
                        } else {
                            QString fddox("Found table: ");
                            QString destfile = QString("%1/%2_%3.sql").arg(QDir::homePath()).arg(stab).arg(ddname);
                            this->_consolelog(fddox + stab);
                            dump_table(stab, destfile);
                        }
                    }

                } else if (rmake == "-lib") {
                    const int versa = (int) SQLITE_VERSION_NUMBER; /// 
                    QString dversion = QString("Sqlite3 version %1 - %2.").arg(versa).arg(QString(SQLITE_VERSION));
                    QString dmini = QString("MiniSql app version %1 - %2.").arg(__MiniSql_Version__).arg((int) MINISQLVERSION);
                    dversion.append("\n");
                    dversion.append(dmini);
                    this->_consolelog(dversion);
                } else if (rmake == "-vacum") {
                    vacumdb();
                } else if (rmake == "-help" || rmake == "-f1" || rmake == "-h") {
                    this->_consolelog(QString(zHelpDb));
                } else if (rmake == "-quit" || rmake == "-exit" || rmake == "-q") {
                    this->_consolelog("By ....  The next time you see your eye doctor...   lol...");
                    exit(1);
                } else {
                    interactive(); /// __MiniSql_Version__
                }
            }
        }
        interactive();

    }

    QStringList SqlMini::fieldnamesTable(const QString tablename) {
        QString query = QString("select * from %1 limit 1;").arg(tablename);
        SqlResult freeresult = resultFrom(query);
        return freeresult.Headers();
    }

    int SqlMini::rowCountTable(const QString tablename) {
        QString query = QString("select count(*) as tot from %1;").arg(tablename);
        SqlResult freeresult = resultFrom(query);
        if (freeresult.size() == 1) {
            const int total = freeresult.pos(0, 0).DatatoInt();
            if (total > 0) {
                freeresult.clear();
                return total;
            }
        }
        return 0;
    }

    void SqlMini::dump_table(const QString table, const QString tofile) {
        QString destfile = tofile;
        if (destfile.size() < 2) {
            destfile = QString("%1.sql").arg(table);
        }
        int xtot = rowCountTable(table);
        const QString xlink = QString("INSERT INTO %1 (").arg(table);
        QString dcreate = QString("select name,sql from sqlite_master where name=\"%1\";").arg(table);
        SqlResult ahead = resultFrom(dcreate);
        const int ccrow = ahead.rowCount();
        const int cccool = ahead.columnCount();
        RamBuffer *buffer = new RamBuffer("sqldump");
        if (cccool == 2) {
            if (!tofile.isEmpty()) {
                QString msga = QString("Write fo file: %1").arg(tofile);
                this->_consolelog(msga);
            }
            QString maketable = ahead.pos(0, 1).DatatoString();
            maketable.append(";\n");
            buffer->device()->write(maketable.toLatin1());
            this->_consolelog(maketable);
            if (xtot > 0) {
                QString ddlongdat = QString("select * from %1;").arg(table); /// test  limit 22
                SqlResult xblobber = resultFrom(ddlongdat);
                const int cools = xblobber.columnCount();
                const int fuller = xblobber.size();
                int columcount = 0;
                QByteArray stream;
                const QByteArray sep(",");
                for (int i = 0; i < fuller; ++i) {
                    //// stream.append(columcount);
                    Field x = xblobber.loops.at(i);
                    stream.append(x.DBdump());
                    if (columcount != (cools - 1)) {
                        stream.append(sep);
                    }
                    columcount++;
                    if (columcount == cools) {
                        columcount = 0;
                        /// new line row... QString
                        QByteArray ddx(xlink.toLatin1());
                        ddx.append(stream);
                        ddx.append(");\n");
                        const QByteArray line = ddx;
                        buffer->device()->write(line);
                        stream.clear();
                        QString show(line.simplified().constData());
                        this->_consolelog(show.simplified());
                    }

                }
                buffer->device()->close();
                if (buffer->flush_onfile(destfile,"utf-8")) {
                    QString msga = QString("Ready file: %1").arg(tofile);
                    this->_consolelog(msga);
                }

            }

        }
        buffer->clear();
        buffer = 0;
    }

    void SqlMini::handle_sp() {
        int geoipcountrywhois_total = rowCountTable("geoipcountrywhois");
        int countryworld_total = rowCountTable("countryworld");
        int geolocation_total = rowCountTable("geolocation");
        if (countryworld_total < 10) {
            QString rem_world_name = QString("DROP TABLE IF EXISTS countryworld;");
            this->simple_query(rem_world_name);
            QString make_world_name = QString("CREATE TABLE countryworld ( country TEXT,longcountry TEXT);");
            this->simple_query(make_world_name);
        }
        if (geolocation_total > 0) {
            QStringList destfield = fieldnamesTable("geolocation");
            int findx = destfield.lastIndexOf("fucountry");
            if (destfield.lastIndexOf("fucountry") == -1) {
                this->simple_query(QString("ALTER TABLE geolocation ADD COLUMN fucountry TEXT;"));
            }
            //// this->_consolelog(destfield.join("-"));
        }


        QString play1 = QString("countryworld having tot. %1").arg(countryworld_total);
        this->_consolelog(play1);
        QString play2 = QString("geoipcountrywhois tot. %1").arg(geoipcountrywhois_total);
        this->_consolelog(play2);
        bool rebuildYes = false;

        if (geoipcountrywhois_total > 7000) {
            QString rem_world_name = QString("DROP TABLE IF EXISTS countryworld;");
            this->simple_query(rem_world_name);
            QString make_world_name = QString("CREATE TABLE countryworld ( country TEXT,longcountry TEXT);");
            this->simple_query(make_world_name);
            QString refill_worldname = QString("INSERT INTO countryworld select distinct tab_4,tab_5 from geoipcountrywhois order by tab_4;");
            this->simple_query(refill_worldname);
            rebuildYes = true;
        } else {
            QString summary = QString("Table geoipcountrywhois not having enought row, only tot. %1 found!.").arg(geoipcountrywhois_total);
            this->_consolelog(summary);
        }

        if (rebuildYes) {
            if (geolocation_total > 0) {
                QString recalcx("select distinct country,longcountry from countryworld order by country;");
                SqlResult ccquery = resultFrom(recalcx);
                const int ccrow = ccquery.rowCount();
                const int cccool = ccquery.columnCount();
                if (ccrow > 1 && cccool > 1) {
                    for (int i = 0; i < ccrow; ++i) {
                        const QString longer = ccquery.pos(i, 1).DatatoString();
                        const QString small = ccquery.pos(i, 0).DatatoString();
                        QString updd = QString("update geolocation set fucountry=\"%2\" where country=\"%1\";").arg(small).arg(longer);
                        if (this->simple_query(updd)) {
                            const int smod = d->changeAffected();
                            QString status = QString("%1) tot:%2|aff:%3 Ok:").arg(i).arg(ccrow).arg(smod);
                            this->_consolelog(status + updd);
                        } else {
                            QString bad = QString("%1) tot:%2| Fail:").arg(i).arg(ccrow);
                            this->_consolelog(bad + updd);
                        }
                        /// const QString small = query.pos(i, 0).cellvalue();

                    }
                }
            }
            //// long update ....

            int c_total = rowCountTable("countryworld");
            QString playx = QString("Refill new countryworld having tot. %1").arg(c_total);
            this->_consolelog(playx);

        }

    }

    SqlResult SqlMini::resultFrom(const QString query) {
        SqlResult nullresult;
        nullresult.clear();
        if (d->prepost(query, 0)) {
            SqlResult x = d->q_result();
            clearSqlResult();
            return x;
        } else {
            return nullresult;
        }
    }

}






