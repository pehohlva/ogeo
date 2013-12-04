//
// GeoIp handler database build a sqlite3 database 
// Author: Peter Hohl <pehohlva@gmail.com>,    1.12.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

#include "geohandler.h"
#include "sqlite3/qt_sqlite3.h"

//// endresult sqlite3   /Users/dev/.GeoIp/geoipDB.db3
//// http://www.splitbrain.org/blog/2011-02/12-maxmind_geoip_db_and_sqlite
/// http://dev.maxmind.com/geoip/legacy/geolite/
/// http://dev.maxmind.com/geoip/geoip2/geolite2/
/// download cvs format from http://dev.maxmind.com/geoip/legacy/geolite/ and build sqlite3 db & table
/// unicode table number http://unicode-table.com/en/
///  inet_ntoa   quint32 QHostAddress::toIPv4Address() const 

static QString bytesToSize(const qint64 size) {
    if (size < 0)
        return QString();
    if (size < 1024)
        return QObject::tr("%1 B").arg(QString::number(((double) size), 'f', 0));
    if ((size >= 1024) && (size < 1048576))
        return QObject::tr("%1 KB").arg(QString::number(((double) size) / 1024, 'f', 0));
    if ((size >= 1048576) && (size < 1073741824))
        return QObject::tr("%1 MB").arg(QString::number(((double) size) / 1048576, 'f', 2));
    if (size >= 1073741824)
        return QObject::tr("%1 GB").arg(QString::number(((double) size) / 1073741824, 'f', 2));
    return QString();
}


//// comma inside Quotation mark "txt ,ash"  
//// so can split field correct and after replace __CARET44SP_ QString("##44##") by comma ,

static QString strips_carcomma(QString istring) {
    bool intag = false;
    const int fx = istring.length();
    QString new_string;
    const int newchars = QChar('|').unicode(); /// 448
    ///  " = 34
    ///  , = 44
    for (int i = 0; i < fx; i++) {
        QChar vox(istring.at(i));
        int letter = vox.unicode(); ///
        //// intag true is not numeric field!!!!  QChar(--).unicode()
        //// comma
        if (letter == 44 && !intag) {
            //// new_string += istring.at(i);
            new_string.append(QString("|")); /// new separator to splitt
        }
        //// comma 
        if (letter == 44 && intag) {
            new_string.append(__CARET44SP_); //// QString("##44##")
        }
        //// new separator |
        if (letter == newchars && intag) {
            ///// new_string += istring.at(i);
            new_string.append("."); /// new separator in text inside not allow!
        }
        /// alpha numeric or space
        if (letter != 44 && letter != 34 && letter != newchars) {
            new_string += istring.at(i);
        }
        /// Quotation start  if is quote not null
        if (letter == 34 && !intag) {
            intag = true;
            int xnextx = i + 1;
            if (xnextx <= fx) {
                const int nextcharunicode = QChar(istring.at(xnextx)).unicode();
                if (nextcharunicode != 34) {
                    new_string.append("*"); //// first char mark as * to signal is TEXT field 
                } else if (nextcharunicode == 34) {
                    new_string.append("NULL");
                } else {
                    new_string.append("NULL");
                }
            }
            /// Quotation end 
        } else if (letter == 34 && intag) {

            intag = false;
            //// new_string.append(">");
        }
    }
    return new_string;
}

