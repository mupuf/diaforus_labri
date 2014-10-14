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

#include "deploymentsettings.h"
#include "sensormodel.h"

#include <QtGui/QToolBar>
#include <QtGui/QSpinBox>
#include <QtGui/QLineEdit>
#include <QtCore/QDebug>
#include <QtCore/QMetaProperty>

#define PROPERTY_COLUMN 0
#define VALUE_COLUMN 1

static inline bool showProperty(const QString &name)
{
    // internal private properties
    return (name != "objectName") && (name != "state") && (name != "type") && (name != "rootName") && (name != "imgSrc");
}

DeploymentSettings::DeploymentSettings(QToolBar *parent) :
    QObject(parent)
    , m_eventLoop(new QEventLoop(parent))
{
    m_widget = new QWidget(parent);
    ui.setupUi(m_widget);

    parent->addWidget(m_widget);
    ui.propertiesTableWidget->verticalHeader()->hide();
    ui.propertiesTableWidget->horizontalHeader()->hide();
    ui.questionLabel->hide();
    ui.buttonBox->hide();
    m_widget->layout()->setMargin(0);
    ui.propertiesTableWidget->setStyleSheet("QTableWidget { background-color: #c2bbb8 }");

    connect(ui.itemTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), SLOT(treeWidgetItemClicked(QTreeWidgetItem*,int)));
    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(createArea()));
    connect(ui.buttonBox, SIGNAL(rejected()), SLOT(noCreateArea()));
}

void DeploymentSettings::setModel(SensorsModel *model)
{
    m_model = model;

    connect(m_model, SIGNAL(areaAdded(AreaItemModel*)), SLOT(areaAdded(AreaItemModel*)));
    connect(m_model, SIGNAL(areaRemoved(AreaItemModel*)), SLOT(removeArea(AreaItemModel*)));
    connect(m_model, SIGNAL(nodeAdded(NodeItemModel*)), SLOT(nodeAdded(NodeItemModel*)));
    connect(m_model, SIGNAL(nodeRemoved(NodeItemModel*)), SLOT(removeNode(NodeItemModel*)));
    connect(m_model, SIGNAL(sensorAdded(SensorItemModel*)), SLOT(sensorAdded(SensorItemModel*)));
    connect(m_model, SIGNAL(sensorRemoved(SensorItemModel*)), SLOT(removeSensor(SensorItemModel*)));
    connect(m_model, SIGNAL(itemPropertyChanged(ItemModel*,const char*,QVariant)), SLOT(itemModelPropertyChanged(ItemModel*,const char*,QVariant)));
}

void DeploymentSettings::createArea()
{
    m_eventLoop->exit(0);
}

void DeploymentSettings::noCreateArea()
{
    m_eventLoop->exit(1);
}

void DeploymentSettings::itemModelPropertyChanged(ItemModel *item, const char *property, QVariant value)
{
    Q_UNUSED(property);
    Q_UNUSED(value);

    if (m_currentItem != item)
        return;
    for (int i = 0; i < ui.propertiesTableWidget->rowCount(); i++)
        if (ui.propertiesTableWidget->item(i, PROPERTY_COLUMN)->text() == QString(property))
            qobject_cast<QLineEdit *>(ui.propertiesTableWidget->cellWidget(i, VALUE_COLUMN))->setText(value.toString());
}

void DeploymentSettings::itemPropertyChanged()
{
    QLineEdit *lineEdit;

    lineEdit = qobject_cast<QLineEdit *>(sender());
    m_currentItem->setProperty(lineEdit->property("propertyName").toString().toLocal8Bit().data(), lineEdit->text());
}

void DeploymentSettings::areaAdded(AreaItemModel *area)
{
    Q_UNUSED(area);
    showItemTreeWidget();
}

void DeploymentSettings::removeArea(AreaItemModel *area)
{
    Q_UNUSED(area);
    showItemTreeWidget();
}

void DeploymentSettings::nodeAdded(NodeItemModel *node)
{
    Q_UNUSED(node);
    showItemTreeWidget();
}

