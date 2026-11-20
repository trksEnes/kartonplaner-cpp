#include "mainwindow.hpp"
#include "theme.hpp"
#include <QtWidgets>
#include <QtTest>
#include <iostream>
#include <stdexcept>

int checks=0;
void check(bool ok,const char* what){++checks;if(!ok)throw std::runtime_error(what);}
QPushButton* button(QWidget& w,const QString& text){
    for(auto* b:w.findChildren<QPushButton*>())if(b->text()==text)return b;
    throw std::runtime_error(("button missing: "+text).toStdString());
}
void finish(MainWindow& w){
    QElapsedTimer t;t.start();
    while(t.elapsed()<10000){QCoreApplication::processEvents();
        for(auto* b:w.findChildren<QPushButton*>())if(b->text()=="Karton berechnen")return;
        QTest::qWait(10);
    }throw std::runtime_error("calculation timed out");
}
int main(int argc,char** argv){
    QApplication app(argc,argv);app.setStyle("Fusion");
    QTemporaryDir preferences;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,preferences.path());
    try {
        MainWindow w;w.show();QTest::qWait(200);finish(w);
        auto tables=w.findChildren<QTableWidget*>();QTableWidget *articles=nullptr,*cartons=nullptr,*sequence=nullptr;
        for(auto* t:tables){if(t->columnCount()==3)articles=t;
            else if(t->horizontalHeaderItem(0)->text()=="Karton")cartons=t;else sequence=t;}
        check(articles&&cartons&&sequence,"all three tables present");
        check(articles->rowCount()==3&&cartons->rowCount()==8&&sequence->rowCount()==7,"demo runs end to end");
        check(button(w,"Packplan exportieren")->isEnabled(),"valid demo export enabled");
        auto* theme=w.findChild<QPushButton*>("themeToggle");
        PackView* pack_view=nullptr;for(auto* widget:w.findChildren<QWidget*>())if(auto* view=dynamic_cast<PackView*>(widget))pack_view=view;
        check(pack_view,"packing view present");
        check(theme&&!theme->isChecked(),"new preferences start in light mode");
        theme->click();QCoreApplication::processEvents();
        check(theme->isChecked()&&theme->text()=="Hell","dark mode toggle");
        check(QSettings(QSettings::defaultFormat(),QSettings::UserScope,"Enes Türksever","KartonPlaner").value("darstellung/dunkel").toBool(),"dark selection persisted");
        check(sequence->rowCount()==7&&button(w,"Packplan exportieren")->isEnabled(),"theme preserves current plan");
        check(pack_view->grab().toImage().pixelColor(10,10)==theme_colors(true).plot,"packing canvas follows dark theme");
        check(w.grab().save("build/vorschau-dunkel.png"),"dark snapshot saved");
        {MainWindow reopened;check(reopened.findChild<QPushButton*>("themeToggle")->isChecked(),"new window restores theme");}
        theme->click();QCoreApplication::processEvents();
        check(!theme->isChecked()&&!QSettings(QSettings::defaultFormat(),QSettings::UserScope,"Enes Türksever","KartonPlaner").value("darstellung/dunkel").toBool(),"return to light persists");
        check(pack_view->grab().toImage().pixelColor(10,10)==theme_colors(false).plot,"packing canvas follows light theme");
        auto* steps=w.findChild<QSlider*>();check(steps->maximum()==7,"seven packing steps");
        check(w.grab().save("build/vorschau.png"),"window snapshot saved");
        steps->setValue(3);QCoreApplication::processEvents();check(steps->value()==3,"partial packing view");
        auto* mode=w.findChild<QComboBox*>();mode->setCurrentIndex(1);button(w,"Drehen")->click();QCoreApplication::processEvents();
        check(mode->currentIndex()==1,"top view selectable");mode->setCurrentIndex(0);steps->setValue(7);
        auto* count=articles->cellWidget(0,1)->findChild<QSpinBox*>();count->setValue(2);
        check(!button(w,"Packplan exportieren")->isEnabled()&&sequence->rowCount()==0,"edit invalidates stale result");
        button(w,"Karton berechnen")->click();finish(w);check(sequence->rowCount()==5,"changed quantity recalculated");
        auto* tabs=w.findChild<QTabWidget*>();tabs->setCurrentIndex(1);
        QTest::mouseClick(cartons->viewport(),Qt::LeftButton,Qt::NoModifier,cartons->visualItemRect(cartons->item(0,0)).center());
        check(!button(w,"Packplan exportieren")->isEnabled()&&sequence->rowCount()==0,"failed carton selection clears old plan");
        QTimer::singleShot(0,[&]{
            auto* d=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if(!d)return;
            check(d->palette().color(QPalette::Text)==theme_colors(true).text,"dialog inherits dark text palette");
            check(d->grab().save("build/dialog-dunkel.png"),"dark article dialog saved");
            for(auto* e:d->findChildren<QLineEdit*>())if(e->accessibleName()=="Artikelname")e->setText("Testartikel");
            button(*d,"Übernehmen")->click();
        });
        theme->click();button(w,"Artikel hinzufügen")->click();check(articles->rowCount()==4,"add article dialog accepts valid article");
        theme->click();
        button(w,"Karton berechnen")->click();finish(w);check(sequence->rowCount()==6,"new article included");
        QTimer::singleShot(0,[]{
            if(auto* d=qobject_cast<QMessageBox*>(QApplication::activeModalWidget()))
                for(auto* b:d->buttons())if(b->text()=="Verwerfen")b->click();
        });
        button(w,"Beispielauftrag laden")->click();finish(w);tabs->setCurrentIndex(0);
        check(sequence->rowCount()==7,"restore sample order");
        check(w.grab().save("build/vorschau.png"),"final snapshot saved");
        tabs->setCurrentIndex(1);QCoreApplication::processEvents();w.grab().save("build/kartonvergleich.png");
        tabs->setCurrentIndex(2);QCoreApplication::processEvents();w.grab().save("build/packfolge.png");
        tabs->setCurrentIndex(0);
        // No files are written by the UI itself without an explicit save action.
        std::cout<<checks<<" Fenster-Prüfungen bestanden.\n";return 0;
    }catch(const std::exception& e){std::cerr<<"UI FEHLER: "<<e.what()<<'\n';return 1;}
}
