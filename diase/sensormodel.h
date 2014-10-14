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
#ifndef ITEMMODEL_H
#define ITEMMODEL_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QVariant>

class QDeclarativeItem;
class NodeItemModel;
class SensorItemModel;

class ItemModel: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ state WRITE setState NOTIFY propertyChanged)
    Q_PROPERTY(QString rootName READ rootName NOTIFY propertyChanged)

public:
    ItemModel *root() const;
    void setRoot(ItemModel *root);

    QString state() const;
    void setState(const QString &state);

    void rename(const QString &name);
    QString rootName() const;

protected:
    ItemModel(ItemModel *root = 0);

Q_SIGNALS:
    void propertyChanged(const char *property, QVariant value);

private:
    virtual bool event(QEvent *e);

private:
    ItemModel *m_root;
    QString m_state;
    QString m_rootName;
};

class AreaItemModel: public ItemModel
{
    Q_OBJECT
    Q_PROPERTY(int area READ area WRITE setArea)

public:
    AreaItemModel(int area);

    int area() const;
    void setArea(int area);

    int count() const;
    NodeItemModel *at(int i);

    void addNode(NodeItemModel *node);
    void removeNode(NodeItemModel *node, bool deleteIt = true);

private:
    int m_area;
    QList<NodeItemModel *> m_nodes;
};


class NodeItemModel: public ItemModel
{
    Q_OBJECT
    Q_PROPERTY(int nodeId READ nodeId WRITE setNodeId)
    Q_PROPERTY(int phyX READ phyX WRITE setPhyX)
    Q_PROPERTY(int phyY READ phyY WRITE setPhyY)
    Q_PROPERTY(int phyZ READ phyZ WRITE setPhyZ)
    Q_PROPERTY(int range READ range WRITE setRange)
    Q_PROPERTY(QString imgSrc READ imgSrc WRITE setImgSrc)

public:
    NodeItemModel(int nodeId, int phyX, int phyY, int range);
    NodeItemModel(QDeclarativeItem *item);

    int nodeId() const;
    void setNodeId(int nodeID);

    int phyX() const;
    void setPhyX(int phyX);

    int phyY() const;
    void setPhyY(int phyY);

    int phyZ() const;
    void setPhyZ(int phyZ);

    int range() const;
    void setRange(int range);

    int count() const;
    SensorItemModel *at(int i);

    QString imgSrc() const;
    void setImgSrc(const QString &src);

    void addSensor(SensorItemModel *sensor);
    void removeSensor(SensorItemModel *sensor, bool deleteIt = true);

private:
    int m_nodeid;
    int m_phyX;
    int m_phyY;
    int m_phyZ;
    int m_range;
    QString m_imgSrc;
    QList<SensorItemModel *> m_sensors;
};



class SensorItemModel: public ItemModel
{
    Q_OBJECT
    Q_PROPERTY(int sensorId READ sensorId WRITE setSensorId)
    Q_PROPERTY(QString type READ type WRITE setType)
    Q_PROPERTY(qreal x READ x WRITE setX)
    Q_PROPERTY(qreal y READ y WRITE setY)
    Q_PROPERTY(qreal z READ z WRITE setZ)
    Q_PROPERTY(qreal rotation READ rotation WRITE setRotation)

public:
    SensorItemModel(QDeclarativeItem *item);
    SensorItemModel(int sensorID, const QString &type, qreal x, qreal y, qreal rotation = 0);

    int sensorId() const;
    void setSensorId(int sensorId);

    QString type() const;
    void setType(const QString &type);

    qreal x() const;
    void setX(qreal x);

    qreal y() const;
    void setY(qreal y);

    qreal z() const;
    void setZ(qreal z);

    qreal rotation() const;
    void setRotation(qreal rotation);

private:
    int m_sensorid;
    QString m_type;
    qreal m_x;
    qreal m_y;
    qreal m_z;
    qreal m_rotation;
};

class SensorsModel : public QObject
{
    Q_OBJECT
public:
    explicit SensorsModel(QObject *parent = 0);

    int areasCount() const;
    int nodesCount() const;
    int sensorsCount() const;

    AreaItemModel * areaAt(int i);
    NodeItemModel * nodeAt(int i);
    SensorItemModel * sensorAt(int i);

public Q_SLOTS:
    void addArea(AreaItemModel *area);
    void removeArea(AreaItemModel *area);

    void addNode(NodeItemModel *node);
    void removeNode(NodeItemModel *node);

    void addSensor(SensorItemModel *item);
    void removeSensor(SensorItemModel *item);

    void clear();

Q_SIGNALS:
    void areaAdded(AreaItemModel *area);
    void areaRemoved(AreaItemModel *area);

    void nodeAdded(NodeItemModel *node);
    void nodeRemoved(NodeItemModel *node);

    void sensorAdded(SensorItemModel *item);
    void sensorRemoved(SensorItemModel *item);

    void itemPropertyChanged(ItemModel *item, const char *property, QVariant value);

private Q_SLOTS:
    void observableItemChanged(const char *property, QVariant value);

private:
    QList<AreaItemModel *> m_areas;
    QList<NodeItemModel *> m_nodes;
    QList<SensorItemModel *> m_sensors;
};

#endif // ITEMMODEL_H
