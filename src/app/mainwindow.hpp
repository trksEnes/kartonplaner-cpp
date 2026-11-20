#pragma once
#include "project.hpp"
#include "packview.hpp"
#include <QMainWindow>
#include <QFutureWatcher>
#include <memory>
#include <optional>

class QTableWidget;
class QLabel;
class QPushButton;
class QDoubleSpinBox;
class QSlider;
class QCloseEvent;

class MainWindow : public QMainWindow {
public:
    MainWindow();
    ~MainWindow() override;
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    Project project_;
    karton::Result result_;
    QFutureWatcher<karton::Result> watcher_;
    std::shared_ptr<std::atomic_bool> cancel_;
    int current_candidate_=-1;
    bool busy_=false,dirty_=false,loading_=false,dark_=false;
    QString filename_;

    QTableWidget *articles_=nullptr,*candidates_=nullptr,*sequence_=nullptr;
    QWidget *input_panel_=nullptr;
    PackView* view_=nullptr;
    QDoubleSpinBox *padding_=nullptr,*margin_=nullptr;
    QLabel *order_summary_=nullptr,*recommendation_=nullptr,*result_caption_=nullptr,
           *fill_=nullptr,*weight_=nullptr,*step_label_=nullptr,*state_=nullptr,*view_caption_=nullptr,*result_title_=nullptr;
    QPushButton *calculate_=nullptr,*export_=nullptr,*save_=nullptr,*open_=nullptr,*theme_toggle_=nullptr;
    QSlider* steps_=nullptr;

    void build_ui();
    void apply_theme(bool dark,bool persist=false);
    void render_articles();
    void invalidate();
    void update_order_summary();
    void edit_article(int row=-1);
    void edit_cartons();
    void calculate();
    void render_result();
    void show_candidate(int row);
    void load_example();
    void clear_order();
    void open_project();
    bool save_project_as(bool force_dialog=false);
    bool confirm_discard();
    void export_current();
    void explain();
};