GeoHandler::GeoHandler(QObject *parent, int modus) :
QObject(parent), actionmake(modus) {
    if (!dir.exists(GEOIPCACHE)) {

        dir.mkpath(GEOIPCACHE);
    }
    ///  DROP TABLE IF EXISTS name
   
    /// db.setDatabaseName(GEOIPCACHE + "geoipDB.db3");
    QString userMake("CREATE TABLE IF NOT EXISTS geouser ( uid INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT);");
    QString logMake("CREATE TABLE IF NOT EXISTS geolog ( uid INTEGER PRIMARY KEY AUTOINCREMENT,action TEXT);");
    this->_sql(userMake);
    this->_sql(logMake);
    QDateTime timer0(QDateTime::currentDateTime());
    const QString nametime = timer0.toString("dd.MM.yyyy.HH.mm.ss.zzz");
    const QString salog = QString("INSERT INTO geolog VALUES (NULL,\"Open app on %1\");").arg(nametime);
    this->_sql(salog);



    /// list file http://dev.maxmind.com/geoip/legacy/geolite/ 
    /// GeoLite Country
    url_list_take << "http://geolite.maxmind.com/download/geoip/database/GeoIPCountryCSV.zip";
    //// GeoLite City 
    url_list_take << "http://geolite.maxmind.com/download/geoip/database/GeoLiteCity_CSV/GeoLiteCity-latest.zip";
    //// GeoLite Country IPv6 
    //// url_list_take << "http://geolite.maxmind.com/download/geoip/database/GeoIPv6.csv.gz";
    //// GeoLite City IPv6 (Beta) 
    ////url_list_take << "http://geolite.maxmind.com/download/geoip/database/GeoLiteCityv6-beta/GeoLiteCityv6.csv.gz";
    /// GeoLite ASN
    //// url_list_take << "http://download.maxmind.com/download/geoip/database/asnum/GeoIPASNum2.zip";
    /// GeoLite ASN IPv6
    //// url_list_take << "http://download.maxmind.com/download/geoip/database/asnum/GeoIPASNum2v6.zip";

    connect(&manager, SIGNAL(finished(QNetworkReply*)), SLOT(downloadFinished(QNetworkReply*)));
}

void GeoHandler::execute() {
    QTextStream out(stdout);
    if (actionmake == 1) {
        //// download item

        foreach(QString sturl, url_list_take) {
            QUrl uri(sturl);
            this->doDownload(uri);
        }
    } else if (actionmake == 2) {
        /// scan cache dir and unzip or deflate file 
        if (openFileCompressed()) {
            /// before ...  
            csvread(); //// long long time insert item on db ...
        } else {
            out << "Unable to find zip gz or csv to handle on:" << dir.absolutePath() << "\n";
            out << "Downlod cvs file or call --help.\n";
            out.flush();
            this->quit();
        }
    } else if (actionmake == 3) {
        QHostAddress qtip(ipadress);
        QString question = QString("SELECT loc.* \
                        FROM geolocation loc, \
                             geoblocks blk \
                       WHERE blk.idx = (%1-(%1 % 65536)) \
                         AND blk.startipnum < %1 \
                         AND blk.endipnum > %1 \
                         AND loc.locid = blk.locId;").arg(qtip.toIPv4Address()).simplified();
        QString hresult;
  
        out << "Check int.." << qtip.toIPv4Address() << " ip " << ipadress << "  \n";
        out << "sql:" << question << "  \n";
        out << "rec:" << hresult << "  \n";
        out.flush();
        this->quit();
    }

}

