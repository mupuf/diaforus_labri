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
#include "networktopology.h"
#include "ui_diase.h"
#include "sensormodel.h"

#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtXml/QDomDocument>

static bool isPython2(QString path)
{
    QProcess process;
    process.start(path, QStringList() << "--version");
    if (!process.waitForStarted())
        return false;

    if (!process.waitForFinished())
        return false;

    process.setReadChannel(QProcess::StandardError);
    QByteArray result = process.readAll();

    return result.startsWith("Python ") && result.mid(7, 1) == "2";
}

static QString python2_path()
{
    static QString python2 = QString();

    if (!python2.isNull())
        return python2;

    if (isPython2("python"))
        python2 = "python";
    else if (isPython2("python2"))
        python2 = "python2";

    return python2;
}

NetworkTopology::NetworkTopology(QWidget *parent, Ui::MainWindow *ui):
    QObject(parent)
    , m_building_dialog(new QDialog(parent))
    , m_nodesDialog(new QDialog())
    , m_hybridDialog(new QDialog())
    , m_buildNetworkProcess(new QProcess(this))
    , m_nodesProcessList(new QList<QProcess *>())
    , m_dispTimer(new QTimer(this))
    , m_reasoningTimer(new QTimer(this))
{
    QAction *action;

    this->ui = ui;

    ui_building.setupUi(m_building_dialog);
    ui_building.progressBar->setMaximum(0);
    ui_building.progressBar->setMinimum(0);

    ui_nodesDialog.setupUi(m_nodesDialog);
    ui_hybridDialog.setupUi(m_hybridDialog);

    action = ui->menuFile->addAction("&Open deployment", this, SLOT(openTopology()));
    action->setObjectName("openDeployment");
    action = ui->menuFile->addAction("&Save deployment", this, SLOT(saveXMLDocument()));
    action->setObjectName("saveDeployment");

#ifndef Q_OS_WIN32
    action = ui->menuNetwork->addAction("&Build", this, SLOT(buildNetwork()));
    action->setEnabled(false);
    action = ui->menuNetwork->addAction("&Start simulation network", this, SLOT(toggleNetwork()));
    action->setProperty("state", "start");
    action->setEnabled(false);

    action = ui->menuNetwork->addAction("&Start hybrid network", this, SLOT(toggleHybridNetwork()));
    action->setProperty("state", "start");
    action->setEnabled(false);

    action = ui->menuNetwork->addAction("&Nodes outputs", this, SLOT(showNodesDialog()));
    ui->menuNetwork->addSeparator();
    ui->menuNetwork->setDefaultAction(action);
    action->setEnabled(false);
#endif

    m_dispTimer->setSingleShot(true);
    m_reasoningTimer->setSingleShot(true);

    connect(m_buildNetworkProcess, SIGNAL(started()), m_building_dialog, SLOT(show()));
    connect(m_buildNetworkProcess, SIGNAL(finished(int)), SLOT(buildNetworkProcessFinished(int)));
    connect(m_dispTimer, SIGNAL(timeout()), SLOT(dispatcherIsReady()));
    connect(m_reasoningTimer, SIGNAL(timeout()), SLOT(reasoningNodesAreReady()));

    /* test if python2 is available on the system */
    if (python2_path().isNull()) {
        QMessageBox::warning(parent,
                             tr("Python2 is not available on this system"),
                             tr("Python2 is not available on this system\n\nPlease install it."));
    }
}

NetworkTopology::~NetworkTopology()
{
    // the processes will be destroyed when @this will be freed
    delete m_nodesProcessList;
    delete m_nodesDialog;
}