void DeploymentSettings::removeNode(NodeItemModel *node)
{
    Q_UNUSED(node);
    showItemTreeWidget();

    if (m_currentItem == node)
        clearItemProperties();
}

void DeploymentSettings::sensorAdded(SensorItemModel *sensor)
{
    Q_UNUSED(sensor);
    showItemTreeWidget();
}

void DeploymentSettings::removeSensor(SensorItemModel *sensor)
{
    Q_UNUSED(sensor);
    showItemTreeWidget();

    if (m_currentItem == sensor)
        clearItemProperties();
}

void DeploymentSettings::sensorChangesParent()
{
    QLineEdit *lineEdit;
    NodeItemModel *parent;
    SensorItemModel *item;
    int nodeId, i;

    lineEdit = qobject_cast<QLineEdit *>(sender());
    nodeId = lineEdit->text().toInt();
    item = qobject_cast<SensorItemModel *>(m_currentItem);
    parent = qobject_cast<NodeItemModel *>(item->root());

    // Reparent
    for (i = 0; i < m_model->nodesCount(); i++) {
        if (m_model->nodeAt(i)->nodeId() == nodeId) {
            if (parent)
                parent->removeSensor(item, false);
            m_model->nodeAt(i)->addSensor(item);
            m_currentItem->setRoot(m_model->nodeAt(i));
            break;
        }
    }
    // The required node does not exist
    if (i == m_model->nodesCount())
        return;

    // Notify the model and all views that the identifier for this sensor has changed
    item->rename(m_model->nodeAt(i)->objectName()+"::Sensor"+item->type() + "-" + QString::number(item->sensorId()));
    showItemTreeWidget();
}

void DeploymentSettings::nodeChangesParent()
{
    QLineEdit *lineEdit;
    AreaItemModel *parent;
    NodeItemModel *item;
    int areaId, i, exitCode;
    bool found = false;

    lineEdit = qobject_cast<QLineEdit *>(sender());
    areaId = lineEdit->text().toInt();
    item = qobject_cast<NodeItemModel *>(m_currentItem);
    parent = qobject_cast<AreaItemModel *>(item->root());

    // Reparent
    for (i = 0; i < m_model->areasCount(); i++) {
        if (m_model->areaAt(i)->area() == areaId) {
            if (parent)
                parent->removeNode(item, false);
            m_model->areaAt(i)->addNode(item);
            m_currentItem->setRoot(m_model->areaAt(i));
            found = true;
            break;
        }
    }
    if (!found) {
        ui.questionLabel->setText("create area " + QString::number(areaId) + " ?");
        ui.questionLabel->show();
        ui.buttonBox->show();

        exitCode = m_eventLoop->exec();

        ui.questionLabel->hide();
        ui.buttonBox->hide();

        if (exitCode == 1)
            return;
        m_model->addArea(new AreaItemModel(areaId));
        i = m_model->areasCount() - 1;
        m_model->areaAt(i)->setObjectName("Area"+ QString::number(areaId));
        m_model->areaAt(i)->addNode(item);
        m_currentItem->setRoot(m_model->areaAt(i));
    }
    // Notify the model and all views that the identifier for this node has changed
    item->rename(m_model->areaAt(i)->objectName()+"::Node"+QString::number(item->nodeId()));

    // Also change identifiers for all sensors attached to this node
    for (i = 0; i < item->count(); i++)
        item->at(i)->rename(item->objectName()+"::Sensor"+item->at(i)->type() + "-" + QString::number(item->at(i)->sensorId()));
    showItemTreeWidget();
}

void DeploymentSettings::clearItemProperties()
{
    ui.label->setText("No item selected");
    ui.propertiesTableWidget->clear();
    ui.propertiesTableWidget->setRowCount(0);
}

