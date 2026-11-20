#include "theme.hpp"
#include <utility>

Theme theme_colors(bool dark) {
    if(dark)return {QColor("#101A22"),QColor("#192831"),QColor("#12212A"),QColor("#243943"),
        QColor("#354B57"),QColor("#E5EFF3"),QColor("#A5B9C5"),QColor("#74D9C6"),
        QColor("#254D4C"),QColor("#E2FFF6"),QColor("#778E9B"),QColor("#12212B"),
        QColor("#20323E"),QColor("#293F4D"),QColor("#8CA9BB")};
    return {QColor("#EDF2F6"),QColor("#FFFFFF"),QColor("#F5F8FA"),QColor("#EAF2F5"),
        QColor("#CFDCE4"),QColor("#233A49"),QColor("#5F7585"),QColor("#176D67"),
        QColor("#DDEFE9"),QColor("#153F43"),QColor("#8295A2"),QColor("#F4F7FA"),
        QColor("#E5EBF0"),QColor("#DFE7ED"),QColor("#637C8D")};
}
QPalette theme_palette(bool dark) {
    const auto t=theme_colors(dark);QPalette p;
    p.setColor(QPalette::Window,t.page);p.setColor(QPalette::WindowText,t.text);
    p.setColor(QPalette::Base,t.surface);p.setColor(QPalette::AlternateBase,t.input);
    p.setColor(QPalette::Text,t.text);p.setColor(QPalette::Button,t.surface);p.setColor(QPalette::ButtonText,t.text);
    p.setColor(QPalette::Highlight,t.selection);p.setColor(QPalette::HighlightedText,t.selected_text);
    p.setColor(QPalette::ToolTipBase,t.surface);p.setColor(QPalette::ToolTipText,t.text);
    p.setColor(QPalette::PlaceholderText,t.muted);p.setColor(QPalette::Link,t.accent);
    p.setColor(QPalette::Light,t.border.lighter());p.setColor(QPalette::Midlight,t.hover);
    p.setColor(QPalette::Mid,t.border);p.setColor(QPalette::Dark,t.border.darker());p.setColor(QPalette::Shadow,t.page);
    for(auto role:{QPalette::Text,QPalette::WindowText,QPalette::ButtonText})p.setColor(QPalette::Disabled,role,t.disabled);
    p.setColor(QPalette::Disabled,QPalette::Button,t.input);
    return p;
}
QString theme_stylesheet(bool dark) {
    const auto t=theme_colors(dark);
    QString css=QString::fromUtf8(R"(
        QMainWindow { background:{page}; }
        QWidget { font-family:"Helvetica Neue","Segoe UI",sans-serif; font-size:13px; color:{text}; }
        QFrame#header { background:{surface}; border-bottom:1px solid {border}; }
        QFrame#card { background:{surface}; border:1px solid {border}; border-radius:13px; }
        QLabel { background:transparent; }
        QLabel#brand { font-size:24px; font-weight:750; }
        QLabel#brandMark { font-size:34px; color:{accent}; padding-right:5px; }
        QLabel#subtle { color:{muted}; font-size:12px; }
        QLabel#sectionTitle { font-size:17px; font-weight:700; }
        QLabel#fieldLabel { font-size:12px; color:{muted}; font-weight:600; }
        QLabel#big { font-size:27px; font-weight:750; color:{accent}; }
        QLabel#metric { font-size:24px; font-weight:650; }
        QLabel#badge { background:{selection}; color:{accent}; padding:7px 11px; border-radius:12px; font-size:11px; font-weight:700; }
        QPushButton { background:{surface}; border:1px solid {border}; border-radius:7px; padding:9px 13px; font-weight:600; min-height:16px; }
        QPushButton:hover { background:{hover}; border-color:{muted}; }
        QPushButton:pressed { background:{selection}; }
        QPushButton:disabled { color:{disabled}; background:{input}; border-color:{border}; }
        QPushButton:focus { border:1px solid {accent}; }
        QPushButton#primary { background:#187F77; border-color:#187F77; color:#FFFFFF; padding:12px 18px; }
        QPushButton#primary:hover { background:#146D66; }
        QPushButton#primary:disabled { background:{hover}; border-color:{border}; color:{disabled}; }
        QPushButton#quiet { border:none; background:transparent; padding:5px 7px; color:{muted}; }
        QPushButton#quiet:hover { color:{text}; background:{hover}; }
        QPushButton#themeToggle { border-radius:17px; padding:8px 12px; }
        QLineEdit,QDoubleSpinBox,QSpinBox,QComboBox { background:{input}; color:{text}; border:1px solid {border}; border-radius:6px; padding:5px 7px; selection-background-color:{selection}; selection-color:{selected_text}; }
        QLineEdit:focus,QDoubleSpinBox:focus,QSpinBox:focus,QComboBox:focus { border:1px solid {accent}; }
        QComboBox { padding-right:23px; }
        QComboBox::drop-down { border:none; width:22px; }
        QComboBox QAbstractItemView { background:{surface}; color:{text}; border:1px solid {border}; selection-background-color:{selection}; selection-color:{selected_text}; }
        QTableWidget { background:{surface}; border:0; selection-background-color:{selection}; selection-color:{selected_text}; }
        QTableWidget::item { padding:6px; border-bottom:1px solid {border}; }
        QHeaderView::section { background:{input}; color:{muted}; border:0; border-bottom:1px solid {border}; padding:9px 7px; font-size:11px; font-weight:650; }
        QTabWidget::pane { border:0; background:{surface}; }
        QTabBar::tab { padding:12px 17px; margin-right:4px; color:{muted}; border-bottom:2px solid transparent; }
        QTabBar::tab:selected { color:{accent}; border-bottom:2px solid {accent}; font-weight:650; }
        QTabBar::tab:hover { color:{accent}; }
        QToolTip { background:{surface}; color:{text}; padding:7px; border:1px solid {border}; }
        QStatusBar { color:{muted}; background:{page}; font-size:11px; }
        QSlider::groove:horizontal { height:5px; background:{border}; border-radius:2px; }
        QSlider::sub-page:horizontal { background:#399D90; border-radius:2px; }
        QSlider::handle:horizontal { background:{accent}; width:14px; margin:-5px 0; border-radius:7px; }
        QDialog,QMessageBox { background:{page}; }
        QCheckBox { spacing:8px; }
        QMenuBar,QMenu { background:{surface}; color:{text}; }
        QMenuBar::item:selected,QMenu::item:selected { background:{selection}; color:{selected_text}; }
        QMenu::separator { background:{border}; height:1px; margin:4px 8px; }
        QScrollBar:vertical { background:{input}; width:10px; margin:0; }
        QScrollBar::handle:vertical { background:{border}; border-radius:4px; min-height:24px; }
        QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
    )");
    for(const auto& entry:{std::pair{"page",t.page}, {"surface",t.surface},{"input",t.input},{"hover",t.hover},
        {"border",t.border},{"text",t.text},{"muted",t.muted},{"accent",t.accent},{"selection",t.selection},
        {"selected_text",t.selected_text},{"disabled",t.disabled}})
        css.replace("{"+QString::fromLatin1(entry.first)+"}",entry.second.name());
    return css;
}
