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
#include "sensormodel.h"
#include <QtCore/QMetaObject>
#include <QtCore/QDynamicPropertyChangeEvent>
#include <QtDeclarative/QDeclarativeItem>

ItemModel::ItemModel(ItemModel *root):
    QObject()
    , m_root(root)
    , m_state("")
{
}

ItemModel *ItemModel::root() const
{
    return m_root;
}

void ItemModel::setRoot(ItemModel *root)
{
    m_root = root;
    m_rootName = root ? root->objectName() : NULL;
    emit propertyChanged("rootName", m_rootName);
}

QString ItemModel::state() const
{
    return m_state;
}

void ItemModel::setState(const QString &state)
{
    m_state = state;
    emit propertyChanged("state", state);
}

void ItemModel::rename(const QString &name)
{
    emit propertyChanged("objectName", name);
    setObjectName(name);
}

QString ItemModel::rootName() const
{
    return m_rootName;
}

bool ItemModel::event(QEvent *e)
{
    QDynamicPropertyChangeEvent *ev;

    if (e->type() == QEvent::DynamicPropertyChange) {
        ev = static_cast<QDynamicPropertyChangeEvent *>(e);
        emit propertyChanged(ev->propertyName().data(), property(ev->propertyName().data()));
    }
    return QObject::event(e);
}

AreaItemModel::AreaItemModel(int area):
    ItemModel()
    , m_area(area)
{
}

int AreaItemModel::area() const
{
    return m_area;
}

void AreaItemModel::setArea(int area)
{
    m_area = area;
    emit propertyChanged("area", area);
}

int AreaItemModel::count() const
{
    return m_nodes.count();
}

NodeItemModel *AreaItemModel::at(int i)
{
    return m_nodes.at(i);
}

void AreaItemModel::addNode(NodeItemModel *node)
{
    m_nodes.append(node);
}

void AreaItemModel::removeNode(NodeItemModel *node, bool deleteIt)
{
    m_nodes.removeAll(node);
    if (deleteIt)
        delete node;
}

NodeItemModel::NodeItemModel(int nodeId, int phyX, int phyY, int range):
    ItemModel()
    , m_nodeid(nodeId)
    , m_phyX(phyX)
    , m_phyY(phyY)
    , m_range(range)
{
}

NodeItemModel::NodeItemModel(QDeclarativeItem *item)
{
    m_nodeid = item->property("nodeId").toInt();
    m_phyX = item->property("x").toInt();
    m_phyY = item->property("y").toInt();
    m_range = 999;
    setObjectName(item->objectName());
}

int NodeItemModel::nodeId() const
{
    return m_nodeid;
}

void NodeItemModel::setNodeId(int nodeID)
{
    if (!rootName().isEmpty())
        rename(rootName() + "::Node"+QString::number(nodeID));
    else
        rename("::Node"+QString::number(nodeID));
    m_nodeid = nodeID;
    emit propertyChanged("nodeid", nodeID);
}

int NodeItemModel::phyX() const
{
    return m_phyX;
}

void NodeItemModel::setPhyX(int phyX)
{
    m_phyX = phyX;
    emit propertyChanged("phyX", phyX);
}

int NodeItemModel::phyY() const
{
    return m_phyY;
}

void NodeItemModel::setPhyY(int phyY)
{
    m_phyY = phyY;
    emit propertyChanged("phyY", phyY);
}

int NodeItemModel::phyZ() const
{
    return m_phyZ;
}

void NodeItemModel::setPhyZ(int phyZ)
{
    m_phyZ = phyZ;
    emit propertyChanged("phyZ", phyZ);
}

int NodeItemModel::range() const
{
    return m_range;
}

void NodeItemModel::setRange(int range)
{
    m_range = range;
    emit propertyChanged("range", range);
}

int NodeItemModel::count() const
{
    return m_sensors.count();
}

QString NodeItemModel::imgSrc() const
{
    return m_imgSrc;
}

void NodeItemModel::setImgSrc(const QString &src)
{
    m_imgSrc = src;
    emit propertyChanged("imgSrc", src);
}

SensorItemModel *NodeItemModel::at(int i)
{
    return m_sensors.at(i);
}

void NodeItemModel::addSensor(SensorItemModel *sensor)
{
    m_sensors.append(sensor);
}

void NodeItemModel::removeSensor(SensorItemModel *sensor, bool deleteIt)
{
    m_sensors.removeAll(sensor);
    if (deleteIt)
        delete sensor;
}


SensorItemModel::SensorItemModel(int sensorID,
                                 const QString &type, qreal x, qreal y, qreal rotation):
    ItemModel()
    , m_sensorid(sensorID)
    , m_type(type)
    , m_x(x)
    , m_y(y)
    , m_rotation(rotation)
{
}

