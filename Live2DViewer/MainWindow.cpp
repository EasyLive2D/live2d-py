#include "MainWindow.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "Live2DView.hpp"


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    ui.setupUi(this);


    connect(ui.tabs, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);

    connect(ui.actionOpen, &QAction::triggered, this, &MainWindow::onOpenModel);
    connect(ui.clbOpen, &QCommandLinkButton::clicked, this, &MainWindow::onOpenModel);
    connect(ui.actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
    connect(ui.actionHelp, &QAction::triggered, this, &MainWindow::showHelp);

    ui.welcomeIcon->setPixmap(QPixmap("./moeroid.ico"));
}

MainWindow::~MainWindow()
{
}

void MainWindow::onTabCloseRequested(int index)
{
    QWidget *widget = ui.tabs->widget(index);

    ui.tabs->removeTab(index);

    delete widget;
}

void MainWindow::onOpenModel()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("打开模型"), "../../Resources/v3", tr("Model Json (*.model3.json *.model.json)"));

    if (fileName.isEmpty())
        return;

    Live2DView *scene = new Live2DView(fileName, this);

    QFileInfo fileInfo(fileName);
    ui.tabs->addTab(scene, fileInfo.baseName());
    ui.tabs->setCurrentWidget(scene);
}

void MainWindow::showAbout()
{
    QMessageBox::information(this, tr("帮助"), tr(R"(
            <html>
                <head>
                    <style>
                        body {
                            font-family: "Microsoft YaHei";
                        }
                    </style>
                </head>
                <body>
                    <p>
                        <a href="https://github.com/Arkueid/live2d-py/">Live2DViewer</a> 是一个 Live2D 模型查看器，支持 Cubism 3.0 以上版本的模型。
                    </p>
                    <p>
                        本程序基于 Qt 6.8.2，使用 C++17 编写。
                    </p>
                    <p>
                        本程序使用 <a href="https://github.com/Live2D/">Live2D Cubism SDK</a>。
                    </p>
                </body>
            </html>
        )"));
}

void MainWindow::showHelp()
{
    QMessageBox::information(this, tr("帮助"), tr(R"(
        <html>
            <head>
                <style>
                    body {
                        font-family: "Microsoft YaHei";
                    }
                </style>
            </head>
            <body>
                <h1>Live2DViewer 帮助</h1>
                <p>
                    点击画布后：
                </p>
                <ul>
                    <li>
                        <p>
                            通过上下左右键移动模型
                        </p>
                    </li>
                    <li>
                        <p>
                            通过减号键缩小、等号键放大模型
                        </p>
                    </li>
                    <li>
                        <p>
                            右键唤出菜单，清空选中状态
                        </p>
                    </li>
                </ul>
                <p>
                    部件面板和图形网格面板可点击对应行来选择模型的绘制区域
                </p>
            </body>
        </html>
    )"));
}