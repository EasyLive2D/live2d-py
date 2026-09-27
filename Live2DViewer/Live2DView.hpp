#pragma once

#include <IModel.hpp>

#include "scene/Live2DScene.hpp"
#include "ui_Live2DView.h"

#include <QJsonObject>
#include <QTimer>


class Live2DView : public QWidget
{
    Q_OBJECT

    void initExpressions(Live2D::IModel *model);
    void initMotions(Live2D::IModel *model);

    void initCdi(Live2D::IModel *model);

    void initParameters(Live2D::IModel *model);
    void initParts(Live2D::IModel *model);
    void initDrawables(Live2D::IModel *model);

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

    ModelHolder* holder;
    int selectedPartIndex;

    QTimer syncTimer;
};
