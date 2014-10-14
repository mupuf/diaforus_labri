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
#ifndef DEPLOYMENTSETTINGS_H
#define DEPLOYMENTSETTINGS_H

#include <QtCore/QObject>
#include "ui_deployment_table.h"

class QToolBar;
class QWidget;
class QEventLoop;
class SensorsModel;
class SensorItemModel;
class NodeItemModel;
class AreaItemModel;
class ItemModel;

/*
 * The class DeploymentSettings is the controller which keeps the deployment view, the sensors model,
 * and the graphical deployments informations (treewidget and table properties in the right panel) up-to-date, according to what the user does
 * when he edits the deployment.
 */

class DeploymentSettings : public QObject
{
    Q_OBJECT
public:
    explicit DeploymentSettings(QToolBar *parent = 0);

    /*
     * Change the sensors model used internally
     * @param model the sensors model used to synchronize everything (views, topology informations, etc)
     */
    void setModel(SensorsModel *model);

public Q_SLOTS:
    void showSensorProperties(SensorItemModel *item);
    void showNodeProperties(NodeItemModel *item);
    void itemPropertyChanged();
    void sensorChangesParent();
    void nodeChangesParent();
    void treeWidgetItemClicked(QTreeWidgetItem *item, int column);
    void createArea();
    void noCreateArea();
    void areaAdded(AreaItemModel *area);
    void removeArea(AreaItemModel *area);
    void nodeAdded(NodeItemModel *node);
    void removeNode(NodeItemModel *node);
    void sensorAdded(SensorItemModel *sensor);
    void removeSensor(SensorItemModel *sensor);
    void itemModelPropertyChanged(ItemModel *item,const char *property, QVariant value);

private:
    void showItemProperties();
    void showItemTreeWidget();
    void clearItemProperties();

private:
    Ui::deploymentTable ui;

    QWidget *m_widget;
    SensorsModel *m_model;
    ItemModel *m_currentItem;
    QEventLoop *m_eventLoop;
};

#endif // DEPLOYMENTSETTINGS_H