bool GeoHandler::recTable(const QString csvfile) {

    QTextStream out(stdout);
    QString str("x");
    out << str.fill(QChar('*'), 76) << "\n";
    out.flush();
    QFileInfo info(csvfile);
    QString ta = info.baseName().toLower().simplified();
    ta.replace("-", "");
    if (ta == "geolitecityblocks") {
        ta = QString("geoblocks");
    } else if (ta == "geolitecitylocation") {
        ta = QString("geolocation");
    }
    const QString tablename = ta; //// tmps.toLower()
    /// drop table if exist!!! 
    RamBuffer *buffer = new RamBuffer("table");
    buffer->LoadFile(info.absoluteFilePath());
    QString strecsv = buffer->fromUtf8();
    const int total_lines = buffer->stream_lines();
    buffer->clear();
    int xcursor = -1;
    QStringList lines = strecsv.split(QRegExp("(\\n)|(\\n\\r)|\\r|\\n"), QString::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = strips_carcomma(lines.at(i)); // lol for name 
        const QString lc = line.simplified().toLower();
        xcursor++;
        //// out << "{" << xcursor << "} " << line << " \n";
        QString dat; /// fill sql line query 
        if (xcursor == 0 || xcursor == 1) {
            bool makel = true;
            if (lc.indexOf("copyright") != -1) {
                makel = false;
            } else {
                ///
            }
            if (makel && !lc.isEmpty()) {
                QString sqlmake;
                int rec = createSql(line, tablename, sqlmake);
                if (!sqlmake.isEmpty()) {
                    //// out << "sql:" << sqlmake << " \n";
                }
                //// if rec == 1  line having data / rec = -1 having fieldname 
                if (rec == 1) {
                    dat = longlineSql(line, tablename);
                }
            }
        } else {
            if (!lc.isEmpty()) {
                dat = longlineSql(line, tablename);
            }
        }
        if (!dat.isEmpty()) {
            int perc = ((xcursor * 100) / total_lines);
            int mec = perc / 2;
            QString sale = str.fill(QChar('-'), mec) + QString("|>");
            out << "Table:" << tablename << " st:" << perc << "% row:" << xcursor << sale << "\r";
            /// out << sale << "\r";
            out.flush();
        }
    }

    /// search ip on 1ms faster as mysql hack... 
    if (tablename == "geoblocks") {
        this->_sql(QString("ALTER TABLE geoblocks ADD COLUMN idx INTEGER;"));
        this->_sql(QString("CREATE INDEX geoidx ON geoblocks(idx);"));
        this->_sql(QString("UPDATE geoblocks SET idx = (endipnum - (endipnum % 65536));"));
        /* query at end is: ip to long long after
            SELECT loc.*
              FROM geolocation loc,
                   geoblocks blk
             WHERE blk.idx = (3588090629-(3588090629 % 65536))
               AND blk.startipnum < 3588090629
               AND blk.endipnum > 3588090629
               AND loc.locid = blk.locId;  
         */
    }


    out << "Table " << tablename << " end - total line:" << total_lines << "\n";
    out << str.fill(QChar('*'), 76) << "\n";
    out.flush();

    return true;
}

QString GeoHandler::longlineSql(QString line, const QString t) {

    ////  SQLBEEP() << "longlineSql line:--->" << line;
    const QString sep("|");
    QStringList data = line.split(sep);
    if (data.size() == 0) {
        return QString();
    }
    QString seek = QString("INSERT INTO %1 VALUES (").arg(t);
    QStringList flatts;
    flatts << "NULL"; /// autoincrements
    for (int i = 0; i < data.size(); ++i) {
        bool is_text = false;
        int please_quote = 1;
        QString item(data.at(i));
        item = item.simplified();
        if (item.size() > 1) {
            /// search  /// 42 at begin... first 
            if (QChar(item.at(0)).unicode() == 42) {
                is_text = true;
                item = item.right(item.size() - 1);
            }
        } else if (item.size() == 0) {
            item = QString("NULL");
        }
        if (item == "NULL") {
            please_quote = 0;
        }

        if (please_quote == 1) {
            item.replace(QChar(34), ""); /// quote remove if having  
            item.replace(__CARET44SP_, ","); /////  #define __CARET44SP_ \ QString("##44##")
        }


        if (please_quote) {
            //// item = QString("\"%1\"").arg(item);
        }
        const QString insertd = (please_quote == 1) ? QString("\"%1\"").arg(item) : item;
        flatts << insertd;
    }
    seek.append(flatts.join(","));
    seek.append(");");
    this->_sql(seek);

    return seek;
}

//// parse line 0 definition table or value

