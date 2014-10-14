/*
 *   Copyright (C) 2012  Romain Perier <romain.perier@labri.fr>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QtCore/QObject>
#include <QtCore/QDir>
#include <QtCore/QString>
#include <QtCore/QDebug>
#include <QtGui/QFileDialog>
#include <QtGui/QToolButton>
#include <QtGui/QWidgetAction>
#include <QtGui/QTreeWidget>
#include <QtDeclarative/QtDeclarative>
#include <QtGui/QAbstractSlider>

#include "mainwindow.h"
#include "eventnotifier.h"
#include "alarmnotifier.h"
#include "declarative/line.h"
#include "networktopology.h"
#include "sensormodel.h"
#include "coapinterface.h"
#include "fancybutton.h"
#include "view.h"
#include "helpers.h"
#include "sensormodel.h"
#include "bargraph.h"
#include "monitoringview.h"
#include "deploymentsettings.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
    , m_connectionDialog(new QDialog(this))
    , m_alarmnotifier(new AlarmNotifier(this))
    , m_sensorsModel(new SensorsModel(this))
{
    // UI
    ui.setupUi(this);
    ui_connection.setupUi(m_connectionDialog);

    // Views
    m_editView = new View(this, ui.declarativeView, true, false);
    m_editView->setModel(m_sensorsModel);
    m_deploymentView = new View(this, ui.deploymentDeclarativeView, false, true);
    m_deploymentView->setModel(m_sensorsModel);
    m_c2View = new View(this, ui.C2DeclarativeView);
    m_c2View->setModel(m_sensorsModel);
    m_currentView = m_deploymentView;

    m_monitoringView = new MonitoringView(this, ui.monitoringDeclarativeView);
    m_monitoringView->setModel(m_sensorsModel);

    m_deploymentSettings = new DeploymentSettings(ui.deploymentToolBar);
    m_deploymentSettings->setModel(m_sensorsModel);

    m_networktopology = new NetworkTopology(this, &ui);
    m_eventnotifier = new EventNotifier(this, &ui);

    // Fancy bar and menus
    initFancyBar();

    initFileMenu();
    initNetworkMenu();
    initDisplayMenu();

    showDeploymentView();
    loadExternalOverlays();

    connect(m_networktopology, SIGNAL(areaAdded(AreaItemModel*)), m_sensorsModel, SLOT(addArea(AreaItemModel*)));
    connect(m_networktopology, SIGNAL(nodeAdded(NodeItemModel*)), m_sensorsModel, SLOT(addNode(NodeItemModel*)));
    connect(m_networktopology, SIGNAL(sensorAdded(SensorItemModel*)), m_sensorsModel, SLOT(addSensor(SensorItemModel*)));
    connect(m_networktopology, SIGNAL(networkStarted()), m_eventnotifier, SLOT(connectToDispatcher()));
    connect(m_networktopology, SIGNAL(networkStarted()), m_alarmnotifier, SLOT(connectToServer()));
    connect(m_networktopology, SIGNAL(clear()), m_sensorsModel, SLOT(clear()));
    connect(m_networktopology, SIGNAL(coapGroupAdded(QString,QStringList)), m_monitoringView, SLOT(addCoapGroup(QString,QStringList)));
    connect(m_networktopology, SIGNAL(nodeAddedToCoapGroup(NodeItemModel*,QString)), m_monitoringView, SLOT(addNodeToCoapGroup(NodeItemModel*,QString)));

    connect(m_deploymentView, SIGNAL(itemCreated(QDeclarativeItem*)), SLOT(itemAddedToView(QDeclarativeItem*)));
    connect(m_deploymentView, SIGNAL(remove(QDeclarativeItem*)), SLOT(removeItem(QDeclarativeItem*)));
    connect(m_editView, SIGNAL(addedSensortoView(QDeclarativeItem*)), SLOT(sensorAddedToView(QDeclarativeItem*)));
    connect(m_alarmnotifier, SIGNAL(nodesAlarm(QList<int>,quint32)), SLOT(nodesAlarm(QList<int>,quint32)));
    connect(m_alarmnotifier, SIGNAL(nodeFailed(quint16)), SLOT(nodeFailed(quint16)));
    connect(m_sensorsModel, SIGNAL(itemPropertyChanged(ItemModel *,const char*,QVariant)), m_c2View,
           SLOT(itemModelPropertyChanged(ItemModel *,const char*,QVariant)));
    connect(m_sensorsModel, SIGNAL(itemPropertyChanged(ItemModel*,const char*,QVariant)), m_networktopology,
            SLOT(itemModelPropertyChanged(ItemModel*,const char*,QVariant)));
    connect(m_sensorsModel, SIGNAL(nodeAdded(NodeItemModel*)), m_monitoringView, SLOT(addNodeToListView(NodeItemModel*)));
    connect(m_deploymentView, SIGNAL(sensorItemClicked(SensorItemModel*)), m_deploymentSettings, SLOT(showSensorProperties(SensorItemModel*)));
    connect(m_deploymentView, SIGNAL(sensorItemClicked(SensorItemModel*)), m_deploymentSettings, SLOT(showSensorProperties(SensorItemModel*)));
    connect(m_deploymentView, SIGNAL(nodeItemClicked(NodeItemModel*)), m_deploymentSettings, SLOT(showNodeProperties(NodeItemModel*)));
}

void MainWindow::initFancyBar()
{
    ui.centralwidget->layout()->setMargin(0);
    ui.C2DockWidget->layout()->setMargin(0);
    ui.stackedWidget->widget(0)->layout()->setMargin(0);
    ui.stackedWidget->widget(1)->layout()->setMargin(0);
    ui.stackedWidget->widget(2)->layout()->setMargin(0);
    ui.stackedWidget->widget(3)->layout()->setMargin(0);

    m_deploymentViewButton = addFancyButton("Deployment");
    m_deploymentViewButton->setChecked(true);
    m_editViewButton = addFancyButton("Scenario");
    m_c2ViewButton = addFancyButton("C2");
    m_monitoringButton = addFancyButton("Monitoring");

    connect(m_editViewButton, SIGNAL(clicked()), SLOT(showEditView()));
    connect(m_deploymentViewButton, SIGNAL(clicked()), SLOT(showDeploymentView()));
    connect(m_c2ViewButton, SIGNAL(clicked()), SLOT(showC2View()));
    connect(m_monitoringButton, SIGNAL(clicked()), SLOT(showMonitoring()));
}

void MainWindow::initFileMenu()
{
    QAction *action;

    action = ui.menuFile->addAction("&Open scenario", this, SLOT(openScenario()));
    action->setObjectName("openScenario");
    action->setVisible(false);
    action = ui.menuFile->addAction("&Save scenario", this, SLOT(saveScenario()));
    action->setObjectName("saveScenario");
    action->setVisible(false);
    ui.menuFile->addAction("&Quit", qApp, SLOT(quit()));
}

void MainWindow::initNetworkMenu()
{
    ui.menuNetwork->addAction("&Connect to remote network", this, SLOT(connectToRemoteNetwork()));
}

void MainWindow::initDisplayMenu()
{
    QAction *action;

    ui.menuDisplay->addAction("&Open overlay", this, SLOT(openOverlay()));
    ui.menuDisplay->addSeparator();
    action = ui.menuDisplay->addAction("&Area", this, SLOT(displaySharedOverlay(bool)));
    action->setObjectName("area");
    action->setCheckable(true);
    action->setChecked(true);

    action = ui.menuDisplay->addAction("&Sensor", this, SLOT(displaySharedOverlay(bool)));
    action->setObjectName("sensor");
    action->setCheckable(true);
    action->setChecked(true);

    action = ui.menuDisplay->addAction("&Global ranges", this, SLOT(displayRangeOverlay(bool)));
    action->setObjectName("range");
    action->setCheckable(true);

    action = ui.menuDisplay->addAction("&Nodes", this, SLOT(displaySharedOverlay(bool)));
    action->setObjectName("nodes");
    action->setCheckable(true);
    action->setChecked(true);
}

void MainWindow::loadExternalOverlays()
{
#if 0
    QAction *action;
    Overlay *overlay;

    overlay = new IntrusionReplay(this);
    overlay->setModel(m_sensorsModel);

    if (!overlay->alwaysVisible()) {
        action = ui.menuDisplay->addAction(overlay->objectName(), this, SLOT(displaySharedOverlay(bool)));
        action->setCheckable(true);
        action->setChecked(true);
        action->setObjectName(overlay->objectName());
    }
    if (overlay->options() & Overlay::RenderInDeploymentView)
        m_deploymentView->addOverlay(overlay);
    if (overlay->options() & Overlay::RenderInScenarioView)
        m_editView->addOverlay(overlay);
    if (overlay->options() & Overlay::RenderInC2View)
        m_c2View->addOverlay(overlay);
#endif
}

void MainWindow::removeItem(QDeclarativeItem *item)
{
    if (item->property("className") == "Sensor") {
        m_sensorsModel->removeSensor(m_sensorsModel->sensorAt(item->property("modelIndex").toInt()));
    } else if (item->property("className") == "Node") {
        m_sensorsModel->removeNode(m_sensorsModel->nodeAt(item->property("modelIndex").toInt()));
    }
}

void MainWindow::openScenario()
{
    QFile file;
    QString fileName;
    QDataStream inputStream;

    fileName = QFileDialog::getOpenFileName(this, tr("Open scenario"), QDir::homePath(), tr("Diase scenario (*.diase)"));
    if (fileName.isEmpty())
        return;
    file.setFileName(fileName);
    file.open(QIODevice::ReadOnly);
    inputStream.setDevice(&file);

    while (!inputStream.atEnd()) {
        char *type;

        inputStream >> type;
        if (!strcmp(type, "Intruder")) {
            char *subtype;
            qint32 x,y;

            inputStream >> subtype;
            inputStream >> x;
            inputStream >> y;
            m_editView->addIntruder(subtype, x, y);
            delete subtype;
            delete type;
        } else if (!strcmp(type, "Path")) {
            qint32 x1,y1,x2,y2;

            inputStream >> x1;
            inputStream >> y1;
            inputStream >> x2;
            inputStream >> y2;
            m_editView->addPath(x1, y1, x2, y2);
            delete type;
        } else if (!strcmp(type, "Deployment")) {
            char *deploymentRelativePath;

            inputStream >> deploymentRelativePath;

            m_networktopology->openTopology(QFileInfo(fileName).absolutePath() + "/" + deploymentRelativePath);
            delete deploymentRelativePath;
            delete type;
        }
    }
}

void MainWindow::openOverlay()
{
    QString fileName;

    fileName = QFileDialog::getOpenFileName(this, tr("Open an overlay"), QDir::homePath(), tr("Diase overlay (*.js)"));
    if (fileName.isEmpty())
        return;
}

void MainWindow::saveScenario()
{
    QFile file;
    QString fileName;
    QString scenarioAbsolutePath;
    QString deploymentRelativePath;
    QPair<QString, QPoint> p;

    QDataStream outputStream;

    fileName = QFileDialog::getSaveFileName(this, tr("Save scenario"), QDir::homePath(), tr("Diase scenario (*.diase)"));
    if (fileName.isEmpty())
        return;
    file.setFileName(fileName);
    file.open(QIODevice::WriteOnly);
    outputStream.setDevice(&file);

    scenarioAbsolutePath = QFileInfo(fileName).absolutePath();
    deploymentRelativePath = QDir(scenarioAbsolutePath).relativeFilePath(m_networktopology->filePath());

    outputStream << "Deployment" << deploymentRelativePath.toLocal8Bit().data();
    foreach(p, m_editView->intruders())
        outputStream << "Intruder" << p.first.toLocal8Bit().data() << (qint32)p.second.rx() << (qint32)p.second.ry();
    foreach(QLine l, m_editView->paths())
        outputStream << "Path" << (qint32)l.x1() << (qint32)l.y1() << (qint32)l.x2() << (qint32)l.y2();
}

void MainWindow::showDeploymentView()
{
    ui.menuFile->findChildren<QAction *>("openDeployment").at(0)->setVisible(true);
    ui.menuFile->findChildren<QAction *>("saveDeployment").at(0)->setVisible(true);
    ui.menuFile->findChildren<QAction *>("openScenario").at(0)->setVisible(false);
    ui.menuFile->findChildren<QAction *>("saveScenario").at(0)->setVisible(false);
    ui.menuDisplay->findChildren<QAction *>("range").at(0)->setVisible(true);
    m_deploymentView->setContentX(m_currentView->contentX());
    m_deploymentView->setContentY(m_currentView->contentY());
    ui.stackedWidget->setCurrentWidget(ui.pageDeploymentView);
    ui.deploymentToolBar->setVisible(true);
    ui.C2DockWidget->setVisible(false);
    m_currentView = m_deploymentView;
}

void MainWindow::showEditView()
{
    ui.menuFile->findChildren<QAction *>("openDeployment").at(0)->setVisible(false);
    ui.menuFile->findChildren<QAction *>("saveDeployment").at(0)->setVisible(false);
    ui.menuFile->findChildren<QAction *>("openScenario").at(0)->setVisible(true);
    ui.menuFile->findChildren<QAction *>("saveScenario").at(0)->setVisible(true);
    ui.menuDisplay->findChildren<QAction *>("range").at(0)->setVisible(false);
    m_editView->setContentX(m_currentView->contentX());
    m_editView->setContentY(m_currentView->contentY());
    ui.stackedWidget->setCurrentWidget(ui.pageEditView);
    ui.deploymentToolBar->setVisible(false);
    ui.C2DockWidget->setVisible(false);
    m_currentView = m_editView;
}

void MainWindow::showC2View()
{
    ui.menuFile->findChildren<QAction *>("openDeployment").at(0)->setVisible(false);
    ui.menuFile->findChildren<QAction *>("saveDeployment").at(0)->setVisible(false);
    ui.menuFile->findChildren<QAction *>("openScenario").at(0)->setVisible(false);
    ui.menuFile->findChildren<QAction *>("saveScenario").at(0)->setVisible(false);
    ui.menuDisplay->findChildren<QAction *>("range").at(0)->setVisible(false);
    m_c2View->setContentX(m_currentView->contentX());
    m_c2View->setContentY(m_currentView->contentY());
    ui.stackedWidget->setCurrentWidget(ui.pageC2View);
    ui.deploymentToolBar->setVisible(false);

    if (ui.C2DockWidget->property("receivedAlarm").toBool())
        ui.C2DockWidget->setVisible(true);

    m_currentView = m_c2View;
}

void MainWindow::showMonitoring()
{
    ui.deploymentToolBar->setVisible(false);
    ui.C2DockWidget->setVisible(false);
    ui.stackedWidget->setCurrentWidget(ui.pageMonitoring);
}

#if 0
void MainWindow::updateMonitoringTreeView(AreaItemModel *area)
{
    int i, j;
    QTreeWidgetItem *areaItem, *nodeItem;

    areaItem = new QTreeWidgetItem(ui.nodesTreeWidget, QStringList() << "Area "+ QString::number(area->area()));
    areaItem->setExpanded(true);
    ui.nodesTreeWidget->insertTopLevelItem(0, areaItem);
    for (i = 0; i < m_sensorsModel->nodesCount(); i++) {
        NodeItemModel *node = m_sensorsModel->nodeAt(i);

        if (node->root() != area)
            continue;
        nodeItem = new QTreeWidgetItem(areaItem, QStringList() << "node "+ QString::number(node->nodeId()));
        nodeItem->setData(0, Qt::UserRole, i);
        ui.nodesTreeWidget->insertTopLevelItem(i + 1, nodeItem);
    }
}
#endif

FancyButton *MainWindow::addFancyButton(const QString &text)
{
    FancyButton *button;

    button = new FancyButton(text, ui.toolBar);
    ui.toolBar->addFancyButton(button);
    return button;
}


void MainWindow::itemAddedToView(QDeclarativeItem *item)
{
    if(item->property("className") == "Sensor") {
        connect(item, SIGNAL(intrusionDetected(int)), SLOT(itemIntrusionDetected(int)));
        m_sensorsModel->addSensor(new SensorItemModel(item));
        delete item;
    } else if (item->property("className") == "Node") {
        m_sensorsModel->addNode(new NodeItemModel(item));
        delete item;
    }
}

void MainWindow::sensorAddedToView(QDeclarativeItem *item)
{
    connect(item, SIGNAL(intrusionDetected(int)), SLOT(itemIntrusionDetected(int)));
}

void MainWindow::itemIntrusionDetected(int value)
{
    QDeclarativeItem *item;
    SensorItemModel *itemModel;
    NodeItemModel *nodeItemModel;

    item = qobject_cast<QDeclarativeItem *>(sender());
    itemModel = m_sensorsModel->sensorAt(item->property("modelIndex").toInt());
    nodeItemModel = qobject_cast<NodeItemModel *>(itemModel->root());
    qDebug() << "intrusionDetected for Node" << nodeItemModel->nodeId() << itemModel->type() << "-" << itemModel->sensorId() << "value=" << value;
    m_eventnotifier->notify(nodeItemModel->nodeId(), itemModel->sensorId(), itemModel->type(), value);
}

void MainWindow::nodeFailed(quint16 nodeId)
{
    NodeItemModel *node;
    int nodes;

    nodes = m_sensorsModel->nodesCount();
    for (int i = 0; i < nodes; i++) {
        node = m_sensorsModel->nodeAt(i);

        if (node->nodeId() == nodeId) {
            node->setProperty("state", "fail");
        }
    }
}

void MainWindow::nodesAlarm(QList<int> nodesId, quint32 duration)
{
    NodeItemModel *node;
    AreaItemModel *area;
    SensorItemModel *sensor;
    QString nodesLabels;
    int i, j, nodes;

    nodes = m_sensorsModel->nodesCount();

    foreach (int nodeid, nodesId) {
        for (i = 0; i < nodes; i++) {
            node = m_sensorsModel->nodeAt(i);

            if (node->nodeId() == nodeid) {
                nodesLabels += QString::number(nodeid) + ",";
                area = qobject_cast<AreaItemModel *>(node->root());
		
                if (area)
                    area->setState("alarm");
                for (j = 0; j < node->count(); j++) {
                    sensor = node->at(j);
                    sensor->setState("alarm");
                }
            }
        }
    }
    nodesLabels.remove(nodesLabels.count() - 1, 1);
    if (area && node) {
        ui.C2DockWidget->setProperty("receivedAlarm", true);

        if (m_currentView == m_c2View)
            ui.C2DockWidget->setVisible(true);
        ui.C2TextEdit->append("[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "]  Alarm received for area " + QString::number(area->area()) + " for nodes " + nodesLabels
                              + ". Duration = " + QString::number(duration) + " ms");
    }
}

void MainWindow::connectToRemoteNetwork()
{
    bool ok = true;
    int p;
    QString host, port;

    m_connectionDialog->exec();
    if (m_connectionDialog->result() != QDialog::Accepted)
        return;
    host = ui_connection.dispHostLineEdit->text();
    port = ui_connection.dispPortLineEdit->text();

    if (! host.isEmpty()) {
        if (! host.contains(QRegExp("[0-9]+.[0-9]+.[0-9]+.[0-9]+")))
            ok = false;
        p = port.toInt(&ok);

        if (!ok) {
            QMessageBox::critical(qobject_cast<QWidget *>(parent()), "Error", "Invalid dispatcher address");
            return;
        }
        m_eventnotifier->connectToDispatcher(host, p);
    }

    host = ui_connection.gatewayHostLineEdit->text();
    port = ui_connection.gatewayPortLineEdit->text();

    if (! host.isEmpty()) {
        if (! host.contains(QRegExp("[0-9]+.[0-9]+.[0-9]+.[0-9]+")))
            ok = false;
        p = port.toInt(&ok);

        if (!ok) {
            QMessageBox::critical(qobject_cast<QWidget *>(parent()), "Error", "Invalid gateway address");
            return;
        }
        m_alarmnotifier->connectToServer(host, p);
    }
}

void MainWindow::displaySharedOverlay(bool checked)
{
    QAction *action;

    action = qobject_cast<QAction *>(sender());

    if (checked) {
        m_editView->showOverlay(action->objectName());
        m_deploymentView->showOverlay(action->objectName());
        m_c2View->showOverlay(action->objectName());
    } else {
        m_editView->hideOverlay(action->objectName());
        m_deploymentView->hideOverlay(action->objectName());
        m_c2View->hideOverlay(action->objectName());
    }
}

void MainWindow::displayRangeOverlay(bool checked)
{
    if (checked)
        m_deploymentView->showOverlay("range");
    else
        m_deploymentView->hideOverlay("range");
}
