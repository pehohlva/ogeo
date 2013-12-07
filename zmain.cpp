
#include <QApplication>
#include <QMainWindow>
#include <QtGui>
#include <QStringList>
#include <QSlider>
#include <QFileDialog>
#include <QWebFrame>
#include <QColorDialog>
#include <QInputDialog>
#include <QDockWidget>
#include <QDesktopServices>
#include <QWebPage>
#include <QWebView>
#include <QTouchEvent>
#include <QMessageBox>
#include <QAction>
#include <QKeySequence>
#include <QTime>
#include <QPrintDialog>
#include <QPrinter>

#if 1 //// 1 or 0
#define FOXBEEP qDebug
#else
#define FOXBEEP if (0) qDebug
#endif
/// lol i love kde 
#define i18n QObject::tr
const static qreal ZOOMMAX = 7.8;
const static qreal ZOOMMIN = 0.15;
const static int PERCENTZOOMSTEEPS = 8;

class FoxEdit : public QWebPage {
    Q_OBJECT
public:
    explicit FoxEdit(int modus = 1);
signals:
public slots:
private:
protected:
    bool event(QEvent * ev);
};

FoxEdit::FoxEdit(int modus) :
QWebPage(0) {
    /// FOXBEEP() << "FoxEdit init";
    setContentEditable(true);
    setForwardUnsupportedContent(false);
    setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
}

bool FoxEdit::event(QEvent * ev) {
    return QWebPage::event(ev);
}
//// Edit end View separator ************************

class BigFox : public QWebView {
    Q_OBJECT
public:
    BigFox(QWebView *parent = 0);
    FoxEdit *edit;
signals:
public slots:
    void action_url(QUrl url);
    void resetVZoom();
    void printPwd();
    void zoom_minus();
    void zoom_plus();

private:
    qreal useScaleFactor;
    int marker;
protected:
    void changeEvent(QEvent * ev);
    bool event(QEvent * ev);
    void execCommand(const QString &cmd);
    void ZoomNewValue(const qreal fak);
};

BigFox::BigFox(QWebView *parent) :
QWebView(parent), useScaleFactor(3.6), marker(QTime::currentTime().msec()) {
    FOXBEEP() << "BigFox init";
    edit = new FoxEdit(0);
    QWebView::setZoomFactor(2.6);
    setPage(edit);
    load(QUrl("http://www.liberatv.ch/"));
    connect(this, SIGNAL(linkClicked(QUrl)), this, SLOT(action_url(QUrl)));
    QAction *zoom_reset = new QAction(i18n("Reset Zoom"), this);
    zoom_reset->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    connect(zoom_reset, SIGNAL(triggered()), this, SLOT(resetVZoom()));
    addAction(zoom_reset);

    QAction *print_current = new QAction(i18n("Print Page"), this);
    print_current->setShortcut(QKeySequence::Print);
    connect(print_current, SIGNAL(triggered()), this, SLOT(printPwd()));
    addAction(print_current);

    QAction *zoomIN = new QAction(i18n("Zoom in"), this);
    zoomIN->setShortcut(QKeySequence::ZoomIn);
    connect(zoomIN, SIGNAL(triggered()), this, SLOT(zoom_plus()));
    addAction(zoomIN);

    QAction *zoomOUT = new QAction(i18n("Zoom out"), this);
    zoomOUT->setShortcut(QKeySequence::ZoomOut);
    connect(zoomOUT, SIGNAL(triggered()), this, SLOT(zoom_minus()));
    addAction(zoomOUT);
}
void BigFox::zoom_minus() {
    const qreal ticks = ((ZOOMMAX / 5) / 100) * PERCENTZOOMSTEEPS; /// up down by 8%
    const qreal oldFaktorNow = zoomFactor();
    const qreal goZoomFaktor = oldFaktorNow - ticks;
    ZoomNewValue(goZoomFaktor);
}

void BigFox::zoom_plus() {
    const qreal ticks = ((ZOOMMAX / 5) / 100) * PERCENTZOOMSTEEPS; /// up down by 8%
    const qreal oldFaktorNow = zoomFactor();
    const qreal goZoomFaktor = oldFaktorNow + ticks;
    ZoomNewValue(goZoomFaktor);
}

void BigFox::printPwd() {
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFileName("");
    QPrintDialog pp(&printer,this );
    int resultofexec = pp.exec();
#endif
    //// FOXBEEP() << resultofexec << ":" << __FUNCTION__ << " your eye doctor like zoomss..";
}

void BigFox::resetVZoom() {
    QWebView::setZoomFactor(1);
}

void BigFox::action_url(QUrl url) {
    QString msg = QString(tr("Open %1 ?")).arg(url.toString());
    if (QMessageBox::question(this, tr("Open link"), msg,
            QMessageBox::Open | QMessageBox::Cancel) ==
            QMessageBox::Open)
        QDesktopServices::openUrl(url);
}

bool BigFox::event(QEvent * ev) {
    const int nr = ev->type();
    switch (ev->type()) {
        case QEvent::TouchBegin:
        case QEvent::TouchUpdate:
        case QEvent::TouchEnd:
        {
            QTouchEvent *touchEvent = static_cast<QTouchEvent *> (ev);
            QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();
            if (touchPoints.count() == 2) {
                const QTouchEvent::TouchPoint &TpointA = touchPoints.first();
                const QTouchEvent::TouchPoint &TpointB = touchPoints.last();
                const qreal linedistance = QLineF(TpointA.pos(), TpointB.pos()).length(); //// 1111 
                const qreal linestartpos = QLineF(TpointA.startPos(), TpointB.startPos()).length(); //// 222
                bool zoomIn = false;
                const qreal oldFaktorNow = zoomFactor();
                /// ZOOMMAX = 500%
                qreal xmax = qMax(linedistance, linestartpos);
                const qreal ticks = ((ZOOMMAX / 5) / 100) * PERCENTZOOMSTEEPS; /// up down by 8%
                //// is zoom in or out ?
                const qreal goZoomFaktor = (linedistance == xmax) ? oldFaktorNow + ticks : oldFaktorNow - ticks;
                //// if fingers go out save new state 
                if (touchEvent->touchPointStates() & Qt::TouchPointReleased) {
                    ZoomNewValue(goZoomFaktor);
                }
            }
        }
        default:
            break;
    }
    return QWebView::event(ev);
}

void BigFox::changeEvent(QEvent * ev) {
    // const int nr = ev->type();
    QWebView::changeEvent(ev);
}

void BigFox::ZoomNewValue(const qreal fak) {

    //// const int difftime = qMax(QTime::currentTime().msec(), marker);
    if (fak > ZOOMMIN && fak < ZOOMMAX) {
        marker = QTime::currentTime().msec(); /// save last update time .. cpu to repaint.
        QWebView::setZoomFactor(fak);
        QWebView::update();
    }
}

void BigFox::execCommand(const QString &cmd) {
    QWebFrame *frame = this->page()->mainFrame();
    QString js = QString("document.execCommand(\"%1\", false, null)").arg(cmd);
    frame->evaluateJavaScript(js);
}

int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    QMainWindow *xx = new QMainWindow(0);
    BigFox *w = new BigFox(0);
    xx->setCentralWidget(w);
    xx->show();
    return app.exec();
}

#include "main.moc"
/* qmake pro qt5 file xx
 TEMPLATE  = app
DESTDIR = ./
TARGET = xx
QT       += network webkitwidgets  core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport
CONFIG +=   qt warn_off silent debug  console
cache()
 */