int GeoHandler::createSql(QString line, const QString t, QString &in) {
    bool havingint = true; /// if having int numericfieldname
    bool havistr = false;
    QString sqldrem = QString("DROP TABLE IF EXISTS %1;").arg(t);
    this->_sql(sqldrem);
    QMap<int, QVariant> fname;
    QString sqlin = QString("CREATE TABLE IF NOT EXISTS %1 ( id INTEGER PRIMARY KEY AUTOINCREMENT,").arg(t);
    QStringList newfieldname;

    const QString sep("|");
    QStringList fields = line.split(sep);
    if (fields.size() == 0) {
        return -1;
    }
    //// all table NOT having fieldname exept 2 table geolocation + geoblocks
    ////  geolocation = GeoLiteCity-Location.csv
    /// geoblocks = GeoLiteCity-Blocks.csv
    if (t == "geolocation") {
        /// make auto fieldname exist here
        havistr = true;
    } else if (t == "geoblocks") {
        havistr = true;
        sqlin.append("startipnum NUMERIC UNIQUE,endipnum NUMERIC UNIQUE,locid INTEGER");
        sqlin.append(");");
        in.clear();
        in = sqlin;
        this->_sql(sqlin);
        return (havistr) ? -1 : 1;
    }


    for (int i = 0; i < fields.size(); ++i) {
        QVariant item(fields.at(i).simplified());
        fname.insert(i, item);
    }
    ///// search if valu numeric or all string
    QMap<int, QVariant>::const_iterator pos;

    for (pos = fname.constBegin(); pos != fname.constEnd(); ++pos) {
        const int x = pos.key();
        QVariant taen = pos.value();
        QString fieldname;
        QString fieldtype = QString("TEXT");
        QString tmps(taen.toString());
        if (!havistr) {
            if (tmps.size() > 1 && tmps != "NULL") {
                /// search  /// 42 at begin is TEXT FIELD ... first 
                if (QChar(tmps.at(0)).unicode() != 42) {
                    fieldtype = QString("INTEGER"); /// no Quotation "  no NULL is INTEGER
                }
            }
        }
        if (havistr) {
            fieldname = QString("%1 TEXT").arg(tmps.simplified().toLower()); /// TEXT all at end make int or float.. lol
        } else {
            fieldname = QString("tab_%1 %2").arg(x).arg(fieldtype);
        }
        newfieldname << fieldname;
    }
    sqlin.append(newfieldname.join(","));
    sqlin.append(");");
    in.clear();
    in = sqlin;
    this->_sql(sqlin);
    return (havistr) ? -1 : 1;
}

void GeoHandler::csvread() {

    QTextStream out(stdout);
    QDir dir(GEOIPCACHE);
    if (dir.exists(GEOIPCACHE)) {
        //// QDir::Size  to read deep dir ->  QDir::DirsFirst

        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::Size)) {
            if (info.isFile()) {
                const QString fullpathfile = info.absoluteFilePath();
                QString ext = info.suffix().toLower();
                if (ext == "csv") {
                    fileincomming << fullpathfile;
                    out << "Register CSV:" << fullpathfile << " size: " << SystemSecure::bytesToSize(info.size()) << " \n";
                    out.flush();
                }
            }
        }
    }
    if (fileincomming.size() < 1) {
        out << "No file register as CSV.... \n";
        out.flush();
        this->quit();
    }

    fileincomming.removeDuplicates();
    for (int i = 0; i < fileincomming.size(); i++) {

        const QString csvfile = fileincomming.at(i);
        bool parse = recTable(csvfile);
    }

    this->quit();
}