void NetworkTopology::itemModelPropertyChanged(ItemModel *item, const char *property, QVariant value)
{
    SensorItemModel *sensorItem = NULL;
    NodeItemModel *nodeItem = NULL;
    AreaItemModel *areaItem = NULL;
    QDomNodeList nodes, modalities, modality_sensors;
    QDomNode sensors, sensor, position, phy;
    int i, j, k, area, nodeId;

    sensorItem = qobject_cast<SensorItemModel *>(item);
    nodeItem = qobject_cast<NodeItemModel *>(item);
    if (sensorItem && (QString::fromLatin1(property) == "x" || QString::fromLatin1(property) == "y")) {
        nodeItem = qobject_cast<NodeItemModel *>(sensorItem->root());

        if (nodeItem)
            areaItem = qobject_cast<AreaItemModel *>(nodeItem->root());

        if (!nodeItem || !areaItem)
            return;

        nodes = m_dom.elementsByTagName("node");
        for(i = 0; i < nodes.count(); i++) {
            area = nodes.at(i).toElement().attribute("area", "").toInt();
            nodeId = nodes.at(i).toElement().attribute("id", "").toInt();

            if (areaItem->area() == area && nodeItem->nodeId() == nodeId) {
                sensors = nodes.at(i).firstChildElement("sensors");
                modalities = sensors.childNodes();

                for (j = 0; j < modalities.count(); j++) {
                    if (modalities.at(j).toElement().attribute("type", "") == sensorItem->type()) {
                        modality_sensors = modalities.at(j).childNodes();

                        for (k = 0; k < modality_sensors.count(); k++) {
                            sensor = modality_sensors.at(k);
                            if (sensor.toElement().attribute("id", "").toInt() == sensorItem->sensorId()) {
                                position = sensor.firstChildElement("position");
                                position.toElement().setAttribute(property, value.toInt());

                                qDebug() << "area " << area << "nodeId" << nodeId << "modality" << modalities.at(j).toElement().attribute("type", "")
                                         << "id" << sensor.toElement().attribute("id", "").toInt() << "updated to" << QString::fromLatin1(property) << "=" << value.toInt();
                            }
                        }
                    }
                }
            }
        }
    }
    else if (nodeItem && (QString::fromLatin1(property) == "phyX" || QString::fromLatin1(property) == "phyY")) {
        areaItem = qobject_cast<AreaItemModel *>(nodeItem->root());
        nodes = m_dom.elementsByTagName("node");

        if (!areaItem)
            return;

         for(i = 0; i < nodes.count(); i++) {
             area = nodes.at(i).toElement().attribute("area", "").toInt();
             nodeId = nodes.at(i).toElement().attribute("id", "").toInt();

             if (areaItem->area() == area && nodeItem->nodeId() == nodeId) {
                phy = nodes.at(i).firstChildElement("phy");

                if (QString::fromLatin1(property) == "phyX")
                    phy.toElement().setAttribute("x", value.toInt());
                else
                    phy.toElement().setAttribute("y", value.toInt());
                //qDebug() << "area" << area << "nodeId" << nodeId << "updated " << QString::fromLatin1(property) << "=" << value.toInt();
             }
         }
    }
}

void NetworkTopology::saveXMLDocument()
{
    QString fileName;
    QFile file;
    fileName = fileName = QFileDialog::getSaveFileName(qobject_cast<QWidget *>(parent()), tr("Save file"), QDir::homePath(), tr("XML files (*.xml)"));

    if (fileName.isEmpty())
        return;
    file.setFileName(fileName);
    if (! file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(qobject_cast<QWidget *>(parent()), "Error", "Unable to save the xml document");
        return;
    }
    QTextStream output(&file);
    m_dom.save(output, 4);
    m_xmlDoc.setFileName(fileName);
}

QString NetworkTopology::filePath() const
{
    return m_xmlDoc.fileName();
}

