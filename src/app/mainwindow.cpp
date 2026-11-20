#include "mainwindow.hpp"
#include "theme.hpp"
#include <QtWidgets>
#include <QtConcurrent/QtConcurrentRun>
#include <cmath>
#include <exception>

namespace {
QLabel* label(const QString& text,const char* style="") {
    auto* l=new QLabel(text);if(*style)l->setObjectName(QLatin1String(style));return l;
}
QFrame* card() {auto* f=new QFrame;f->setObjectName("card");return f;}
QPushButton* action(const QString& text,const char* style="") {
    auto* b=new QPushButton(text);b->setCursor(Qt::PointingHandCursor);
    if(*style)b->setObjectName(QLatin1String(style));return b;
}
QDoubleSpinBox* decimal(double value,double min,double max,const QString& suffix,int precision=1) {
    auto* s=new QDoubleSpinBox;s->setLocale(QLocale(QLocale::German,QLocale::Germany));
    s->setDecimals(precision);s->setRange(min,max);s->setValue(value);s->setSuffix(suffix);
    s->setMinimumHeight(37);s->setButtonSymbols(QAbstractSpinBox::PlusMinus);s->setSingleStep(precision==3?0.05:0.5);return s;
}
QWidget* field(const QString& title,QWidget* widget) {
    widget->setAccessibleName(title);
    auto* w=new QWidget;auto* layout=new QVBoxLayout(w);layout->setContentsMargins(0,0,0,0);layout->setSpacing(6);
    auto* l=label(title,"fieldLabel");l->setBuddy(widget);layout->addWidget(l);layout->addWidget(widget);return w;
}
void configure_table(QTableWidget* table,const QStringList& headers) {
    table->setColumnCount(headers.size());table->setHorizontalHeaderLabels(headers);
    table->verticalHeader()->hide();table->setShowGrid(false);table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(false);table->setFocusPolicy(Qt::StrongFocus);
    table->horizontalHeader()->setHighlightSections(false);table->setFrameShape(QFrame::NoFrame);
}
QIcon swatch(int index) {QPixmap pix(14,14);pix.fill(Qt::transparent);QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);p.setBrush(article_color(index));p.setPen(Qt::NoPen);p.drawRoundedRect(QRectF(1,1,12,12),3,3);return QIcon(pix);}
QString reason(const karton::Candidate& c) {
    using karton::Status;
    switch(c.status) {
    case Status::Fits:return "Packplan gefunden";
    case Status::Weight:return "Zuladung überschritten";
    case Status::Volume:return "Volumen zu klein";
    case Status::Dimensions:return "Maße passen nicht";
    case Status::NoArrangement:return "Kein Plan gefunden";
    case Status::Cancelled:return "Abgebrochen";
    }
    return {};
}
}

MainWindow::MainWindow() {
    setWindowTitle("KartonPlaner · Packauftrag");resize(1280,850);setMinimumSize(1040,720);
    build_ui();apply_theme(QSettings(QSettings::defaultFormat(),QSettings::UserScope,"Enes Türksever","KartonPlaner").value("darstellung/dunkel",false).toBool());render_articles();update_order_summary();
    connect(&watcher_,&QFutureWatcher<karton::Result>::finished,this,[this] {
        result_=watcher_.result();busy_=false;input_panel_->setEnabled(true);open_->setEnabled(true);save_->setEnabled(true);
        calculate_->setText("Karton berechnen");
        if(result_.cancelled){state_->setText("Berechnung abgebrochen. Dein Auftrag bleibt erhalten.");return;}
        if(!result_.error.empty()){state_->setText(QString::fromStdString(result_.error));return;}
        render_result();
    });
    QTimer::singleShot(100,this,[this]{calculate();});
}
MainWindow::~MainWindow(){if(cancel_)cancel_->store(true);watcher_.waitForFinished();}