void DeploymentSettings::showItemProperties()
{
    const QMetaObject *mo;
    QLineEdit *lineEdit;
    int currRow = 1;

    if (!m_currentItem->objectName().isEmpty())
        ui.label->setText(m_currentItem->objectName().split(":").last());
    else
        ui.label->setText("New item");
    mo = m_currentItem->metaObject();
    for (int i = 0; i < mo->propertyCount(); i++) {
        if (!showProperty(mo->property(i).name()))
            continue;
        QTableWidgetItem *witem;

        witem = new QTableWidgetItem(mo->property(i).name());
        lineEdit = new QLineEdit(mo->property(i).read(m_currentItem).toString(), ui.propertiesTableWidget);
        if (currRow % 2 == 0) {
            witem->setBackgroundColor(QColor(255, 255, 222));
            lineEdit->setStyleSheet("QLineEdit { background-color: rgb(255,255,222); border : 1px }");
        }
        else {
            witem->setBackgroundColor(QColor(255, 255, 191));
            lineEdit->setStyleSheet("QLineEdit { background-color: rgb(255,255,191); border : 1px }");
        }
        ui.propertiesTableWidget->setRowCount(ui.propertiesTableWidget->rowCount() + 1);
        ui.propertiesTableWidget->setItem(currRow, PROPERTY_COLUMN, witem);

        lineEdit->setProperty("propertyName", mo->property(i).name());
        connect(lineEdit, SIGNAL(returnPressed()), SLOT(itemPropertyChanged()));

        ui.propertiesTableWidget->setCellWidget(currRow++, VALUE_COLUMN, lineEdit);
    }
}

//TODO: refactoring
void DeploymentSettings::showSensorProperties(SensorItemModel *item)
{
    QLineEdit *lineEdit;
    NodeItemModel *node;

    m_currentItem = item;

    ui.propertiesTableWidget->clear();
    ui.propertiesTableWidget->setColumnCount(2);
    ui.propertiesTableWidget->setRowCount(0);

    ui.propertiesTableWidget->setRowCount(1);
    ui.propertiesTableWidget->setItem(0, PROPERTY_COLUMN, new QTableWidgetItem("node"));
    ui.propertiesTableWidget->item(0, PROPERTY_COLUMN)->setBackgroundColor(QColor(255, 255, 222));

    node = qobject_cast<NodeItemModel *>(item->root());
    if (node)
        lineEdit = new QLineEdit(QString::number(node->nodeId()), ui.propertiesTableWidget);
    else
        lineEdit = new QLineEdit("undefined", ui.propertiesTableWidget);
    lineEdit->setStyleSheet("QLineEdit { background-color: rgb(255,255,222); border : 1px }");
    connect(lineEdit, SIGNAL(returnPressed()), SLOT(sensorChangesParent()));
    ui.propertiesTableWidget->setCellWidget(0, VALUE_COLUMN, lineEdit);

    showItemProperties();
    showItemTreeWidget();
}

void DeploymentSettings::showNodeProperties(NodeItemModel *item)
{
    QLineEdit *lineEdit;
    AreaItemModel *area;

    m_currentItem = item;
    ui.propertiesTableWidget->clear();
    ui.propertiesTableWidget->setColumnCount(2);
    ui.propertiesTableWidget->setRowCount(0);

    ui.propertiesTableWidget->setRowCount(1);
    ui.propertiesTableWidget->setItem(0, PROPERTY_COLUMN, new QTableWidgetItem("area"));
    ui.propertiesTableWidget->item(0, PROPERTY_COLUMN)->setBackgroundColor(QColor(255, 255, 222));

    area = qobject_cast<AreaItemModel *>(item->root());
    if (area)
        lineEdit = new QLineEdit(QString::number(area->area()), ui.propertiesTableWidget);
    else
        lineEdit = new QLineEdit("undefined", ui.propertiesTableWidget);
    lineEdit->setStyleSheet("QLineEdit { background-color: rgb(255,255,222); border : 1px }");
    connect(lineEdit, SIGNAL(returnPressed()), SLOT(nodeChangesParent()));
    ui.propertiesTableWidget->setCellWidget(0, VALUE_COLUMN, lineEdit);

    showItemProperties();
    showItemTreeWidget();
}

