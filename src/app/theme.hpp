#pragma once
#include <QColor>
#include <QPalette>
#include <QString>

struct Theme {
    QColor page,surface,input,hover,border,text,muted,accent,selection,selected_text,disabled,
           plot,dots,floor,edge;
};
Theme theme_colors(bool dark);
QPalette theme_palette(bool dark);
QString theme_stylesheet(bool dark);