bool GeoHandler::openFileCompressed() {
    QTextStream out(stdout);
    fileincomming.clear();
    int cocursor = 0;
    bool canmakenextsteep = false;
    QDir dir(GEOIPCACHE);
    if (dir.exists(GEOIPCACHE)) {

        out << "Read Files on:" << dir.absolutePath() << "\n";
        out.flush();

        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            if (info.isFile()) {
                cocursor++;
                QString ext = info.suffix().toLower();
                if (ext == "csv") {
                    return true;
                }
                const QString fullpathfile = info.absoluteFilePath();
                QByteArray openas = "easy and cool.....";
                QFile file(fullpathfile);
                if (file.open(QIODevice::ReadOnly)) {
                    QByteArray header = file.peek(10); /// 	
                    header.resize(10);
                    const ushort gzip_magic = QChar(header[1]).unicode(); //// header[1] = 0x8b; // gzip-magic[1] 
                    const ushort method = QChar(header[2]).unicode(); /// Compression method DEFLATE 
                    file.reset();
                    if (file.peek(2) == "PK" && gzip_magic == 75) {
                        openas = "ZIP";
                    } else if (gzip_magic == 139 && method == 8 /* gzip-magic DEFLATE search  */) {
                        openas = "GZ";
                    }

                    file.close();
                }
                out << "Handle:" << fullpathfile << " size: " << SystemSecure::bytesToSize(info.size()) << "    Decompress as:" << openas << "\n";
                out.flush();
                if (openas == "GZ") {
                    const QString newname = QString("%1.csv").arg(info.baseName());
                    RamBuffer *buffer = new RamBuffer("csv");
                    buffer->LoadFile(fullpathfile);
                    const qint64 gzsuccess = buffer->gzopen();
                    if (gzsuccess > 0) {
                        if (buffer->flush_onfile(GEOIPCACHE + newname)) {
                            out << "Handle: CSV ok:" << newname << " decompress size:" << SystemSecure::bytesToSize(gzsuccess) << " on Dir: " << GEOIPCACHE << "    \n";
                            out.flush();
                            canmakenextsteep = true;
                            QFile::remove(fullpathfile);
                            fileincomming << GEOIPCACHE + newname;
                        }
                    } else {
                        fprintf(stderr, "GZopen failed on: %s\n", qPrintable(fullpathfile));
                    }
                } else if (openas == "ZIP") {
                    Zip::Stream *kzip = new Zip::Stream(fullpathfile);
                    if (kzip->canread()) {
                        kzip->explode_todir(GEOIPCACHE, 9);
                        QFile::remove(fullpathfile);
                        canmakenextsteep = true;
                        out << "Zip explode all file on Dir:" << GEOIPCACHE << "    \n";
                    } else {
                        fprintf(stderr, "Zip failed on: %s\n", qPrintable(fullpathfile));
                    }
                }
            }
        }


        if (cocursor == 0) {
            out << "Unable to find file on:" << dir.absolutePath() << "\n";
            out << "Downlod cvs file or call --help.\n";
            out.flush();
            this->quit();
        }

    } else {

        out << "Unable to find file on:" << dir.absolutePath() << "\n";
        out << "Downlod cvs file or call --help.\n";
        out.flush();
        this->quit();
    }

    return canmakenextsteep;
}

QString GeoHandler::saveFileName(const QUrl &url) {
    QString path = url.path();
    QString basename = QFileInfo(path).fileName();
    if (basename.isEmpty()) {

        return QString("NullName");
    }
    return basename;
}


//// long wait .... lol

void GeoHandler::downloadProgress(qint64 r, qint64 tot) {

    QTextStream xc(stdout);
    xc << "In:" << bytesToSize(r) << "|" << bytesToSize(tot) << "\r";
    xc.flush();
}
/// start download

void GeoHandler::doDownload(const QUrl &url) {

    QTextStream out(stdout);
    out << "doDownload:" << url.toString();
    out << "\n";
    out.flush();

    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64, qint64)));
}

void GeoHandler::downloadFinished(QNetworkReply *reply) {
    QUrl url = reply->url();
    if (reply->error()) {
        fprintf(stderr, "Download of %s failed: %s\n",
                url.toEncoded().constData(),
                qPrintable(reply->errorString()));
        quit();
        //// 
    } else {
        QString filename = saveFileName(url);
        if (saveToDisk(filename, reply)) {
            /// register download
            fileincomming << filename;
            if (fileincomming.size() == url_list_take.size()) {

                fileincomming.clear();
                url_list_take.clear();
                quit();
                this->deleteLater();
            }
        }
    }

}

bool GeoHandler::_sql(const QString q) {

    return false;
}

void GeoHandler::quit() {
    QCoreApplication::instance()->quit();
}

bool GeoHandler::saveToDisk(const QString &filename, QIODevice *data) {

    const QString destfile = GEOIPCACHE + filename;
    printf("Begin save file to: %s\n", qPrintable(destfile));
    QFile file(destfile);
    if (file.exists()) {
        printf("Overwriting old file: %s\n", qPrintable(filename));
    }
    if (!file.open(QIODevice::WriteOnly)) {
        fprintf(stderr, "Could not open %s for writing: %s\n",
                qPrintable(destfile),
                qPrintable(file.errorString()));
        return false;
    }
    file.write(data->readAll());
    file.close();
    return true;
}


