SensorItemModel::SensorItemModel(QDeclarativeItem *item):
    ItemModel()
{
    m_sensorid = item->property("sensorId").toInt();
    m_type = item->property("type").toString();
    m_x = item->property("x").toReal();
    m_y = item->property("y").toReal();
    m_rotation = item->property("rotation").toReal();
    setObjectName(item->objectName());
}

int SensorItemModel::sensorId() const
{
    return m_sensorid;
}

void SensorItemModel::setSensorId(int sensorId)
{
    if (!rootName().isEmpty())
        rename(rootName() + "::Sensor"+type()+"-"+QString::number(sensorId));
    else
        rename("::Sensor"+type()+"-"+QString::number(sensorId));
    m_sensorid = sensorId;
    emit propertyChanged("sensorId", sensorId);
}

QString SensorItemModel::type() const
{
    return m_type;
}

void SensorItemModel::setType(const QString &type)
{
    m_type = type;
    emit propertyChanged("type", type);
}

qreal SensorItemModel::x() const
{
    return m_x;
}

void SensorItemModel::setX(qreal x)
{
    m_x = x;
    emit propertyChanged("x", x);
}

qreal SensorItemModel::y() const
{
    return m_y;
}

void SensorItemModel::setY(qreal y)
{
    m_y = y;
    emit propertyChanged("y", y);
}

qreal SensorItemModel::z() const
{
    return m_z;
}

void SensorItemModel::setZ(qreal z)
{
    m_z = z;
    emit propertyChanged("z", z);
}

qreal SensorItemModel::rotation() const
{
    return m_rotation;
}

void SensorItemModel::setRotation(qreal rotation)
{
    m_rotation = rotation;
    emit propertyChanged("rotation", rotation);
}

SensorsModel::SensorsModel(QObject *parent) :
    QObject(parent)
{

}

int SensorsModel::areasCount() const
{
    return m_areas.count();
}

int SensorsModel::nodesCount() const
{
    return m_nodes.count();
}

int SensorsModel::sensorsCount() const
{
    return m_sensors.count();
}

AreaItemModel * SensorsModel::areaAt(int i)
{
    return m_areas.at(i);
}

NodeItemModel * SensorsModel::nodeAt(int i)
{
    return m_nodes.at(i);
}

SensorItemModel * SensorsModel::sensorAt(int i)
{
    return m_sensors.at(i);
}

void SensorsModel::addArea(AreaItemModel *area)
{
    area->setParent(this);
    m_areas.append(area);
    connect(area, SIGNAL(propertyChanged(const char*,QVariant)),
            SLOT(observableItemChanged(const char*,QVariant)));
    emit areaAdded(area);
}

void SensorsModel::removeArea(AreaItemModel *area)
{
    emit areaRemoved(area);
    m_areas.removeAll(area);
    delete area;
}

void SensorsModel::addNode(NodeItemModel *node)
{
    node->setParent(this);
    m_nodes.append(node);
    connect(node, SIGNAL(propertyChanged(const char*,QVariant)),
            SLOT(observableItemChanged(const char*,QVariant)));
    emit nodeAdded(node);
}

void SensorsModel::removeNode(NodeItemModel *node)
{
    int areaId;

    m_nodes.removeAll(node);
    if (node->root()) {
        areaId = node->root()->property("area").toInt();
        for (int i = 0; i < areasCount(); i++)
            if (areaAt(i)->area() == areaId)
                areaAt(i)->removeNode(node, false);
    }
    for (int i = 0; i < node->count(); i++)
        node->at(i)->setRoot(NULL);
    emit nodeRemoved(node);
    delete node;
}

void SensorsModel::addSensor(SensorItemModel *item)
{
    item->setParent(this);
    connect(item, SIGNAL(propertyChanged(const char*,QVariant)),
            SLOT(observableItemChanged(const char*,QVariant)));
    m_sensors.append(item);
    emit sensorAdded(item);
}

void SensorsModel::removeSensor(SensorItemModel *item)
{
    m_sensors.removeAll(item);
    if (item->root()) {
        NodeItemModel *node = qobject_cast<NodeItemModel *>(item->root());
        node->removeSensor(item, false);
    }
    emit sensorRemoved(item);
    delete item;
}

void SensorsModel::observableItemChanged(const char *property, QVariant value)
{
    emit itemPropertyChanged(qobject_cast<ItemModel *>(sender()), property, value);
}

void SensorsModel::clear()
{
    foreach(SensorItemModel *sensor, m_sensors)
        emit sensorRemoved(sensor);
    foreach(NodeItemModel *node, m_nodes)
        emit nodeRemoved(node);
    foreach(AreaItemModel *area, m_areas)
        emit areaRemoved(area);
    m_sensors.clear();
    m_nodes.clear();
    m_areas.clear();
}
