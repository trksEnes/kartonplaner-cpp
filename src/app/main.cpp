#include "mainwindow.hpp"
#include <QApplication>
#include <QStyleFactory>
#include <QPainter>
#include <QFile>
#include <iostream>

int main(int argc,char** argv) {
    QApplication app(argc,argv);
    app.setApplicationName("KartonPlaner");
    app.setOrganizationName("Enes Türksever");
    app.setApplicationVersion("1.0.0");
    QLocale::setDefault(QLocale(QLocale::German,QLocale::Germany));
    app.setStyle(QStyleFactory::create("Fusion"));
    // A code-drawn icon keeps the demo independent of external artwork.
    QPixmap icon(256,256);icon.fill(Qt::transparent);
    QPainter p(&icon);p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);p.setBrush(QColor("#187F77"));p.drawRoundedRect(QRectF(8,8,240,240),52,52);
    p.setPen(QPen(QColor("#F6F1DE"),9,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    p.setBrush(QColor("#2F9990"));
    QPolygonF poly;poly<<QPointF(52,90)<<QPointF(128,51)<<QPointF(204,90)<<QPointF(204,171)<<QPointF(128,211)<<QPointF(52,171);
    p.drawPolygon(poly);p.drawLine(52,90,128,131);p.drawLine(128,131,204,90);p.drawLine(128,131,128,211);
    p.drawLine(90,71,166,111);p.end();app.setWindowIcon(QIcon(icon));
    if(app.arguments().contains("--write-examples")) {
        Project demo;QString error;
        QString dir=app.arguments().value(app.arguments().indexOf("--write-examples")+1);
        if(dir.isEmpty())return 2;
        return save_project(dir+"/beispielauftrag.json",demo,error)?0:2;
    }
    MainWindow window;window.show();
    return app.exec();
}