void DeploymentSettings::showItemTreeWidget()
{
    ui.itemTreeWidget->clear();

    for (int i = 0; i < m_model->areasCount(); i++) {
        AreaItemModel *area =  m_model->areaAt(i);
        QTreeWidgetItem *areaItem = new QTreeWidgetItem();

        areaItem->setText(0, "Area " + QString::number(area->area()));

        ui.itemTreeWidget->insertTopLevelItem(i, areaItem);
        ui.itemTreeWidget->expandItem(areaItem);

        for (int j = 0; j < area->count(); j++) {
            NodeItemModel *node = area->at(j);
            QTreeWidgetItem *nodeItem = new QTreeWidgetItem(areaItem);

            nodeItem->setText(0, "Node " + QString::number(node->nodeId()));
            nodeItem->setData(0, Qt::UserRole, node->objectName());

            ui.itemTreeWidget->insertTopLevelItem(j, nodeItem);

            for (int k = 0; k < node->count(); k++) {
                SensorItemModel *sensor = node->at(k);
                QTreeWidgetItem *sensorItem = new QTreeWidgetItem(nodeItem);

                sensorItem->setText(0, "Sensor " + sensor->type() + " " + QString::number(sensor->sensorId()));
                sensorItem->setData(0, Qt::UserRole, sensor->objectName());
                ui.itemTreeWidget->insertTopLevelItem(k, sensorItem);
            }
            ui.itemTreeWidget->expandItem(nodeItem);
        }
    }
    // Show orphan nodes
    for (int i = 0; i < m_model->nodesCount(); i++) {
        if (m_model->nodeAt(i)->root() == NULL) {
            NodeItemModel *node = m_model->nodeAt(i);
            QTreeWidgetItem *nodeItem = new QTreeWidgetItem;

            nodeItem->setText(0, "Node " + QString::number(node->nodeId()));
            nodeItem->setData(0, Qt::UserRole, node->objectName());
            ui.itemTreeWidget->addTopLevelItem(nodeItem);

            for (int k = 0; k < node->count(); k++) {
                SensorItemModel *sensor = node->at(k);
                QTreeWidgetItem *sensorItem = new QTreeWidgetItem(nodeItem);

                sensorItem->setText(0, "Sensor " + sensor->type() + " " + QString::number(sensor->sensorId()));
                sensorItem->setData(0, Qt::UserRole, sensor->objectName());
                ui.itemTreeWidget->addTopLevelItem(sensorItem);
            }
            ui.itemTreeWidget->expandItem(nodeItem);
        }
    }
    // Show orphan sensors
    for (int i = 0; i < m_model->sensorsCount(); i++) {
        if (m_model->sensorAt(i)->root() == NULL) {
            SensorItemModel *sensor = m_model->sensorAt(i);
            QTreeWidgetItem *sensorItem = new QTreeWidgetItem;

            sensorItem->setText(0, "Sensor " + sensor->type() + " " + QString::number(sensor->sensorId()));
            sensorItem->setData(0, Qt::UserRole, sensor->objectName());
            ui.itemTreeWidget->addTopLevelItem(sensorItem);
        }
    }
}

void DeploymentSettings::treeWidgetItemClicked(QTreeWidgetItem *item, int column)
{
    QString selectedObjectName;

    if (item->text(column).startsWith("Area")) {
        ui.label->setText("No item selected");
        ui.propertiesTableWidget->clear();
        ui.propertiesTableWidget->setColumnCount(0);
        ui.propertiesTableWidget->setRowCount(0);
        return;
    }
    selectedObjectName = item->data(column, Qt::UserRole).toString();

    if (item->text(column).startsWith("Node")) {
        for (int i = 0; i < m_model->nodesCount(); i++) {
            if (m_model->nodeAt(i)->objectName() == selectedObjectName) {
                showNodeProperties(m_model->nodeAt(i));
            }
        }
    } else {
        for (int i = 0; i < m_model->sensorsCount(); i++) {
            if (m_model->sensorAt(i)->objectName() == selectedObjectName) {
                showSensorProperties(m_model->sensorAt(i));
            }
        }
    }
}