void NetworkTopology::openTopology(const QString &filePath)
{
    QString fileName, errorMsg;
    QDomNodeList nodes, groups;
    AreaItemModel *areaItem = NULL;
    QFileInfo info;
    int errorLine, errorColumn;

    if (filePath.isEmpty())
        fileName = QFileDialog::getOpenFileName(qobject_cast<QWidget *>(parent()), tr("Open file"), QDir::homePath(), tr("XML files (*.xml)"));
    else
        fileName = filePath;

    if (fileName.isEmpty())
        return;
    m_dom.clear();
    emit clear();
    m_xmlDoc.setFileName(fileName);

    if (! m_xmlDoc.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(qobject_cast<QWidget *>(parent()), "Error", "Unable to open the xml document");
        return;
    }

    if (! m_dom.setContent(&m_xmlDoc, &errorMsg, &errorLine, &errorColumn)) {
        QMessageBox::critical(qobject_cast<QWidget *>(parent()), "Error", QString("Parse error: %1").arg(errorMsg));
        m_xmlDoc.close();
        return;
    }
    info = m_xmlDoc.fileName();
    groups = m_dom.elementsByTagName("coap_groups").at(0).childNodes();
    for (int i = 0; i < groups.count(); i++) {
        QStringList resourceNames;
        QDomNodeList resources = groups.at(i).childNodes();

        for (int j = 0; j < resources.count(); j++)
            resourceNames << resources.at(j).toElement().attribute("name", "");
        emit coapGroupAdded(groups.at(i).toElement().attribute("name", ""), resourceNames);
    }

    // For each node
    nodes = m_dom.elementsByTagName("node");
    for (int i = 0; i < nodes.count(); i++) {
        QDomNode sensors, phy;
        int area, nodeID, phyX, phyY, range;
        QString emblem;
        NodeItemModel *nodeItem;

        area = nodes.at(i).toElement().attribute("area", "").toInt();

        if (!areaItem) {
            areaItem = new AreaItemModel(area);
            areaItem->setObjectName("Area"+QString::number(area));
        }

        phy = nodes.at(i).firstChildElement("phy");
        phyX = phy.toElement().attribute("x", "").toInt();
        phyY = phy.toElement().attribute("y", "").toInt();
        range = phy.toElement().attribute("range", "").toInt();
        nodeID = nodes.at(i).toElement().attribute("id", "").toInt();
        emblem = nodes.at(i).firstChildElement("emblem").toElement().attribute("source");

        nodeItem = new NodeItemModel(nodeID, phyX, phyY, range);
        nodeItem->setObjectName(areaItem->objectName()+"::Node"+QString::number(nodeID));
        nodeItem->setRoot(areaItem);
        if (!emblem.isEmpty())
            nodeItem->setProperty("imgSrc", info.absoluteDir().absolutePath() + "/" + emblem);
        areaItem->addNode(nodeItem);

        sensors = nodes.at(i).firstChildElement("sensors");

        // For each modality in <sensors></sensors>
        QDomNodeList modalities = sensors.childNodes();
        for (int j =  0; j < modalities.count(); j++) {
            QDomNodeList modality_sensors;
            QDomNode modality;
            QString type;

            modality = modalities.at(j);
            type = modality.toElement().attribute("type", "");
            modality_sensors = modality.childNodes();

            // For each sensor of this modality
            for (int k = 0; k < modality_sensors.count(); k++) {
                QDomNode sensor, pos;
                int sensorID, x, y, rotation;
                SensorItemModel *sensorItem;

                sensor = modality_sensors.at(k);
                pos = sensor.firstChildElement("position");

                sensorID = sensor.toElement().attribute("id", "").toInt();
                x = pos.toElement().attribute("x", "").toInt();
                y = pos.toElement().attribute("y", "").toInt();
                rotation = pos.toElement().attribute("z_rotation", "").toInt();

                sensorItem = new SensorItemModel(sensorID, type, x, y, rotation);
                sensorItem->setObjectName(nodeItem->objectName()+"::Sensor"+sensorItem->type()+"-"+QString::number(sensorID));
                sensorItem->setRoot(nodeItem);
                nodeItem->addSensor(sensorItem);
                emit sensorAdded(sensorItem);
            }
        }
        emit nodeAdded(nodeItem);

        QDomNodeList coap_resources = nodes.at(i).firstChildElement("coap").childNodes();
        for (int k = 0; k < coap_resources.count(); k++) {
            emit nodeAddedToCoapGroup(nodeItem, coap_resources.at(k).toElement().attribute("name", ""));
        }

        // If the next area is about to change, the current one is done
        if (i < (nodes.count() - 1)) {
            int nextArea = nodes.at(i+1).toElement().attribute("area").toInt();
            if (areaItem->area() != nextArea) {
                emit areaAdded(areaItem);
                areaItem = NULL;
            }
        }
    }
    emit areaAdded(areaItem);

    // Enable all entries into the network menu except "Nodes output"
    // (network not started yet)
    foreach(QObject *child, ui->menuNetwork->children()) {
        QAction *action = qobject_cast<QAction *>(child);
        action->setEnabled(true);
    }
    ui->menuNetwork->defaultAction()->setEnabled(false);
    m_xmlDoc.close();
}

void NetworkTopology::buildNetwork()
{
    QFileInfo info = m_xmlDoc.fileName();
    QString buildNetworkDir = QDir(info.absoluteDir().absolutePath() + "/../../").absolutePath();
    QString xmlPath = info.filePath().remove(buildNetworkDir + "/");

    m_buildNetworkProcess->setWorkingDirectory(buildNetworkDir);
    m_buildNetworkProcess->start(python2_path(), QStringList() << "build_network.py" << "-b" << xmlPath);
}

void NetworkTopology::toggleNetwork()
{
    QAction *a = NULL;

    a = qobject_cast<QAction *>(sender());
    if (a->property("state").toString() == "start") {
        a->setText("&Stop simulation network");
        a->setProperty("state", "stop");
        startNetwork();
    } else {
        a->setText("&Start simulation network");
        a->setProperty("state", "start");
        stopNetwork();
    }
}

