#include "Live2DView.hpp"
#include "scene/Live2DScene.hpp"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QSlider>
#include <QTreeWidgetItem>


using namespace Live2D::V3;

Live2DView::Live2DView(const QString& filePath, QWidget* parent)
    : QWidget(parent)
    , holer(ui.scene->GetModel())
    , hasCdi(false)
    , syncTimer(this)
    , selectedPartIndex(-1)
{
    ui.setupUi(this);

    ui.splitter->setStretchFactor(0, 0);
    ui.splitter->setStretchFactor(1, 1);

    ui.scene->LoadModel(filePath);

    if (holer.version == Version::V3) {
        initExpressions(holer.model3);
        initMotions(holer.model3);

        initCdi(holer.model3);

        initParameters(holer.model3);
        initParts(holer.model3);
        initDrawables(holer.model3);
    }

    connect(
        ui.treeWidget, &QTreeWidget::itemDoubleClicked, this, &Live2DView::onTreeItemDoubleClicked);

    connect(ui.autoBreath, &QCheckBox::checkStateChanged, [&](Qt::CheckState state) {
        ui.scene->setAutoBreath(state == Qt::Checked);
    });
    connect(ui.autoBlink, &QCheckBox::checkStateChanged, [&](Qt::CheckState state) {
        ui.scene->setAutoBlink(state == Qt::Checked);
    });
    connect(ui.autoPhysics, &QCheckBox::checkStateChanged, [&](Qt::CheckState state) {
        ui.scene->setAutoPhysics(state == Qt::Checked);
    });

    connect(ui.partTable, &QTableWidget::itemClicked, this, &Live2DView::onPartTableItemClicked);

    connect(
        ui.drawableList, &QListWidget::itemClicked, this, &Live2DView::onDrawableListItemClicked);

    connect(ui.scene, &Live2DScene::clearSelection, this, &Live2DView::onClearSelection);

    connect(ui.scene, &Live2DScene::paramValuesUpdated, this, &Live2DView::onParamValuesUpdated);

    ui.pages->setCurrentIndex(0);
}

Live2DView::~Live2DView() {}

void Live2DView::initExpressions(Model* model)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(ui.treeWidget);
    item->setText(0, "Expressions");

    model->GetExpressions(item, [](void* collector, const char* id, const char* file) {
        QTreeWidgetItem* topLevel = (QTreeWidgetItem*)collector;
        QTreeWidgetItem* item = new QTreeWidgetItem(topLevel);
        QFileInfo info(file);
        item->setText(0, info.fileName());
        item->setData(0, Qt::UserRole, QVariant(id));
    });
}

void Live2DView::initMotions(Model* model)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(ui.treeWidget);
    item->setText(0, "Motions");
    ui.treeWidget->addTopLevelItem(item);

    QMap<QString, QTreeWidgetItem*> groups;
    void* collector[2] = {item, &groups};
    model->GetMotions(
        collector, [](void* collector, const char* group, int no, const char* file, const char*) {
            QTreeWidgetItem* topLevel = (QTreeWidgetItem*)(((void**)collector)[0]);
            QMap<QString, QTreeWidgetItem*>* groups =
                (QMap<QString, QTreeWidgetItem*>*)(((void**)collector)[1]);
            QTreeWidgetItem* groupItem;
            if (!groups->contains(group)) {
                groupItem = new QTreeWidgetItem(topLevel);
                groupItem->setText(0, group);
                groups->insert(group, groupItem);
            }
            groupItem = groups->value(group);
            QTreeWidgetItem* item = new QTreeWidgetItem(groupItem);
            QFileInfo info(file);
            item->setText(0, info.fileName());
            item->setData(0, Qt::UserRole, QVariant(group));
            item->setData(0, Qt::UserRole + 1, QVariant(no));
        });
}

void Live2DView::onTreeItemDoubleClicked(QTreeWidgetItem* item, int column)
{
    if (holer.version == Version::V3) {
        auto model = holer.model3;
        if (item->parent() == nullptr) {
            model->ResetExpression();
            return;
        }

        if (item->parent()->text(0) == "Expressions") {
            model->SetExpression(item->data(0, Qt::UserRole).toString().toStdString().c_str());
        } else if (item->parent()->parent() != nullptr &&
                   item->parent()->parent()->text(0) == "Motions") {
            model->StartMotion(item->data(0, Qt::UserRole).toString().toStdString().c_str(),
                               item->data(0, Qt::UserRole + 1).toInt());
        }
    }
}

void Live2DView::initCdi(Model* model)
{
    QDir dir(model->GetModelHomeDir());
    QStringList files = dir.entryList(QStringList() << "*.cdi3.json", QDir::Files);
    if (files.size() > 0) {
        hasCdi = true;
        QFile file(dir.absoluteFilePath(files[0]));
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            cdi = doc.object();
            file.close();
        }
    }
}