void MainWindow::apply_theme(bool dark,bool persist) {
    dark_=dark;const auto t=theme_colors(dark);
    qApp->setPalette(theme_palette(dark));setPalette(theme_palette(dark));
    setStyleSheet(theme_stylesheet(dark));view_->set_dark(dark);
    const QSignalBlocker blocker(theme_toggle_);theme_toggle_->setChecked(dark);
    theme_toggle_->setText(dark?"Hell":"Dunkel");
    theme_toggle_->setToolTip(dark?"Zur hellen Darstellung wechseln":"Zur dunklen Darstellung wechseln");
    theme_toggle_->setAccessibleDescription(dark?"Dunkle Darstellung aktiv":"Helle Darstellung aktiv");
    QPixmap pix(20,20);pix.fill(Qt::transparent);QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);painter.setPen(QPen(t.accent,1.6,Qt::SolidLine,Qt::RoundCap));
    if(dark){
        painter.drawEllipse(QPointF(10,10),3.4,3.4);
        for(int i=0;i<8;++i){painter.save();painter.translate(10,10);painter.rotate(i*45);painter.drawLine(QPointF(0,-6),QPointF(0,-8));painter.restore();}
    }else{
        QPainterPath moon;moon.moveTo(14.6,15.8);moon.cubicTo(5,20,0,8,8,3);
        moon.cubicTo(5.5,10,10,15,14.6,15.8);painter.setBrush(t.accent);painter.drawPath(moon);
    }
    painter.end();theme_toggle_->setIcon(QIcon(pix));
    for(int row=0;row<candidates_->rowCount();++row)
        for(int col=0;col<candidates_->columnCount();++col)
            if(auto* item=candidates_->item(row,col))item->setForeground(result_.candidates[std::size_t(row)].status==karton::Status::Fits?t.accent:t.muted);
    if(persist)QSettings(QSettings::defaultFormat(),QSettings::UserScope,"Enes Türksever","KartonPlaner").setValue("darstellung/dunkel",dark);
}

