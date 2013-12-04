/* 
 * File:   console_table.h
 * Author: dev
 *
 * Created on 4. dicembre 2013, 08:46
 */

#ifndef CONSOLE_TABLE_H
#define	CONSOLE_TABLE_H

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


//// memo Q_DECLARE_METATYPE not work on other namenspace!

#define i18n QObject::tr

const int fillconsole = 130;

#if 1 //// 1 or 0 
#define DBTELL qDebug
#else
#define DBTELL if (0) qDebug
#endif

struct ConsoleCell {
    int column;
    int row;
    int rtype; //  SQLITE_TEXT = 3  / SQLITE_INTEGER = 1  /  SQLITE_FLOAT = 2  / SQLITE_BLOB = 4 / NULL = 5
    QByteArray rname; /// fielnname 
    QVariant rdata; /// data from the query 

    void setData(QByteArray n, QVariant d, int t) {
        if (d.isNull()) {
            rname = QByteArray("unknow");
        } else {
            rname = n;
        }
        if (d.isNull()) {
            rdata = QVariant("NULL");
            rtype = 5;
        } else {
            rdata = d;
            rtype = t;
        }
    }

    QString cellName() {
        return QString(rname.constData());
    }

    QString cellvalue() {
        if (rtype == 3) {
            return QString(rdata.toByteArray().constData());
        } else if (rtype == 1) {
            return QString(rdata.toByteArray().constData());
        } else if (rtype == 5) {
            return QString("NULL");
        } else if (rtype == 4) {
            return QString("BLOBDATA");
        } else {
            return QString();
        }
        /// QString::number(x.rtype);
    }

    void setCell(int r, int c) {
        row = r;
        column = c;
    }

    bool finderCell(int r, int c) {
        if (row == r && column == c) {
            return true;
        } else {
            return false;
        }
    }

    QVariant data() {
        return rdata;
    }

    operator QVariant() const {
        return QVariant::fromValue(*this);
    }
};

Q_DECLARE_METATYPE(ConsoleCell);

typedef QList<ConsoleCell> cell_list;

struct ConsoleTable {
    int rows;
    int realityrows;
    int cools;
    cell_list loops;

    void clean() {
        rows = 0;
        realityrows = 0;
        cools = 0;
        loops.clear();
    }

    int columnCount() {
        return cools;
    }

    int rowCount() {
        return rows;
    }

    void setCapacity(int co) {
        clean();
        cools = co;
    }

    ConsoleCell pos(int r, int c) {
        for (int i = 0; i < loops.size(); ++i) {
            ConsoleCell x = loops.at(i);
            if (x.finderCell(r, c)) {
                return x;
            }
        }
        ConsoleCell last;
        last.setCell(0, 0);
        return last;
    }

    QStringList header() {
        QStringList toph;
        if (loops.size() < cools) {
            for (int i = 0; i < cools; ++i) {
                toph << ":header_null:";
            }
            return toph;
        }
        for (int i = 0; i < cools; ++i) {
            ConsoleCell x = loops.at(i);
            toph << x.cellName();
        }
        return toph;
    }

    QStringList data_row(int x) {
        int startfromrow = x + 1;
        int endtake = startfromrow * cools;
        if (endtake > loops.size()) {
            return QStringList();
        }
        const int begin = endtake - cools;
        QStringList line;
        for (int i = begin; i < endtake; ++i) {
            ConsoleCell x = loops.at(i);
            line << x.cellvalue(); //// + QString::number(x.rtype);
        }
        return line;
    }

    void insert(ConsoleCell append) {
        loops << append;
    }

    cell_list fullData() {
        return loops;
    }

    void end() {
        const int len = loops.size() / cools;
        rows = len;
        realityrows = len - 1; /// count by zero 
    }

    operator QVariant() const {
        return QVariant::fromValue(*this);
    }
};


Q_DECLARE_METATYPE(ConsoleTable);





#endif	/* CONSOLE_TABLE_H */