void Live2DView::initParameters(Model* model)
{
    void* ptrs[4] = {ui.paramTable, nullptr, model};
    QJsonArray parameters;
    if (hasCdi) {
        parameters = cdi["Parameters"].toArray();
        ptrs[1] = &parameters;
    }
    model->GetParameterIds(ptrs, [](void* collector, const char* id) {
        QTableWidget* table = (QTableWidget*)(((void**)collector)[0]);
        QJsonArray* cdiParams = (QJsonArray*)(((void**)collector)[1]);
        Model* model = (Model*)(((void**)collector)[2]);

        QString name = id;
        const int index = table->rowCount();
        if (cdiParams) {
            name = cdiParams->at(index).toObject()["Name"].toString();
        }
        float maxValue = model->GetParameterMaximumValue(index);
        float minValue = model->GetParameterMinimumValue(index);
        float defaultValue = model->GetParameterDefaultValue(index);

        table->insertRow(index);
        QTableWidgetItem* idItem = new QTableWidgetItem(id);
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        QTableWidgetItem* valueItem = new QTableWidgetItem(QString::number(defaultValue));
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        QSlider* slider = new QSlider(Qt::Horizontal, table);
        slider->setRange(0, 100);
        slider->setValue((defaultValue - minValue) * 100 / (maxValue - minValue));
        QObject::connect(slider, &QSlider::valueChanged, [=](int value) {
            const float v = minValue + (maxValue - minValue) * value / 100;
            valueItem->setText(QString::number(v, 'f', 2));
            if (slider->isSliderDown()) {
                model->SetAndSaveParameterValue(index, v);
            }
        });
        table->setItem(index, 0, idItem);
        table->setItem(index, 1, nameItem);
        table->setCellWidget(index, 2, slider);
        table->setItem(index, 3, valueItem);
    });
}

void Live2DView::initParts(Model* model)
{
    void* ptrs[2] = {ui.partTable, nullptr};
    QJsonArray parts;
    if (hasCdi) {
        parts = cdi["Parts"].toArray();
        ptrs[1] = &parts;
    }
    model->GetPartIds(ptrs, [](void* collector, const char* id) {
        QTableWidget* table = (QTableWidget*)(((void**)collector)[0]);
        QJsonArray* cdiParts = (QJsonArray*)(((void**)collector)[1]);
        QString name = id;
        const int index = table->rowCount();
        if (cdiParts) {
            name = cdiParts->at(index).toObject()["Name"].toString();
        }
        table->insertRow(index);
        QTableWidgetItem* idItem = new QTableWidgetItem(id);
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        QTableWidgetItem* nameItem = new QTableWidgetItem(name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(index, 0, idItem);
        table->setItem(index, 1, nameItem);
    });
}

void Live2DView::initDrawables(Model* model)
{
    model->GetDrawableIds(ui.drawableList, [](void* collector, const char* id) {
        QListWidget* list = (QListWidget*)collector;
        list->addItem(id);
    });
}

void Live2DView::onParamValuesUpdated()
{
    if (holer.version == Version::V3) {
        auto model = holer.model3;
        for (int i = 0; i < ui.paramTable->rowCount(); i++) {
            QSlider* slider = (QSlider*)ui.paramTable->cellWidget(i, 2);
            if (slider->isSliderDown()) {
                return;
            }
            slider->setValue(
                100 * (model->GetParameterValue(i) - model->GetParameterMinimumValue(i)) /
                (model->GetParameterMaximumValue(i) - model->GetParameterMinimumValue(i)));
        }
    }
}

void Live2DView::onPartTableItemClicked(QTableWidgetItem* item)
{
    if (holer.version == Version::V3) {
        auto model = holer.model3;
        if (selectedPartIndex != -1) {
            model->SetPartMultiplyColor(selectedPartIndex, 1.0f, 1.0f, 1.0f, 1.0f);
        }
        const int row = item->row();
        selectedPartIndex = row;
        model->SetPartMultiplyColor(row, 0.2f, 0.2f, 1.0f, 0.8f);
    }
}

void Live2DView::onDrawableListItemClicked(QListWidgetItem* item)
{
    if (holer.version == Version::V3) {
        auto model = holer.model3;
        const int index = ui.drawableList->row(item);

        ui.scene->selectDrawable(index);
    }
}

void Live2DView::onClearSelection()
{
    if (holer.version == Version::V3) {
        auto model = holer.model3;
        if (selectedPartIndex != -1) {
            model->SetPartMultiplyColor(selectedPartIndex, 1.0f, 1.0f, 1.0f, 1.0f);
        }

        selectedPartIndex = -1;
        ui.partTable->clearSelection();
        ui.drawableList->clearSelection();
    }
}