void MainWindow::build_ui() {
    auto* central=new QWidget;setCentralWidget(central);auto* root=new QVBoxLayout(central);root->setContentsMargins(0,0,0,0);root->setSpacing(0);
    auto* header=new QFrame;header->setObjectName("header");auto* h=new QHBoxLayout(header);h->setContentsMargins(25,15,25,15);
    auto* logo=label("▣","brandMark");
    h->addWidget(logo);auto* names=new QVBoxLayout;names->setSpacing(3);names->addWidget(label("KartonPlaner","brand"));
    names->addWidget(label("Passende Kartons. Nachvollziehbar gepackt.","subtle"));h->addLayout(names);h->addStretch();
    open_=action("Auftrag öffnen");save_=action("Speichern");auto* help=action("So funktioniert’s","quiet");
    theme_toggle_=action("Dunkel","themeToggle");theme_toggle_->setCheckable(true);theme_toggle_->setAccessibleName("Darstellung wechseln");
    h->addWidget(theme_toggle_);h->addSpacing(4);
    connect(theme_toggle_,&QPushButton::toggled,this,[this](bool dark){apply_theme(dark,true);});
    h->addWidget(help);h->addSpacing(6);h->addWidget(open_);h->addWidget(save_);h->addSpacing(8);h->addWidget(label("LOKALE DEMO","badge"));root->addWidget(header);
    connect(help,&QPushButton::clicked,this,[this]{explain();});connect(open_,&QPushButton::clicked,this,[this]{open_project();});connect(save_,&QPushButton::clicked,this,[this]{save_project_as();});

    auto* body=new QHBoxLayout;body->setContentsMargins(22,20,22,10);body->setSpacing(18);root->addLayout(body,1);
    input_panel_=new QWidget;input_panel_->setFixedWidth(390);auto* left=new QVBoxLayout(input_panel_);left->setContentsMargins(0,0,0,0);left->setSpacing(14);body->addWidget(input_panel_);
    auto* order_card=card();auto* order=new QVBoxLayout(order_card);order->setContentsMargins(18,18,18,18);order->setSpacing(12);
    auto* order_title=new QHBoxLayout;order_title->addWidget(label("Dein Packauftrag","sectionTitle"));order_title->addStretch();auto* clear=action("Leeren","quiet");order_title->addWidget(clear);order->addLayout(order_title);
    order_summary_=label("","subtle");order->addWidget(order_summary_);
    articles_=new QTableWidget;articles_->setAccessibleName("Artikel im Packauftrag");configure_table(articles_,{"Artikel / Maße","Anzahl",""});
    articles_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);articles_->setColumnWidth(1,68);articles_->setColumnWidth(2,34);
    articles_->setMinimumHeight(200);order->addWidget(articles_,1);
    auto* buttons=new QHBoxLayout;auto* add=action("Artikel hinzufügen");auto* edit=action("Bearbeiten","quiet");buttons->addWidget(add,1);buttons->addWidget(edit);order->addLayout(buttons);
    auto* example=action("Beispielauftrag laden","quiet");order->addWidget(example,0,Qt::AlignLeft);left->addWidget(order_card,1);
    connect(add,&QPushButton::clicked,this,[this]{edit_article();});connect(edit,&QPushButton::clicked,this,[this]{if(articles_->currentRow()>=0)edit_article(articles_->currentRow());});
    connect(articles_,&QTableWidget::cellDoubleClicked,this,[this](int r,int){edit_article(r);});
    connect(articles_,&QTableWidget::itemSelectionChanged,this,[this]{view_->highlight(articles_->currentRow());});
    connect(clear,&QPushButton::clicked,this,[this]{clear_order();});connect(example,&QPushButton::clicked,this,[this]{load_example();});
    auto* settings_card=card();auto* settings=new QVBoxLayout(settings_card);settings->setContentsMargins(18,17,18,17);settings->setSpacing(11);
    settings->addWidget(label("Verpackung","sectionTitle"));auto* reserves=new QHBoxLayout;reserves->setSpacing(12);
    padding_=decimal(project_.settings.article_padding_mm/10.0,0,20," cm");margin_=decimal(project_.settings.wall_margin_mm/10.0,0,20," cm");
    padding_->setSingleStep(0.1);margin_->setSingleStep(0.1);
    reserves->addWidget(field("Polster je Artikelseite",padding_));reserves->addWidget(field("Randreserve im Karton",margin_));settings->addLayout(reserves);
    auto* cartons=action("Beispielkartons bearbeiten","quiet");settings->addWidget(cartons,0,Qt::AlignLeft);left->addWidget(settings_card);
    connect(cartons,&QPushButton::clicked,this,[this]{edit_cartons();});
    connect(padding_,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[this](double v){if(!loading_){project_.settings.article_padding_mm=int(std::lround(v*10));dirty_=true;invalidate();}});
    connect(margin_,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[this](double v){if(!loading_){project_.settings.wall_margin_mm=int(std::lround(v*10));dirty_=true;invalidate();}});

    auto* right=new QVBoxLayout;right->setSpacing(14);body->addLayout(right,1);
    auto* result_card=card();auto* result_layout=new QHBoxLayout(result_card);result_layout->setContentsMargins(22,18,22,18);result_layout->setSpacing(24);
    auto* recommendation=new QVBoxLayout;recommendation->setSpacing(5);result_title_=label("BESTE GEFUNDENE OPTION","fieldLabel");recommendation->addWidget(result_title_);
    recommendation_=label("Noch kein Packplan","big");recommendation->addWidget(recommendation_);
    result_caption_=label("Starte mit einem Beispiel oder eigenen Artikeln.","subtle");result_caption_->setWordWrap(true);recommendation->addWidget(result_caption_);result_layout->addLayout(recommendation,1);
    auto* fill_group=new QVBoxLayout;fill_group->addWidget(label("Füllgrad","fieldLabel"));fill_=label("–","metric");fill_group->addWidget(fill_);fill_group->addWidget(label("Artikel / Innenvolumen","subtle"));result_layout->addLayout(fill_group);
    auto* weight_group=new QVBoxLayout;weight_group->addWidget(label("Artikelgewicht","fieldLabel"));weight_=label("–","metric");weight_group->addWidget(weight_);weight_group->addWidget(label("ohne Verpackungsmaterial","subtle"));result_layout->addLayout(weight_group);right->addWidget(result_card);

    auto* visual_card=card();auto* visual=new QVBoxLayout(visual_card);visual->setContentsMargins(16,8,16,15);visual->setSpacing(10);
    auto* tabs=new QTabWidget;visual->addWidget(tabs,1);
    auto* picture=new QWidget;auto* picture_layout=new QVBoxLayout(picture);picture_layout->setContentsMargins(0,10,0,0);picture_layout->setSpacing(10);
    auto* view_toolbar=new QHBoxLayout;view_caption_=label("Noch kein Karton gewählt","subtle");view_toolbar->addWidget(view_caption_,1);
    auto* view_mode=new QComboBox;view_mode->addItems({"Räumliche Ansicht","Draufsicht"});view_mode->setAccessibleName("Ansicht der Packanordnung");view_mode->setMinimumHeight(30);view_toolbar->addWidget(view_mode);
    auto* turn=action("Drehen","quiet");view_toolbar->addWidget(turn);picture_layout->addLayout(view_toolbar);
    view_=new PackView;picture_layout->addWidget(view_,1);connect(turn,&QPushButton::clicked,this,[this]{view_->rotate_view();});
    connect(view_mode,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int i){view_->set_top_view(i==1);});
    auto* step_row=new QHBoxLayout;step_label_=label("Packfolge","subtle");step_label_->setMinimumWidth(120);step_row->addWidget(step_label_);
    steps_=new QSlider(Qt::Horizontal);steps_->setRange(0,0);steps_->setAccessibleName("Anzahl eingeblendeter Artikel");step_row->addWidget(steps_,1);picture_layout->addLayout(step_row);
    connect(steps_,&QSlider::valueChanged,this,[this](int v){view_->set_step(v);step_label_->setText(QString("Schritt %1 von %2").arg(v).arg(steps_->maximum()));});
    auto* hint=label("Nummern zeigen die Packreihenfolge. Farbige Quader enthalten das gewählte Artikelpolster.","subtle");hint->setWordWrap(true);picture_layout->addWidget(hint);
    tabs->addTab(picture,"Packansicht");
    candidates_=new QTableWidget;configure_table(candidates_,{"Karton","Innenmaße","Max. Zuladung","Ergebnis"});
    candidates_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    candidates_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    candidates_->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
    candidates_->horizontalHeader()->setSectionResizeMode(3,QHeaderView::ResizeToContents);
    connect(candidates_,&QTableWidget::cellClicked,this,[this](int row,int){show_candidate(row);});tabs->addTab(candidates_,"Kartonvergleich");
    sequence_=new QTableWidget;configure_table(sequence_,{"Schritt","Artikel","Drehung L × B × H","Position X / Y / Z"});
    sequence_->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);sequence_->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    sequence_->horizontalHeader()->setSectionResizeMode(2,QHeaderView::ResizeToContents);sequence_->horizontalHeader()->setSectionResizeMode(3,QHeaderView::ResizeToContents);
    tabs->addTab(sequence_,"Packfolge");right->addWidget(visual_card,1);

    auto* bottom=new QHBoxLayout;state_=label("","subtle");state_->setWordWrap(true);bottom->addWidget(state_,1);
    export_=action("Packplan exportieren");export_->setEnabled(false);calculate_=action("Karton berechnen","primary");
    bottom->addWidget(export_);bottom->addWidget(calculate_);right->addLayout(bottom);
    connect(calculate_,&QPushButton::clicked,this,[this]{calculate();});connect(export_,&QPushButton::clicked,this,[this]{export_current();});
    statusBar()->showMessage("Beispieldaten · frei drehbare, stapelbare Quader · alles bleibt auf diesem Rechner");statusBar()->setSizeGripEnabled(true);
    auto* file=menuBar()->addMenu("Datei");
    auto* fresh=file->addAction("Neuer Auftrag");fresh->setShortcut(QKeySequence::New);connect(fresh,&QAction::triggered,this,[this]{clear_order();});
    auto* open=file->addAction("Auftrag öffnen…");open->setShortcut(QKeySequence::Open);connect(open,&QAction::triggered,this,[this]{open_project();});
    auto* save=file->addAction("Speichern");save->setShortcut(QKeySequence::Save);connect(save,&QAction::triggered,this,[this]{save_project_as();});
    auto* save_as=file->addAction("Speichern unter…");save_as->setShortcut(QKeySequence::SaveAs);connect(save_as,&QAction::triggered,this,[this]{save_project_as(true);});
    file->addSeparator();auto* quit=file->addAction("Beenden");quit->setShortcut(QKeySequence::Quit);connect(quit,&QAction::triggered,this,&QWidget::close);
    auto* help_menu=menuBar()->addMenu("Hilfe");connect(help_menu->addAction("Über die Berechnung"),&QAction::triggered,this,[this]{explain();});
}

