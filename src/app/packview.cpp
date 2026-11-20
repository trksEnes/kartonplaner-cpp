#include "packview.hpp"
#include "project.hpp"
#include "theme.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QPolygonF>
#include <algorithm>
#include <array>
#include <cmath>

QColor article_color(int i) {
    static const std::array<const char*,8> colors{"#258B86","#DD9856","#657DC4","#CA758F","#7F9B57","#8B72B5","#4789A8","#BF7960"};
    return QColor(colors[std::size_t(std::max(0,i))%colors.size()]);
}
PackView::PackView(QWidget* parent):QWidget(parent) {
    setMinimumSize(350,265);setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    setAccessibleName("Räumliche Packansicht");
}
void PackView::show_plan(const karton::Candidate& c,int padding){plan_=c;padding_=padding;step_=int(c.placements.size());highlight_=-1;update();}
void PackView::clear_plan(){plan_.reset();update();}
void PackView::set_top_view(bool t){top_=t;update();}
void PackView::set_step(int s){step_=s;update();}
void PackView::rotate_view(){rotation_=(rotation_+1)%4;update();}
void PackView::highlight(int i){highlight_=i;update();}
void PackView::set_dark(bool dark){dark_=dark;update();}
void PackView::paintEvent(QPaintEvent*) {
    const auto t=theme_colors(dark_);
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.fillRect(rect(),t.plot);
    p.setPen(QPen(t.dots,1));
    for(int x=20;x<width();x+=28)for(int y=20;y<height();y+=28)p.drawPoint(x,y);
    if(!plan_) {
        p.setPen(t.muted);QFont font=p.font();font.setPointSize(13);p.setFont(font);
        p.drawText(rect().adjusted(25,30,-25,-30),Qt::AlignCenter|Qt::TextWordWrap,
                   "Dein Packplan erscheint hier.\nFüge Artikel hinzu und starte die Berechnung.");
        return;
    }
    const auto& box=plan_->carton.inside;
    auto rotate=[&](double x,double y){
        if(rotation_==1)return QPointF(box.width-y,x);
        if(rotation_==2)return QPointF(box.length-x,box.width-y);
        if(rotation_==3)return QPointF(y,box.length-x);
        return QPointF(x,y);
    };
    auto raw=[&](double x,double y,double z){auto q=rotate(x,y);return top_?QPointF(q.x(),q.y()):QPointF((q.x()-q.y())*0.866,(q.x()+q.y())*0.45-z*0.88);};
    QRectF bounds;
    bool first=true;
    for(int x:{0,box.length})for(int y:{0,box.width})for(int z:{0,box.height}) {
        auto q=raw(x,y,z);
        if(first){bounds=QRectF(q,QSizeF(0,0));first=false;}
        else {bounds.setLeft(std::min(bounds.left(),q.x()));bounds.setRight(std::max(bounds.right(),q.x()));
              bounds.setTop(std::min(bounds.top(),q.y()));bounds.setBottom(std::max(bounds.bottom(),q.y()));}
    }
    double scale=std::min((width()-110)/std::max(1.0,bounds.width()),(height()-95)/std::max(1.0,bounds.height()));
    QPointF origin(width()/2.0-bounds.center().x()*scale,height()/2.0-bounds.center().y()*scale-7);
    auto point=[&](double x,double y,double z){return raw(x,y,z)*scale+origin;};
    auto polygon=[&](std::initializer_list<QPointF> points){QPolygonF poly;for(auto q:points)poly<<q;return poly;};
    auto floor=polygon({point(0,0,0),point(box.length,0,0),point(box.length,box.width,0),point(0,box.width,0)});
    p.setPen(QPen(t.edge,1.3));p.setBrush(t.floor);p.drawPolygon(floor);
    if(!top_) {
        QColor glass=t.edge;glass.setAlpha(dark_?28:35);p.setBrush(glass);
        double back_x=(rotation_==0||rotation_==1)?0:box.length;
        double back_y=(rotation_==0||rotation_==3)?0:box.width;
        p.drawPolygon(polygon({point(0,back_y,0),point(box.length,back_y,0),point(box.length,back_y,box.height),point(0,back_y,box.height)}));
        p.drawPolygon(polygon({point(back_x,0,0),point(back_x,box.width,0),point(back_x,box.width,box.height),point(back_x,0,box.height)}));
    }
    std::vector<int> order;
    for(int i=0;i<std::min(step_,int(plan_->placements.size()));++i)order.push_back(i);
    std::stable_sort(order.begin(),order.end(),[&](int a,int b) {
        auto depth=[&](const karton::Placement& s){auto q=rotate(s.x+s.occupied.length/2.0,s.y+s.occupied.width/2.0);return top_?double(s.z):q.x()+q.y()+s.z+s.occupied.height/2.0;};
        return depth(plan_->placements[a])<depth(plan_->placements[b]);
    });
    for(int index:order) {
        const auto& a=plan_->placements[index];
        // Draw padded cuboids: the plan must reserve the visible occupied extent.
        double x=a.x,y=a.y,z=a.z,l=a.occupied.length,w=a.occupied.width,h=a.occupied.height;
        QColor color=article_color(a.article_index);bool selected=highlight_<0||highlight_==a.article_index;
        if(!selected)color.setAlpha(65);
        p.setPen(QPen(selected?color.darker(140):QColor("#ACBAC5"),highlight_==a.article_index?2.4:1.1));
        if(!top_) {
            double fx=(rotation_==0||rotation_==1)?x+l:x;
            double fy=(rotation_==0||rotation_==3)?y+w:y;
            p.setBrush(color.darker(112));p.drawPolygon(polygon({point(fx,y,z),point(fx,y+w,z),point(fx,y+w,z+h),point(fx,y,z+h)}));
            p.setBrush(color.darker(132));p.drawPolygon(polygon({point(x,fy,z),point(x+l,fy,z),point(x+l,fy,z+h),point(x,fy,z+h)}));
        }
        auto top=polygon({point(x,y,z+h),point(x+l,y,z+h),point(x+l,y+w,z+h),point(x,y+w,z+h)});
        p.setBrush(color.lighter(125));p.drawPolygon(top);
        if(selected&&top.boundingRect().width()>19&&top.boundingRect().height()>13) {
            p.setPen(QColor("#163C43"));QFont f=p.font();f.setPointSize(10);f.setBold(true);p.setFont(f);
            p.drawText(top.boundingRect(),Qt::AlignCenter,QString::number(index+1));
        }
    }
    // Front carton edges remain visible, without covering articles with a solid wall.
    p.setBrush(Qt::NoBrush);p.setPen(QPen(t.edge,1.4,Qt::DashLine));
    auto rim=polygon({point(0,0,box.height),point(box.length,0,box.height),point(box.length,box.width,box.height),point(0,box.width,box.height)});
    p.drawPolygon(rim);
    if(!top_)for(auto q:std::array<QPointF,4>{QPointF(0,0),QPointF(box.length,0),QPointF(box.length,box.width),QPointF(0,box.width)})
        p.drawLine(point(q.x(),q.y(),0),point(q.x(),q.y(),box.height));
    p.setPen(t.muted);QFont f=p.font();f.setPointSize(10);f.setBold(false);p.setFont(f);
    p.drawText(QRectF(12,height()-36,width()-24,23),Qt::AlignCenter,
               size_text(box)+"  ·  "+(padding_?"inkl. Artikelpolster":"ohne Artikelpolster"));
}
