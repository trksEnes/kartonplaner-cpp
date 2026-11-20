#pragma once
#include "kartonplaner/packing.hpp"
#include <QString>
#include <QJsonDocument>

struct Project {
    std::vector<karton::Article> articles = karton::example_order();
    std::vector<karton::Carton> cartons = karton::example_cartons();
    karton::Settings settings;
};

QJsonDocument project_json(const Project& project);
bool read_project(const QJsonDocument& document, Project& project, QString& error);
bool load_project(const QString& filename, Project& project, QString& error);
bool save_project(const QString& filename, const Project& project, QString& error);
bool export_plan(const QString& filename, const Project& project,
                 const karton::Candidate& candidate, QString& error);
QString size_text(karton::Size size);
QString number_text(double value, int decimals = 1);