void MainWindow::render_articles() {
    loading_=true;articles_->clearContents();articles_->setRowCount(int(project_.articles.size()));
    for(int row=0;row<int(project_.articles.size());++row) {
        const auto& a=project_.articles[std::size_t(row)];
        auto* item=new QTableWidgetItem(swatch(row),QString::fromStdString(a.name)+"\n"+size_text(a.size));
        item->setToolTip(QString::fromStdString(a.name)+"\n"+size_text(a.size)+"\n"+number_text(a.weight_g/1000.0,3)+" kg je Stück");articles_->setItem(row,0,item);articles_->setRowHeight(row,62);
        auto* count=new QSpinBox;count->setRange(1,80);count->setValue(a.quantity);count->setAccessibleName("Stückzahl "+QString::fromStdString(a.name));
        auto* count_wrap=new QWidget;auto* count_layout=new QVBoxLayout(count_wrap);count_layout->setContentsMargins(3,14,3,14);count_layout->addWidget(count);articles_->setCellWidget(row,1,count_wrap);
        connect(count,QOverload<int>::of(&QSpinBox::valueChanged),this,[this,row](int q){if(!loading_){project_.articles[std::size_t(row)].quantity=q;dirty_=true;update_order_summary();invalidate();}});
        auto* remove=action("×","quiet");remove->setToolTip("Artikel entfernen");remove->setAccessibleName(QString::fromStdString(a.name)+" entfernen");
        articles_->setCellWidget(row,2,remove);connect(remove,&QPushButton::clicked,this,[this,row]{project_.articles.erase(project_.articles.begin()+row);dirty_=true;render_articles();update_order_summary();invalidate();});
    }
    loading_=false;
}
void MainWindow::update_order_summary() {
    int n=0;long long grams=0;for(const auto& a:project_.articles){n+=a.quantity;grams+=std::int64_t(a.quantity)*a.weight_g;}
    order_summary_->setText(QString("%1 Stück · %2 Positionen · %3 kg").arg(n).arg(project_.articles.size()).arg(number_text(grams/1000.0,2)));
}
void MainWindow::invalidate() {
    result_={};current_candidate_=-1;candidates_->setRowCount(0);sequence_->setRowCount(0);view_->clear_plan();export_->setEnabled(false);
    recommendation_->setText("Neu berechnen");result_caption_->setText("Der Auftrag wurde geändert.");fill_->setText("–");weight_->setText("–");
    steps_->setRange(0,0);step_label_->setText("Packfolge");view_caption_->setText("Noch kein gültiger Packplan");state_->setText("Bereit für eine neue Berechnung.");
}
void MainWindow::edit_article(int row) {
    if(busy_)return;
    karton::Article a=row>=0?project_.articles.at(std::size_t(row)):karton::Article{"Neuer Artikel",{120,80,60},250,1};
    QDialog dialog(this);dialog.setWindowTitle(row>=0?"Artikel bearbeiten":"Artikel hinzufügen");dialog.resize(540,410);auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(25,23,25,22);layout->setSpacing(18);
    layout->addWidget(label(row>=0?"Artikel bearbeiten":"Ein Artikel für deinen Auftrag","sectionTitle"));
    auto* name=new QLineEdit(QString::fromStdString(a.name));name->setMaxLength(80);name->setMinimumHeight(36);layout->addWidget(field("Artikelname",name));
    auto* dims=new QHBoxLayout;auto* l=decimal(a.size.length/10.0,0.1,500," cm");auto* w=decimal(a.size.width/10.0,0.1,500," cm");auto* h=decimal(a.size.height/10.0,0.1,500," cm");
    dims->addWidget(field("Länge",l));dims->addWidget(field("Breite",w));dims->addWidget(field("Höhe",h));layout->addLayout(dims);
    auto* meta=new QHBoxLayout;auto* weight=decimal(a.weight_g/1000.0,0,1000," kg",3);auto* quantity=new QSpinBox;quantity->setRange(1,80);quantity->setValue(a.quantity);quantity->setMinimumHeight(37);
    meta->addWidget(field("Gewicht je Stück",weight));meta->addWidget(field("Anzahl",quantity));layout->addLayout(meta);
    auto* note=label("Produktmaße ohne Polster eingeben. Alle Artikel dürfen gedreht und gestapelt werden.","subtle");note->setWordWrap(true);layout->addWidget(note);
    auto* buttons=new QDialogButtonBox;auto* cancel=buttons->addButton("Abbrechen",QDialogButtonBox::RejectRole);auto* accept=buttons->addButton("Übernehmen",QDialogButtonBox::AcceptRole);accept->setObjectName("primary");layout->addWidget(buttons);
    connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);
    connect(accept,&QPushButton::clicked,&dialog,[&]{
        if(name->text().trimmed().isEmpty()){QMessageBox::warning(&dialog,"Artikelname fehlt","Bitte gib einen Namen für den Artikel ein.");return;}
        a={name->text().trimmed().toStdString(),{int(std::lround(l->value()*10)),int(std::lround(w->value()*10)),int(std::lround(h->value()*10))},int(std::lround(weight->value()*1000)),quantity->value()};
        auto next=project_.articles;if(row>=0)next[std::size_t(row)]=a;else next.push_back(a);
        auto error=karton::input_error(next,project_.cartons,project_.settings);
        if(!error.empty()){QMessageBox::warning(&dialog,"Bitte Eingaben prüfen",QString::fromStdString(error));return;}dialog.accept();
    });
    if(dialog.exec()!=QDialog::Accepted)return;
    if(row>=0)project_.articles[std::size_t(row)]=a;else project_.articles.push_back(a);
    dirty_=true;render_articles();update_order_summary();invalidate();
}