void NetworkTopology::toggleHybridNetwork()
{
    QAction *a = NULL;
    QString iface;
    int speed;

    a = qobject_cast<QAction *>(sender());

    if (a->property("state").toString() == "start") {
        iface = ui_hybridDialog.deviceLineEdit->text();
        speed = ui_hybridDialog.speedLineEdit->text().toInt();
        if (m_hybridDialog->exec() != QDialog::Accepted)
            return;
        a->setText("&Stop hybrid network");
        a->setProperty("state", "stop");
        startNetwork(iface, speed);
    } else {
        a->setText("&Start hybrid network");
        a->setProperty("state", "start");
        stopNetwork();
    }
}

QString NetworkTopology::dispatcherDirectory(bool hybrid) const
{
    QFileInfo info;
    QString xmlSheetDir;

    info = m_xmlDoc.fileName();
    xmlSheetDir = info.absoluteDir().absolutePath();
    return hybrid ? QDir(xmlSheetDir + "/../../../gateway/gateway_waveport/").absolutePath() : QDir(xmlSheetDir + "/../../").absolutePath();
}

void NetworkTopology::startNetwork(const QString &iface, int speed)
{
    QFileInfo info;
    QProcess *p;

    info = m_xmlDoc.fileName();
    /* Firstly, we start the dispatcher */
    p = new QProcess(this);
    p->setObjectName("dispatcher");
    m_nodesProcessList->append(p);
    p->setProcessChannelMode(QProcess::MergedChannels);
    p->setWorkingDirectory(dispatcherDirectory(speed ? true : false));

    p->setProperty("pageIndex", ui_nodesDialog.tabWidget->indexOf(ui_nodesDialog.dispatcherTab));

    connect(p, SIGNAL(readyReadStandardOutput()), SLOT(displayNodeOutput()));
    connect(p, SIGNAL(readyReadStandardError()), SLOT(displayNodeOutput()));

    if (!speed)
        p->start(python2_path(), QStringList() << "dispatcher.py" << "1234" << info.absoluteFilePath().remove(dispatcherDirectory() + "/"));
    else
        p->start(python2_path(), QStringList() << "dispatcher_hybrid.py" << "1234" << info.absoluteFilePath()  << "diase" << iface << QString::number(speed));
    p->waitForStarted();

    m_dispTimer->start(1000);
}

void NetworkTopology::stopNetwork()
{
    QLayout *layout;
    int index;

    m_nodesDialog->hide();
    foreach (QProcess *process, *m_nodesProcessList) {
        process->disconnect(this, SLOT(displayNodeOutput()));
        process->disconnect(this, SLOT(processFinished(int,QProcess::ExitStatus)));
        process->kill();
        if (process->objectName() != "dispatcher" && process->objectName() != "alarmServer") {
            index = process->property("pageIndex").toInt();
            ui_nodesDialog.tabWidget->setCurrentIndex(index);
            ui_nodesDialog.tabWidget->currentWidget()->deleteLater();
        }
        delete process;
    }
     m_nodesProcessList->clear();

    // Clear the dispatcher's logs
    layout = ui_nodesDialog.tabWidget->widget(0)->layout();
    qobject_cast<QTextEdit *>(layout->itemAt(0)->widget())->clear();

    // Clear the alarm server's logs
    layout = ui_nodesDialog.tabWidget->widget(1)->layout();
    qobject_cast<QTextEdit *>(layout->itemAt(0)->widget())->clear();

    // Disable the "Nodes Output" action
    ui->menuNetwork->defaultAction()->setEnabled(false);
}

