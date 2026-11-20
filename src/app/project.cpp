#include "project.hpp"
#include <QFile>
#include <QSaveFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <cmath>
#include <stdexcept>

namespace {
QJsonObject size_json(karton::Size s) {
    return {{"length_mm",s.length},{"width_mm",s.width},{"height_mm",s.height}};
}
int integer(const QJsonObject& o,const char* key,int min,int max) {
    auto v=o.value(QLatin1String(key));double n=v.toDouble();
    if(!v.isDouble()||!std::isfinite(n)||n!=std::floor(n)||n<min||n>max)
        throw std::runtime_error(std::string("Ungültiger Wert: ")+key);
    return int(n);
}
std::string name(const QJsonObject& o) {
    auto v=o.value("name");
    if(!v.isString()||v.toString().trimmed().isEmpty()||v.toString().size()>80)
        throw std::runtime_error("Ein Name fehlt oder ist zu lang.");
    return v.toString().trimmed().toStdString();
}
karton::Size dimensions(const QJsonValue& v) {
    if(!v.isObject())throw std::runtime_error("Maße fehlen.");auto o=v.toObject();
    return {integer(o,"length_mm",1,5000),integer(o,"width_mm",1,5000),integer(o,"height_mm",1,5000)};
}
QString quoted(QString value) {
    // Do not turn user-supplied article labels into spreadsheet formulas.
    QString trimmed=value.trimmed();
    if(!trimmed.isEmpty()&&QString("=+-@").contains(trimmed.front()))value.prepend('\'');
    value.replace('"',"\"\"");return '"'+value+'"';
}
bool write_atomic(const QString& filename,const QByteArray& bytes,QString& error) {
    QSaveFile file(filename);
    if(!file.open(QIODevice::WriteOnly)||file.write(bytes)!=bytes.size()||!file.commit()) {
        error="Datei konnte nicht gespeichert werden: "+file.errorString();return false;
    }
    return true;
}
}

QString number_text(double n,int decimals) {return QLocale(QLocale::German,QLocale::Germany).toString(n,'f',decimals);}
QString size_text(karton::Size s) {
    return number_text(s.length/10.0)+" × "+number_text(s.width/10.0)+" × "+number_text(s.height/10.0)+" cm";
}
QJsonDocument project_json(const Project& p) {
    QJsonArray articles,cartons;
    for(const auto& a:p.articles)articles.append(QJsonObject{{"name",QString::fromStdString(a.name)},
        {"size",size_json(a.size)},{"weight_g",a.weight_g},{"quantity",a.quantity}});
    for(const auto& c:p.cartons)cartons.append(QJsonObject{{"name",QString::fromStdString(c.name)},
        {"inside",size_json(c.inside)},{"max_payload_g",c.max_payload_g}});
    return QJsonDocument(QJsonObject{{"format","kartonplaner"},{"version",1},{"articles",articles},{"cartons",cartons},
        {"settings",QJsonObject{{"article_padding_mm",p.settings.article_padding_mm},{"wall_margin_mm",p.settings.wall_margin_mm}}}});
}
bool read_project(const QJsonDocument& document,Project& p,QString& error) {
    try {
        if(!document.isObject())throw std::runtime_error("Die Datei enthält keinen gültigen Auftrag.");
        auto root=document.object();
        if(root.value("format").toString()!="kartonplaner"||integer(root,"version",1,1)!=1)
            throw std::runtime_error("Dieses Dateiformat wird nicht unterstützt.");
        if(!root.value("articles").isArray()||!root.value("cartons").isArray()||!root.value("settings").isObject())
            throw std::runtime_error("Artikelliste, Kartons oder Einstellungen fehlen.");
        Project next;next.articles.clear();next.cartons.clear();
        auto aa=root.value("articles").toArray(),cc=root.value("cartons").toArray();
        if(aa.size()>80||cc.size()>40)throw std::runtime_error("Zu viele Artikel oder Kartons.");
        for(auto value:aa) {
            if(!value.isObject())throw std::runtime_error("Ungültiger Artikel.");auto a=value.toObject();
            next.articles.push_back({name(a),dimensions(a.value("size")),integer(a,"weight_g",0,1000000),integer(a,"quantity",1,80)});
        }
        for(auto value:cc) {
            if(!value.isObject())throw std::runtime_error("Ungültiger Karton.");auto c=value.toObject();
            next.cartons.push_back({name(c),dimensions(c.value("inside")),integer(c,"max_payload_g",1,1000000)});
        }
        auto settings=root.value("settings").toObject();
        next.settings.article_padding_mm=integer(settings,"article_padding_mm",0,200);
        next.settings.wall_margin_mm=integer(settings,"wall_margin_mm",0,200);
        auto to_validate=next.articles;
        if(to_validate.empty())to_validate.push_back({"Validierung",{1,1,1},0,1});
        auto problem=karton::input_error(to_validate,next.cartons,next.settings);
        if(!problem.empty())throw std::runtime_error(problem);
        p=std::move(next);return true;
    }catch(const std::exception& e){error=QString::fromUtf8(e.what());return false;}
}
bool load_project(const QString& filename,Project& p,QString& error) {
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)){error="Datei konnte nicht geöffnet werden: "+file.errorString();return false;}
    if(file.size()>2*1024*1024){error="Die Auftragsdatei ist zu groß (maximal 2 MB).";return false;}
    QJsonParseError parse;auto doc=QJsonDocument::fromJson(file.readAll(),&parse);
    if(parse.error!=QJsonParseError::NoError){error="Ungültiges JSON: "+parse.errorString();return false;}
    return read_project(doc,p,error);
}
bool save_project(const QString& filename,const Project& p,QString& error) {
    auto document=project_json(p);Project checked;
    if(!read_project(document,checked,error))return false;
    return write_atomic(filename,document.toJson(QJsonDocument::Indented),error);
}
bool export_plan(const QString& filename,const Project& project,const karton::Candidate& candidate,QString& error) {
    auto problem=karton::validate_plan(project.articles,candidate,project.settings);
    if(!problem.empty()){error=QString::fromStdString(problem);return false;}
    QString text; // BOM is prepended to the UTF-8 bytes below.
    text+="Schritt;Artikel;Stueck;Laenge_mm;Breite_mm;Hoehe_mm;X_mm;Y_mm;Z_mm;Karton;Polster_je_Seite_mm;Randreserve_mm\r\n";
    int i=0;
    for(const auto& p:candidate.placements) {
        text+=QString::number(++i)+';'+quoted(QString::fromStdString(project.articles[p.article_index].name))+';'+
            QString::number(p.copy_number)+';'+QString::number(p.rotated.length)+';'+QString::number(p.rotated.width)+';'+
            QString::number(p.rotated.height)+';'+QString::number(p.x)+';'+QString::number(p.y)+';'+QString::number(p.z)+';'+
            quoted(QString::fromStdString(candidate.carton.name))+';'+QString::number(project.settings.article_padding_mm)+';'+
            QString::number(project.settings.wall_margin_mm)+"\r\n";
    }
    return write_atomic(filename,QByteArray::fromHex("efbbbf")+text.toUtf8(),error);
}