void MainWindow::edit_cartons() {
    if(busy_)return;
    QDialog dialog(this);dialog.setWindowTitle("Kartonliste bearbeiten");dialog.resize(850,540);auto* layout=new QVBoxLayout(&dialog);layout->setContentsMargins(22,22,22,22);layout->setSpacing(13);
    layout->addWidget(label("Deine Kartonauswahl","sectionTitle"));auto* caption=label("Innenmaße verwenden. Zuladung meint nur die Artikel, ohne Karton- und Polstergewicht.","subtle");caption->setWordWrap(true);layout->addWidget(caption);
    auto* table=new QTableWidget;table->setColumnCount(5);table->setHorizontalHeaderLabels({"Name","Länge (cm)","Breite (cm)","Höhe (cm)","Max. Zuladung (kg)"});
    table->verticalHeader()->hide();table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);table->setSelectionBehavior(QAbstractItemView::SelectRows);layout->addWidget(table,1);
    auto add_row=[&](const karton::Carton& c){int row=table->rowCount();table->insertRow(row);table->setRowHeight(row,44);auto* name=new QLineEdit(QString::fromStdString(c.name));name->setMaxLength(40);table->setCellWidget(row,0,name);
        table->setCellWidget(row,1,decimal(c.inside.length/10.0,0.1,500,""));table->setCellWidget(row,2,decimal(c.inside.width/10.0,0.1,500,""));table->setCellWidget(row,3,decimal(c.inside.height/10.0,0.1,500,""));table->setCellWidget(row,4,decimal(c.max_payload_g/1000.0,0.001,1000,"",3));};
    for(const auto& c:project_.cartons)add_row(c);
    auto* tools=new QHBoxLayout;auto* add=action("Karton hinzufügen");auto* remove=action("Markierten entfernen","quiet");tools->addWidget(add);tools->addWidget(remove);tools->addStretch();layout->addLayout(tools);
    connect(add,&QPushButton::clicked,&dialog,[&]{if(table->rowCount()<40)add_row({"Karton "+std::to_string(table->rowCount()+1),{300,200,150},5000});});
    connect(remove,&QPushButton::clicked,&dialog,[&]{if(table->currentRow()>=0)table->removeRow(table->currentRow());});
    auto* buttons=new QDialogButtonBox;auto* cancel=buttons->addButton("Abbrechen",QDialogButtonBox::RejectRole);auto* accept=buttons->addButton("Kartons übernehmen",QDialogButtonBox::AcceptRole);accept->setObjectName("primary");layout->addWidget(buttons);
    connect(cancel,&QPushButton::clicked,&dialog,&QDialog::reject);std::vector<karton::Carton> next;
    connect(accept,&QPushButton::clicked,&dialog,[&]{next.clear();for(int row=0;row<table->rowCount();++row){auto value=[&](int col){return static_cast<QDoubleSpinBox*>(table->cellWidget(row,col))->value();};
        next.push_back({static_cast<QLineEdit*>(table->cellWidget(row,0))->text().trimmed().toStdString(),{int(std::lround(value(1)*10)),int(std::lround(value(2)*10)),int(std::lround(value(3)*10))},int(std::lround(value(4)*1000))});}
        auto articles=project_.articles;if(articles.empty())articles.push_back({"Prüfung",{1,1,1},0,1});auto error=karton::input_error(articles,next,project_.settings);
        if(!error.empty()){QMessageBox::warning(&dialog,"Bitte Kartons prüfen",QString::fromStdString(error));return;}dialog.accept();
    });
    if(dialog.exec()==QDialog::Accepted){project_.cartons=std::move(next);dirty_=true;invalidate();}
}

