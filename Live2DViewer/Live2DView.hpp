#pragma once

#include "scene/Live2DScene.hpp"
#include "ui_Live2DView.h"

#include <QJsonObject>
#include <QTimer>


class Live2DView : public QWidget
{
    Q_OBJECT

    void initExpressions(V3::Model *model);
    void initMotions(V3::Model *model);
    
    void initCdi(V3::Model *model);

    void initParameters(V3::Model *model);
    void initParts(V3::Model *model);
    void initDrawables(V3::Model *model);

private slots:
    void onTreeItemDoubleClicked(QTreeWidgetItem *item, int column);
    void onParamValuesUpdated();
    void onPartTableItemClicked(QTableWidgetItem *item);
    void onDrawableListItemClicked(QListWidgetItem* item);
    void onClearSelection();

public:
    Live2DView(const QString& filePath, QWidget *parent = nullptr);
    ~Live2DView() override;

private:
    Ui::Live2DView ui;

    bool hasCdi;
    QJsonObject cdi;

    const ModelHolder& holer;
    int selectedPartIndex;

    QTimer syncTimer;
};