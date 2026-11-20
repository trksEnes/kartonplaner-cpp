#pragma once
#include "kartonplaner/packing.hpp"
#include <QWidget>
#include <QColor>
#include <optional>

QColor article_color(int index);

class PackView : public QWidget {
public:
    explicit PackView(QWidget* parent=nullptr);
    void show_plan(const karton::Candidate& candidate,int padding);
    void clear_plan();
    void set_top_view(bool top);
    void set_step(int step);
    void rotate_view();
    void highlight(int article);
    void set_dark(bool dark);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::optional<karton::Candidate> plan_;
    int step_=0,padding_=0,rotation_=0,highlight_=-1;
    bool top_=false,dark_=false;
};
