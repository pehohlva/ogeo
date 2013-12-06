//
// MiniSql to use on QT  sqlite3 database here SqlResult cache
// Author: Peter Hohl <pehohlva@gmail.com>,    1.12.2013
// http://www.freeroad.ch/
// Copyright: See COPYING file that comes with this distribution

// last update 6.12.2013

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

#if 0 //// 1 or 0 
#define DBTELL qDebug
#else
#define DBTELL if (0) qDebug
#endif





#define  VINTEGER  1
#define  VFLOAT  2
#define  VBLOB  4
#define  VNULL  5
#define  VTEXT  3
#define  VBOOLEAN 10

struct Field {
    int column;
    int row; //// if row == -1  is header fieldname...
    int fieldtype;
    QByteArray rname; /// fielnname 
    QVariant rdata; /// data filled from the query 

    void init() {
        column = 0;
        row = 0;
        fieldtype = 5;
    }
    //// how to convert Field Type

    const int Row() {
        return row;
    }

    const int Column() {
        return column;
    }

    QVariant::Type getType() {
        QVariant::Type fieldType;
        const int currentType = fieldtype;
        switch (currentType) {
            case VINTEGER:
                fieldType = QVariant::Int;
                break;
            case VFLOAT:
                fieldType = QVariant::Double;
                break;
            case VBLOB:
                fieldType = QVariant::ByteArray;
                break;
            case VTEXT:
                fieldType = QVariant::String;
                break;
            case VNULL:
                fieldType = QVariant::Invalid;
                break;
            default:
                fieldType = QVariant::Invalid;
                break;
        }
        return fieldType;
    }

    void setData(QByteArray n, QVariant d, int t) {
        if (d.isNull()) {
            rname = QByteArray("uname");
        } else {
            rname = n;
        }
        if (d.isNull()) {
            rdata = QVariant("NULL");
            fieldtype = 5;
        } else {
            rdata = d;
            fieldtype = t;
        }
    }

    QString FielName() {
        return QString(rname.constData());
    }

    QVariant Value() {
        return rdata;
    }

    QVariant Data() {
        return rdata;
    }

    int DatatoInt() {
        bool ok;
        int r = rdata.toInt(&ok);
        if (ok) {
            return r;
        } else {
            return 0;
        }
    }

    qreal DatatoReal() {
        bool ok;
        qreal r = rdata.toReal(&ok);
        if (ok) {
            return r;
        } else {
            return 0;
        }
    }

    QString DatatoString() {
        return Svalue();
    }

    double DatatoDouble() {
        bool ok;
        double r = rdata.toDouble(&ok);
        if (ok) {
            return r;
        } else {
            return 0;
        }
    }

    QString Svalue() {
        if (fieldtype == 3) {
            return QString(rdata.toByteArray().constData());
        } else if (fieldtype == 1) {
            return QString(rdata.toByteArray().constData());
        } else if (fieldtype == 5) {
            return QString("NULL");
        } else if (fieldtype == 4) {
            return QString("BLOBDATA");
        } else {
            return QString();
        }
    }

    QByteArray DBdump() {
        if (fieldtype == 3 || fieldtype == 4) {
            QByteArray rec(1,QChar(34).toLatin1());   
            rec.append(rdata.toByteArray());
            rec.append(QChar(34).toLatin1());
            return rec;
        } else if (fieldtype == 1) {
            return rdata.toByteArray();
        } else if (fieldtype == 5) {
            return QByteArray("NULL");
        } else {
            return QByteArray("NULL");
        }
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

    operator QVariant() const {
        return QVariant::fromValue(*this);
    }
};

Q_DECLARE_METATYPE(Field);

typedef QList<Field> Fcache;

struct SqlResult {
    int rows;
    int realityrows;
    int cools;
    Fcache loops; /// data 
    Fcache header; /// header fields

    int size() {
        return loops.size();
    }

    void insertCell(Field append) {
        loops << append;
    }

    void insertHead(Field append) {
        header << append;
    }

    Fcache fullData() {
        return loops;
    }

    void clean() {
        rows = 0;
        realityrows = 0;
        cools = 0;
        loops.clear();
        header.clear();
    }

    void clear() {
        clean();
    }

    int columnCount() {
        return cools;
    }

    int rowCount() {
        return rows;
    }

    void setCapacity(int co, int r) {
        clean();
        cools = co;
        rows = r;
    }

    Field pos(int r, int c) {
        for (int i = 0; i < loops.size(); ++i) {
            Field x = loops.at(i);
            if (x.finderCell(r, c)) {
                return x;
            }
        }
        Field last;
        last.setCell(0, 0);
        return last;
    }

    QStringList Headers() {
        QStringList toph;
        /// DBTELL() << "........c" << cools;
        ////DBTELL() << "........r" << rows;
        //// DBTELL() << "........hs" << header.size();
        if (header.size() != cools) {
            for (int i = 0; i < cools; ++i) {
                toph << ":header_null:";
            }
            return toph;
        }
        for (int i = 0; i < cools; ++i) {
            Field x = header.at(i);
            toph << x.FielName();
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
            Field x = loops.at(i);
            line << x.Svalue(); ////   cellvalue(); //// + QString::number(x.rtype);
        }
        return line;
    }

    void end() {
        const int len = loops.size() / cools;
        rows = len;
        realityrows = len; ///  - 1; /// count by zero ???
    }

    operator QVariant() const {
        return QVariant::fromValue(*this);
    }
};


Q_DECLARE_METATYPE(SqlResult);





#endif	/* CONSOLE_TABLE_H */

