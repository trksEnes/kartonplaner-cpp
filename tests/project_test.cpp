#include "project.hpp"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <iostream>
#include <stdexcept>

int checks=0;
void check(bool ok,const char* what){++checks;if(!ok)throw std::runtime_error(what);}
int main(int argc,char** argv){
    QCoreApplication app(argc,argv);
    try {
        Project original,loaded;QString error;QTemporaryDir temp;
        check(temp.isValid(),"temporary directory");
        check(read_project(project_json(original),loaded,error),"round trip");
        check(project_json(original)==project_json(loaded),"round trip retains all settings");
        check(save_project(temp.path()+"/order.json",original,error),"save");
        loaded.articles.clear();check(load_project(temp.path()+"/order.json",loaded,error),"load");
        check(project_json(original)==project_json(loaded),"file round trip");
        Project oversized=original;oversized.articles[0].quantity=80;
        check(!save_project(temp.path()+"/order.json",oversized,error),"invalid order not saved");
        check(load_project(temp.path()+"/order.json",loaded,error)&&project_json(original)==project_json(loaded),"failed save preserves original file");
        for(int mode=0;mode<6;++mode){
            auto bad=project_json(original).object();
            if(mode==0)bad["version"]=2;
            else if(mode==1)bad["articles"]=QString("no array");
            else if(mode==2)bad.remove("settings");
            else {
                auto aa=bad["articles"].toArray();auto a=aa[0].toObject();
                if(mode==3)a["quantity"]=1.5;
                if(mode==4)a["weight_g"]=-1;
                if(mode==5)a.remove("size");
                aa[0]=a;bad["articles"]=aa;
            }
            check(!read_project(QJsonDocument(bad),loaded,error),"invalid document rejected");
            check(project_json(loaded)==project_json(original),"failed load does not replace order");
        }
        Project empty=original;empty.articles.clear();
        check(read_project(project_json(empty),loaded,error)&&loaded.articles.empty(),"empty draft round trip");
        QFile invalid(temp.path()+"/invalid.json");invalid.open(QIODevice::WriteOnly);invalid.write("{bad");invalid.close();
        check(!load_project(invalid.fileName(),loaded,error),"malformed JSON rejected");
        original.articles[0].name="=SUM(A1:A3); \"Satz\"";
        auto result=karton::solve(original.articles,original.cartons,original.settings);
        check(result.recommended_index()>=0,"demo solved for export");
        auto candidate=result.candidates[result.recommended_index()];
        auto path=temp.path()+"/plan.csv";
        check(export_plan(path,original,candidate,error),"export valid plan");
        QFile csv(path);csv.open(QIODevice::ReadOnly);auto bytes=csv.readAll();csv.close();
        check(bytes.startsWith(QByteArray::fromHex("efbbbf")),"UTF8 BOM");
        check(bytes.contains("\"'=SUM(A1:A3); \"\"Satz\"\"\""),"spreadsheet formula protected and quoting escaped");
        check(bytes.count('\n')==result.article_count+1,"one row per piece");
        candidate.placements[0].x=-100;
        check(!export_plan(path,original,candidate,error),"invalid plan not exported");
        csv.open(QIODevice::ReadOnly);check(csv.readAll()==bytes,"failed export leaves old file intact");
        std::cout<<checks<<" Datei-Prüfungen bestanden.\n";return 0;
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