void MainWindow::calculate() {
    if(busy_){if(cancel_)cancel_->store(true);calculate_->setText("Wird abgebrochen…");return;}
    auto error=karton::input_error(project_.articles,project_.cartons,project_.settings);
    if(!error.empty()){QMessageBox::information(this,"Auftrag prüfen",QString::fromStdString(error));return;}
    invalidate();busy_=true;input_panel_->setEnabled(false);open_->setEnabled(false);save_->setEnabled(false);
    calculate_->setText("Abbrechen");state_->setText("Kartons und Packanordnungen werden geprüft…");
    recommendation_->setText("Wir packen…");result_caption_->setText("Drehungen, Platz und Zuladung werden berücksichtigt.");
    cancel_=std::make_shared<std::atomic_bool>(false);auto cancel=cancel_;auto snapshot=project_;
    watcher_.setFuture(QtConcurrent::run([snapshot,cancel]{
        try{return karton::solve(snapshot.articles,snapshot.cartons,snapshot.settings,cancel.get());}
        catch(const std::exception& e){karton::Result r;r.error=std::string("Berechnung fehlgeschlagen: ")+e.what();return r;}
    }));
}
void MainWindow::render_result() {
    candidates_->setRowCount(int(result_.candidates.size()));
    for(int row=0;row<int(result_.candidates.size());++row){const auto& c=result_.candidates[std::size_t(row)];
        QStringList fields{QString::fromStdString(c.carton.name),size_text(c.carton.inside),number_text(c.carton.max_payload_g/1000.0,1)+" kg",reason(c)};
        for(int col=0;col<4;++col){auto* item=new QTableWidgetItem(fields[col]);item->setToolTip(QString::fromStdString(c.explanation));
            item->setForeground(c.status==karton::Status::Fits?theme_colors(dark_).accent:theme_colors(dark_).muted);candidates_->setItem(row,col,item);}
        candidates_->setRowHeight(row,43);
    }
    int best=result_.recommended_index();
    if(best<0){recommendation_->setText("Kein Packplan gefunden");result_caption_->setText("Kartonvergleich öffnen oder Artikel und Reserven anpassen.");state_->setText("Ein fehlender Suchtreffer ist kein allgemeiner Unmöglichkeitsbeweis.");return;}
    candidates_->selectRow(best);show_candidate(best);
    state_->setText(QString("%1 Kartons geprüft · %2 Stück · %3 ms").arg(result_.candidates.size()).arg(result_.article_count).arg(result_.elapsed_ms));
}
void MainWindow::show_candidate(int row) {
    if(row<0||row>=int(result_.candidates.size()))return;
    const auto& c=result_.candidates[std::size_t(row)];
    if(c.status!=karton::Status::Fits){
        current_candidate_=-1;export_->setEnabled(false);view_->clear_plan();sequence_->setRowCount(0);steps_->setRange(0,0);
        result_title_->setText("AUSGEWÄHLTER KARTON");recommendation_->setText("Karton "+QString::fromStdString(c.carton.name));
        result_caption_->setText(reason(c));fill_->setText("–");weight_->setText(number_text(result_.weight_g/1000.0,2)+" kg");
        view_caption_->setText("Für diesen Karton liegt kein Packplan vor.");state_->setText(QString::fromStdString(c.explanation));return;
    }
    current_candidate_=row;
    result_title_->setText(row==result_.recommended_index()?"BESTE GEFUNDENE OPTION":"AUSGEWÄHLTER KARTON");
    recommendation_->setText("Karton "+QString::fromStdString(c.carton.name));
    result_caption_->setText(size_text(c.carton.inside)+" innen · "+(row==result_.recommended_index()?"kleinste gefundene Option":"ausgewählte Alternative"));
    fill_->setText(number_text(100.0*result_.article_volume_mm3/c.carton.inside.volume(),1)+" %");
    weight_->setText(number_text(result_.weight_g/1000.0,2)+" kg");
    view_->show_plan(c,project_.settings.article_padding_mm);view_caption_->setText("Karton "+QString::fromStdString(c.carton.name)+" · "+QString::number(c.placements.size())+" nummerierte Artikel");
    steps_->setRange(0,int(c.placements.size()));steps_->setValue(int(c.placements.size()));step_label_->setText(QString("Schritt %1 von %1").arg(c.placements.size()));
    sequence_->setRowCount(int(c.placements.size()));int i=0;
    for(const auto& p:c.placements){QStringList fields{QString::number(i+1),QString::fromStdString(project_.articles[p.article_index].name)+" #"+QString::number(p.copy_number),size_text(p.rotated),
        number_text(p.x/10.0)+" / "+number_text(p.y/10.0)+" / "+number_text(p.z/10.0)+" cm"};
        for(int col=0;col<4;++col){auto* item=new QTableWidgetItem(fields[col]);if(col==1)item->setIcon(swatch(p.article_index));sequence_->setItem(i,col,item);}sequence_->setRowHeight(i,39);++i;}
    export_->setEnabled(true);
}
bool MainWindow::confirm_discard() {
    if(!dirty_)return true;
    QMessageBox box(QMessageBox::Question,"Änderungen speichern?","Dein Packauftrag enthält ungespeicherte Änderungen.",QMessageBox::NoButton,this);
    auto* save=box.addButton("Speichern",QMessageBox::AcceptRole);auto* discard=box.addButton("Verwerfen",QMessageBox::DestructiveRole);box.addButton("Abbrechen",QMessageBox::RejectRole);box.exec();
    if(box.clickedButton()==save)return save_project_as();return box.clickedButton()==discard;
}
void MainWindow::load_example() {
    if(busy_||!confirm_discard())return;project_.articles=karton::example_order();filename_.clear();dirty_=true;render_articles();update_order_summary();invalidate();calculate();
}
void MainWindow::clear_order() {
    if(busy_||!confirm_discard())return;project_.articles.clear();filename_.clear();dirty_=false;render_articles();update_order_summary();invalidate();
}
void MainWindow::open_project() {
    if(busy_||!confirm_discard())return;
    QString path=QFileDialog::getOpenFileName(this,"Packauftrag öffnen",QString(),"Packauftrag (*.json)");if(path.isEmpty())return;
    Project next;QString error;if(!load_project(path,next,error)){QMessageBox::warning(this,"Öffnen fehlgeschlagen",error);return;}
    project_=std::move(next);filename_=path;dirty_=false;loading_=true;padding_->setValue(project_.settings.article_padding_mm/10.0);margin_->setValue(project_.settings.wall_margin_mm/10.0);loading_=false;
    render_articles();update_order_summary();invalidate();setWindowTitle("KartonPlaner · "+QFileInfo(path).fileName());if(!project_.articles.empty())calculate();
}
bool MainWindow::save_project_as(bool force_dialog) {
    if(busy_)return false;
    QString path=filename_;
    if(path.isEmpty()||force_dialog)path=QFileDialog::getSaveFileName(this,"Packauftrag speichern",path.isEmpty()?"Packauftrag.json":path,"Packauftrag (*.json)");
    if(path.isEmpty())return false;if(!path.endsWith(".json",Qt::CaseInsensitive))path+=".json";
    QString error;if(!save_project(path,project_,error)){QMessageBox::warning(this,"Speichern fehlgeschlagen",error);return false;}
    filename_=path;dirty_=false;setWindowTitle("KartonPlaner · "+QFileInfo(path).fileName());state_->setText("Auftrag lokal gespeichert.");return true;
}
void MainWindow::export_current() {
    if(current_candidate_<0||busy_)return;
    QString path=QFileDialog::getSaveFileName(this,"Packplan exportieren","Packplan.csv","CSV-Datei (*.csv)");if(path.isEmpty())return;if(!path.endsWith(".csv",Qt::CaseInsensitive))path+=".csv";
    QString error;if(!export_plan(path,project_,result_.candidates[std::size_t(current_candidate_)],error))QMessageBox::warning(this,"Export fehlgeschlagen",error);
    else state_->setText("Packplan als CSV exportiert. Positionen beziehen sich auf die äußere Polsterkante.");
}
void MainWindow::explain() {
    QMessageBox box(this);box.setWindowTitle("So arbeitet der KartonPlaner");box.setIcon(QMessageBox::Information);
    box.setText("Mehr als ein Volumenvergleich");
    box.setInformativeText("Das Programm prüft bis zu sechs Drehungen je Artikel und mehrere Packreihenfolgen in jedem Karton. Alle Artikel sind rechteckig, frei drehbar und stapelbar.\n\n"
        "Polster wird auf jede Artikelseite aufgeschlagen. Die Randreserve bleibt an allen sechs Kartonwänden frei. Angezeigte Quader und exportierte Positionen enthalten diese Reserven.\n\n"
        "Ein gefundener Plan wird auf vollständige Stückzahlen, Kollisionen, Innenmaße, Zuladung und vollständige Auflagefläche geprüft. Füllgrad bedeutet Artikelvolumen geteilt durch Kartoninnenvolumen.\n\n"
        "Die Empfehlung ist die kleinste gefundene Option, kein mathematischer Optimalitätsnachweis. Für die Demo gelten höchstens 80 Stück und ein begrenztes Suchbudget pro Karton.\n\n"
        "Die maximale Zuladung bezieht sich auf die Artikel. Gewicht von Karton und Polster, Druckfestigkeit, Schwerpunkt und besondere Behandlung empfindlicher Waren werden nicht berechnet. Die acht Startkartons sind Beispieldaten.");
    box.setStandardButtons(QMessageBox::Ok);box.exec();
}
void MainWindow::closeEvent(QCloseEvent* e) {
    if(busy_){if(cancel_)cancel_->store(true);watcher_.waitForFinished();busy_=false;input_panel_->setEnabled(true);open_->setEnabled(true);save_->setEnabled(true);}
    if(confirm_discard())e->accept();else e->ignore();
}