void NetworkTopology::dispatcherIsReady()
{
    QDomNodeList nodes;
    QString firmware;
    QStringList brokers, reasonings, simpleNodes;
    int brokerId = 0;
    QProcess *p;

    nodes = m_dom.elementsByTagName("node");
    for (int i = 0; i < nodes.count(); i++) {
        QDomElement node;

        if (nodes.at(i).toElement().attribute("simulation", "") != "true")
            continue;

        firmware = nodes.at(i).firstChildElement("firmware").toElement().attribute("name");
        node = nodes.at(i).firstChildElement("pubsub");
        if (!node.isNull() && node.toElement().attribute("broker") == "true") {
            brokers << firmware;
            brokerId = nodes.at(i).toElement().attribute("id", "").toInt();
        }
        node = nodes.at(i).firstChildElement("reasoning").firstChildElement("reasoning_node");
        if (!node.isNull() && !brokers.contains(firmware))
            reasonings << firmware;
        if (!brokers.contains(firmware) && !reasonings.contains(firmware))
            simpleNodes << firmware;
    }
    // Start the alarm server
    p = new QProcess(this);
    p->setObjectName("alarmServer");
    m_nodesProcessList->append(p);
    p->setProcessChannelMode(QProcess::MergedChannels);
    p->setWorkingDirectory(dispatcherDirectory());

    p->setProperty("pageIndex", ui_nodesDialog.tabWidget->indexOf(ui_nodesDialog.alarmServerTab));
    connect(p, SIGNAL(readyReadStandardOutput()), SLOT(displayNodeOutput()));
    connect(p, SIGNAL(readyReadStandardError()), SLOT(displayNodeOutput()));
    p->start(python2_path(), QStringList() << "alarm_server.py" << "4242" << QString::number(brokerId));
    p->waitForStarted();

    /* Firstly, We start the brokers */
    startNodes(brokers);

    /* Then, we start the reasoning nodes */
    startNodes(reasonings);
    m_reasoningTimer->setProperty("simpleNodes", simpleNodes);
    m_reasoningTimer->start(100);
}

void NetworkTopology::reasoningNodesAreReady()
{
    QTimer *timer;
    QStringList simpleNodes;

    timer = qobject_cast<QTimer *>(sender());
    simpleNodes = timer->property("simpleNodes").toStringList();
    /* And finally, each remaining node */
    startNodes(simpleNodes);
    /* The network is started, we can enable the "Nodes output" action */
    ui->menuNetwork->defaultAction()->setEnabled(true);

    emit networkStarted();
}

void NetworkTopology::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QProcess *p;

    p = qobject_cast<QProcess *>(sender());
    QString status = (exitStatus == QProcess::NormalExit) ? "exited normally" : "crashed";    
    QMessageBox::critical(qobject_cast<QWidget *>(parent()), "A node terminated suddenly",p->objectName() + " " + status + " (with code " + QString::number(exitCode) + " )");

    displayNodeOutput(p);
}

void NetworkTopology::startNodes(const QStringList &nodes)
{
    QProcess *p;
    QString xmlSheetDir;
    QWidget *widget;
    int id;

    xmlSheetDir = QFileInfo(m_xmlDoc).absoluteDir().absolutePath();
    foreach(QString node, nodes) {

        p = new QProcess(this);
        p->setObjectName(node);
        m_nodesProcessList->append(p);
        p->setProcessChannelMode(QProcess::MergedChannels);
        p->setWorkingDirectory(xmlSheetDir);

        widget = new QWidget(ui_nodesDialog.tabWidget);
        widget->setLayout(new QVBoxLayout(widget));
        widget->layout()->addWidget(new QTextEdit(widget));
        id = ui_nodesDialog.tabWidget->addTab(widget, node);
        p->setProperty("pageIndex", id);

        connect(p, SIGNAL(readyReadStandardOutput()), SLOT(displayNodeOutput()));
        connect(p, SIGNAL(readyReadStandardError()), SLOT(displayNodeOutput()));
        connect(p, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int,QProcess::ExitStatus)));

        qDebug() << "starting node " << xmlSheetDir + "/" + node;
        p->start("./" + node);
        p->waitForStarted();
    }
}

void NetworkTopology::showNodesDialog()
{
    m_nodesDialog->show();
}

void NetworkTopology::displayNodeOutput(QProcess *process)
{
    QLayout *layout;
    QTextEdit *textEdit;
    QTextCursor cursor;
    QProcess *p;
    QFile file;
    int index;

    if (process)
        p = process;
    else
        p = qobject_cast<QProcess *>(sender());
    index = p->property("pageIndex").toInt();
    layout = ui_nodesDialog.tabWidget->widget(index)->layout();
    textEdit = qobject_cast<QTextEdit *>(layout->itemAt(0)->widget());
    textEdit->insertPlainText(p->readAllStandardOutput());
    cursor = textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    textEdit->setTextCursor(cursor);

    file.setFileName(p->workingDirectory() + "/" + p->objectName() + ".log");
    if (!file.open(QIODevice::WriteOnly))
        return;
    QTextStream outputStream(&file);
    outputStream << textEdit->toPlainText();
}

void NetworkTopology::buildNetworkProcessFinished(int exitCode)
{
    m_building_dialog->hide();

    if (exitCode != 0) {
        QMessageBox::critical(qobject_cast<QWidget *>(parent()), "Build Error", m_buildNetworkProcess->readAllStandardError());
    }
}
