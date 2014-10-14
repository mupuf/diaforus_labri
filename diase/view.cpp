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
#include "view.h"
#include <stdint.h>
#include <QtDeclarative/QtDeclarative>

#include "declarative/line.h"
#include "sensormodel.h"
#include "helpers.h"
#include "overlay.h"
#include <assert.h>


#define SHOW_OVERLAY 0
#define HIDE_OVERLAY 1

static void translateTo(QDeclarativeItem *walker, const QPoint &d)
{
    QPointF destination(d.x() - walker->width() / 2, d.y() - walker->height() / 2);
    qreal distance, duration, speed;

    distance = sqrt( (d.x() - walker->x()) * (d.x() - walker->x()) + (d.y() - walker->y()) * (d.y() - walker->y()) );
    speed = walker->property("speed").toReal() * 1000 / 3600;

    // 4 pixels per meter and speed m/s
    duration = (distance / 4) / speed;
    qDebug() << "duration" << duration << ", distance" << distance << ", speed" << speed;

    walker->setProperty("leftSide", (d.x() - walker->x()) > 0 ? false : true);
    walker->setProperty("destination", destination);
    walker->setProperty("duration", duration * 1000);
    walker->setProperty("state", "moving");
}

View::View(QObject *parent, QDeclarativeView *view, bool editable, bool deployment) :
    QObject(parent)
    , m_view(view)
    , m_globalRangeActivated(false)
{
    m_view->setSource(QUrl("qrc:/qml/diase/main.qml"));
    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);
    m_view->rootObject()->setProperty("editable", editable);
    m_view->rootObject()->setProperty("deployment", deployment);
    m_view->rootContext()->setContextProperty("app", this);

    connect(m_view->rootObject(), SIGNAL(qmlItemCreated(QVariant)), SLOT(qmlItemCreated(QVariant)));
    connect(m_view->rootObject(), SIGNAL(reset()), SLOT(restartScenario()));
    connect(m_view->rootObject(), SIGNAL(startAnimation()), SLOT(startAnimation()));
    connect(m_view->rootObject(), SIGNAL(remove(QVariant)), SLOT(removeItem(QVariant)));
}

QVariant View::transformCoordinates(QVariant item, qreal x, qreal y)
{
    QDeclarativeItem *qmlItem;
    QPointF ret, foo;

    Q_UNUSED(x);
    Q_UNUSED(y);

    qmlItem = qobject_cast<QDeclarativeItem *>(item.value<QObject *>());
    ret = qmlItem->pos();
    qDebug() << "pos" << ret;
    qmlItem = loadQMLComponent("Intruder");
    qmlItem->setProperty("color", "green");
    qmlItem->setProperty("x", ret.x());
    qmlItem->setProperty("y", ret.y());

    foo = qmlItem->mapFromParent(ret);
    qDebug() << "mapToParent" << foo;
    qmlItem = loadQMLComponent("Intruder");
    qmlItem->setProperty("x", foo.x());
    qmlItem->setProperty("y", foo.y());
    return foo;
}

void View::assignRoute(QDeclarativeItem *walker)
{
    quint32 distance = 4294967295U, tmp = 0;
    Line *selectedPath = NULL;
    QDeclarativeItem *contentItemObj;
    QList<QVariant> route;

    contentItemObj = contentItem();

    // Find the nearest path
    foreach(QObject *obj, contentItemObj->children()) {
        if (obj->property("className") == "Path") {
            Line *line = qobject_cast<Line *>(obj);
            tmp = sqrt( (line->x1() - walker->x()) * (line->x1() - walker->x()) + (line->y1() - walker->y()) * (line->y1() - walker->y()) );

            if (tmp <= distance) {
                distance = tmp;
                selectedPath = line;
            }
        }
    }

    route << QPoint(selectedPath->x1(), selectedPath->y1());

    // Construct the route
    foreach(QObject *obj, contentItemObj->children()) {
        if ((obj->property("className") == "Path") && (obj != selectedPath)) {
            Line *line = qobject_cast<Line *>(obj);
            if (QPoint(selectedPath->x2(), selectedPath->y2()) == QPoint(line->x1(), line->y1())) {
                route << QPoint(line->x1(), line->y1());
                selectedPath = line;
            }
        }
    }
    route << QPoint(selectedPath->x2(), selectedPath->y2());
    walker->setProperty("route", route);
}

void View::removeItem(QVariant item)
{
    emit remove(qobject_cast<QDeclarativeItem *>(item.value<QObject *>()));
}

void View::startAnimation()
{
    QDeclarativeItem *contentItem;
    QList<QVariant> route;
    QPoint currPoint;

    contentItem = this->contentItem();

    foreach(QDeclarativeItem *walker, contentItem->findChildren<QDeclarativeItem *>(QString::fromLatin1("intruder"))) {

        if (walker->property("route").toList().isEmpty())
            assignRoute(walker);

        route = walker->property("route").toList();
        currPoint = walker->property("currRoutePoint").toPoint();

        if (currPoint.isNull()) {
            currPoint = route.at(0).toPoint();
            translateTo(walker, currPoint);
            walker->setProperty("currRoutePoint", currPoint);
            continue;
        }

        if (currPoint == route.last().toPoint())
            break;

        for (int i = 0; i < route.count(); i++) {
            if (currPoint == route.at(i).toPoint()) {
                currPoint = route.at(i+1).toPoint();
                translateTo(walker, currPoint);
                walker->setProperty("currRoutePoint", currPoint);
                break;
            }
        }
    }
}

void View::checkCollisions(QVariant obj)
{
    QDeclarativeItem *contentItem, *walker;
    QDeclarativeItem *item;
    bool concurrent = false;

    walker = qobject_cast<QDeclarativeItem *>(obj.value<QObject *>());
    contentItem = this->contentItem();

    foreach(QObject *child, contentItem->children()) {

        item = qobject_cast<QDeclarativeItem *>(child);

        if(child->property("className") == "Sensor") {

            if (walker->collidesWithItem(item)) {
                QPointF pos = walker->pos();
                pos.setX(pos.x() + walker->width() / 2);
                pos.setY(pos.y() + walker->height() / 2);
                QPointF itemPos = item->mapFromParent(pos);

                QMetaObject::invokeMethod(child, "collision", Qt::DirectConnection, Q_ARG(bool, true), Q_ARG(int, itemPos.x()), Q_ARG(int, itemPos.y()));
            } else {
                // If another intruder collide with this item, don't cancel its collision
                foreach(QObject *c, contentItem->children()) {
                    if (isIntruder(c) && (c != walker) && qobject_cast<QDeclarativeItem *>(c)->collidesWithItem(item))
                        concurrent = true;
                }
                if (!concurrent)
                    QMetaObject::invokeMethod(child, "collision", Qt::DirectConnection, Q_ARG(bool, false), Q_ARG(int, 0), Q_ARG(int, 0));
                concurrent = false;
            }
        }
    }
}

void View::setModel(SensorsModel *model)
{
    m_model = model;

    connect(model, SIGNAL(areaAdded(AreaItemModel*)), SLOT(addArea(AreaItemModel*)));
    connect(model, SIGNAL(nodeAdded(NodeItemModel*)), SLOT(addNode(NodeItemModel*)));
    connect(model, SIGNAL(sensorAdded(SensorItemModel*)), SLOT(addSensor(SensorItemModel*)));
    connect(model, SIGNAL(itemPropertyChanged(ItemModel*,const char*,QVariant)),
            SLOT(itemModelPropertyChanged(ItemModel*,const char*,QVariant)));
    connect(model, SIGNAL(areaRemoved(AreaItemModel*)), SLOT(removeArea(AreaItemModel*)));
    connect(model, SIGNAL(nodeRemoved(NodeItemModel*)), SLOT(removeNode(NodeItemModel*)));
    connect(model, SIGNAL(sensorRemoved(SensorItemModel*)), SLOT(removeSensor(SensorItemModel*)));
}

void View::sensorClicked()
{
    QDeclarativeItem *sensor;
    int modelIndex;

    sensor = qobject_cast<QDeclarativeItem *>(sender());
    modelIndex = sensor->property("modelIndex").toInt();

    emit sensorItemClicked(m_model->sensorAt(modelIndex));
}

void View::nodeClicked()
{
    QDeclarativeItem *node;
    int modelIndex;

    node = qobject_cast<QDeclarativeItem *>(sender());
    modelIndex = node->property("modelIndex").toInt();

    emit nodeItemClicked(m_model->nodeAt(modelIndex));
}

void View::moveSensorTo(int x, int y)
{
    QDeclarativeItem *sensor;
    SensorItemModel *sensorItem;
    QPointF pos;
    int modelIndex;

    // Compute the new position according to the parent's
    // system coordinates (contentItem is the parent of all items in the scene)
    sensor = qobject_cast<QDeclarativeItem *>(sender());
    pos = sensor->mapToParent(x, y);
    pos.setX(pos.x() - sensor->width() / 2);
    pos.setY(pos.y() - sensor->height()  / 2);
    sensor->setPos(pos);

    // Refresh the model and notify other views
    modelIndex = sensor->property("modelIndex").toInt();
    sensorItem = m_model->sensorAt(modelIndex);
    sensorItem->setX(pos.x());
    sensorItem->setY(pos.y());
}

void View::moveNodeTo(int x, int y)
{
    QDeclarativeItem *node;
    NodeItemModel *nodeItem;
    QPointF pos;
    int modelIndex;

    // Compute the new position according to the parent's
    // system coordinates (contentItem is the parent of all items in the scene)
    node = qobject_cast<QDeclarativeItem *>(sender());

    // Refresh the model and notify other views
    modelIndex = node->property("modelIndex").toInt();
    nodeItem = m_model->nodeAt(modelIndex);
    pos = node->mapToParent(x, y);
    nodeItem->setPhyX(pos.x());
    nodeItem->setPhyY(pos.y());
    rangeOverlayForNode(node, false);
}

void View::mouseAcquiredBySensorOrNode(bool acquired)
{
    QDeclarativeItem *mapView;

    mapView = m_view->rootObject()->findChild<QDeclarativeItem *>(QString::fromLatin1("mapView"));
    mapView->setProperty("interactive", !acquired);
}

bool View::isDeploymentView() const
{
    return m_view->rootObject()->property("deployment").toBool();
}

bool View::isScenarioView() const
{
    return m_view->rootObject()->property("editable").toBool();
}

bool View::isC2View() const
{
    return !isDeploymentView() && !isScenarioView();
}

void View::addIntruder(const QString &type, int x, int y)
{
    QDeclarativeItem *item;

    item = loadQMLComponent("Intruder");

    if (!item)
        return;
    if (type == "car") {
        item->setProperty("skin", "qrc:/icons/car-black.png");
        item->setProperty("width", 32);
        item->setProperty("height", 16);
        item->setProperty("speed", 50);
    }
    item->setProperty("x", x);
    item->setProperty("y", y);
    qmlItemCreated(QVariant::fromValue(qobject_cast<QObject *>(item)));
}

QList< QPair<QString, QPoint> > View::intruders() const
{
    QList< QPair<QString, QPoint> > list;

    foreach(QObject *obj, contentItem()->children()) {
        if (obj->property("className") == "Intruder") {
            QPair<QString, QPoint> p;

            if (obj->property("skin") == "qrc:/icons/intruder-black.png")
                p.first = "walker";
            else
                p.first = "car";
            p.second = QPoint(obj->property("x").toInt(), obj->property("y").toInt());
            list << p;
        }
    }
    return list;
}

void View::addPath(int x1, int y1, int x2, int y2)
{
    QDeclarativeItem *item;

    item = loadQMLComponent("Path");

    if (!item)
        return;
    item->setProperty("x1", x1);
    item->setProperty("y1", y1);
    item->setProperty("x2", x2);
    item->setProperty("y2", y2);
}

QList<QLine> View::paths() const
{
    QList<QLine> list;

    foreach(QObject *obj, contentItem()->children())
        if (obj->property("className") == "Path")
            list << QLine(obj->property("x1").toInt(), obj->property("y1").toInt(), obj->property("x2").toInt(), obj->property("y2").toInt());
    return list;
}

void View::addArea(AreaItemModel *area, bool update)
{
    QDeclarativeItem *areaItem = NULL;
    NodeItemModel *node;
    SensorItemModel *sensor;
    QDeclarativeItem *nodeItem, *sensorItem, *nodeLabel;
    QRectF boudingBox;
    int x, y, i, j;
    int xMin = INT_MAX, xMax = -1, yMin = INT_MAX, yMax = -1;

    if (update) {
        areaItem = contentItem()->findChild<QDeclarativeItem *>(area->objectName());
    } else {
        areaItem = loadQMLComponent("Area");
    }

    if (!areaItem)
        return;

    // For each node and sensor within a specific area, find the ones with the
    // smallest coordinates and the bigger coordinates.
    // These pairs of smallest and bigger coordinates
    // are the area geometry.
    for (i = 0; i < area->count(); i++) {
        node = area->at(i);
        nodeItem = contentItem()->findChild<QDeclarativeItem *>(node->objectName());

        if (!nodeItem) {
            qDebug() << "WARNING: addArea: found an invalid nodeItem for" << node->objectName();
            return;
        }
	nodeLabel = nodeItem->findChild<QDeclarativeItem *>("label");
        x = nodeItem->pos().x();
        y = nodeItem->pos().y();

        xMin = x <= xMin ? x : xMin;
        xMax = x+nodeItem->width()+nodeLabel->width() >= xMax ? x+nodeItem->width()+nodeLabel->width() : xMax;
        yMin = y <= yMin ? y : yMin;
        yMax = y+nodeItem->height()+nodeLabel->height() >= yMax ? y+nodeItem->height()+nodeLabel->height() : yMax;

        for (j = 0; j < node->count(); j++) {
            sensor = node->at(j);
            sensorItem = contentItem()->findChild<QDeclarativeItem *>(sensor->objectName());

            if (!sensorItem) {
                qDebug() << "WARNING: addArea: found an invalid sensorItem for" << sensor->objectName();
                return;
            }

            // Get back the bouding box for this item after the transformation has been applied
            boudingBox = sensorItem->mapToParent(0, 0, sensorItem->width(), sensorItem->height()).boundingRect();
            x = boudingBox.x();
            y = boudingBox.y();

            xMin = x <= xMin ? x : xMin;
            xMax = x+boudingBox.width() >= xMax ? x+boudingBox.width() : xMax;
            yMin = y <= yMin ? y : yMin;
            yMax = y+boudingBox.height() >= yMax ? y+boudingBox.height() : yMax;
        }
    }
    areaItem->setObjectName("Area"+ QString::number(area->area()));
    areaItem->setProperty("name", "Area " + QString::number(area->area()));
    areaItem->setProperty("x", xMin - 20);
    areaItem->setProperty("y", yMin - 20);
    areaItem->setProperty("width", (xMax-xMin) + 40);
    areaItem->setProperty("height", (yMax-yMin) + 40);
    areaItem->setProperty("modelIndex", m_model->areasCount() - 1);

    if (isC2View()) {
        connect(areaItem, SIGNAL(ack(QVariant)), SLOT(ack(QVariant)));
    }
    // Only activate Area acknowlegment on the C2 View
    areaItem->setProperty("ackEnabled", isC2View());
}


void View::removeArea(AreaItemModel *area)
{
    QDeclarativeItem *item;

    item = contentItem()->findChild<QDeclarativeItem *>(area->objectName());
    if (item)
        delete item;
}

void View::addNode(NodeItemModel *node)
{
    QDeclarativeItem *nodeItem;
    AreaItemModel *area;

    nodeItem = loadQMLComponent("Node");

    if (!nodeItem)
        return;
    area = qobject_cast<AreaItemModel *>(node->root());

    if (area)
        nodeItem->setObjectName(area->objectName()+"::Node"+QString::number(node->nodeId()));
    else
        nodeItem->setObjectName("::Node"+QString::number(node->nodeId()));
    assert(nodeItem->parentItem() == this->contentItem());

    nodeItem->setPos(node->phyX() - nodeItem->width() / 2, node->phyY() - nodeItem->height() / 2);
    nodeItem->setProperty("modelIndex", m_model->nodesCount() - 1);
    nodeItem->setProperty("imgSrc", node->imgSrc());
    nodeItem->setProperty("nodeId", QString::number(node->nodeId()));

    if (isDeploymentView()) {
        nodeItem->setProperty("z", 2);
        connect(nodeItem, SIGNAL(mouseEntered()), SLOT(nodeItemMouseEntered()));
        connect(nodeItem, SIGNAL(mouseExited()), SLOT(nodeItemMouseExited()));
        connect(nodeItem, SIGNAL(moveTo(int, int)), SLOT(moveNodeTo(int,int)));
        connect(nodeItem, SIGNAL(mouseAcquired(bool)), SLOT(mouseAcquiredBySensorOrNode(bool)));
        connect(nodeItem, SIGNAL(nodeClicked()), SLOT(nodeClicked()));
    }
}

void View::removeNode(NodeItemModel *node)
{
    QDeclarativeItem *item;

    item = contentItem()->findChild<QDeclarativeItem *>(node->objectName());
    if (item)
        delete item;
}

void View::addSensor(SensorItemModel *item)
{
    QDeclarativeItem *instance;
    NodeItemModel *node;

    instance = loadQMLComponent(item->type());

    if (!instance)
        return;
    node = qobject_cast<NodeItemModel *>(item->root());

    if(node)
        instance->setObjectName(node->objectName()+"::Sensor"+item->type() + "-" + QString::number(item->sensorId()));
    else
        instance->setObjectName("::Sensor"+item->type() + "-" + QString::number(item->sensorId()));
    instance->setPos(item->x(), item->y());

    instance->setProperty("modelIndex", m_model->sensorsCount() - 1);
    instance->setProperty("rotation", item->rotation());

    if (isDeploymentView()) {
        connect(instance, SIGNAL(moveTo(int, int)), SLOT(moveSensorTo(int, int)));
        connect(instance, SIGNAL(mouseAcquired(bool)), SLOT(mouseAcquiredBySensorOrNode(bool)));
        connect(instance, SIGNAL(sensorClicked()), SLOT(sensorClicked()));
    }
    emit addedSensortoView(instance);
}

void View::removeSensor(SensorItemModel *sensor)
{
    QDeclarativeItem *item;

    item = contentItem()->findChild<QDeclarativeItem *>(sensor->objectName());
    if (item)
        delete item;
}

void View::qmlItemCreated(QVariant obj)
{
    QDeclarativeItem *item;

    item = qobject_cast<QDeclarativeItem *>( obj.value<QObject *>() );

    if (!item)
        return;

    if (item->property("className") == "Intruder") {
        connect(item, SIGNAL(startAnimation()), SLOT(startAnimation()));
        connect(item, SIGNAL(hasMoved(QVariant)), SLOT(checkCollisions(QVariant)));
    }
    emit itemCreated(item);
}

void View::restartScenario()
{
    QDeclarativeItem *contentItemObj;

    contentItemObj = contentItem();
    foreach(QObject *obj, contentItemObj->children()) {
        if (obj->property("className") == "Path" || obj->property("className") == "Area")
            continue;
        obj->setProperty("state", "");

        if (obj->property("className") == "Intruder")
            obj->setProperty("currRoutePoint", QPoint());
    }
}

void View::ack(QVariant obj)
{
    QDeclarativeItem *contentItemObj, *item;
    AreaItemModel *areaItem;
    int i, j;

    contentItemObj = contentItem();
    item = qobject_cast<QDeclarativeItem *>(obj.value<QObject *>());


    areaItem = m_model->areaAt(item->property("modelIndex").toInt());
    foreach (QObject *obj, contentItemObj->children()) {
        if (obj->objectName() == areaItem->objectName()) {
            obj->setProperty("state", "");
        }
    }
    for (i = 0; i < areaItem->count(); i++) {
        NodeItemModel *nodeItem = areaItem->at(i);
        for (j = 0; j < nodeItem->count(); j++) {
            SensorItemModel *sensorItem = nodeItem->at(j);
            foreach (QObject *obj, contentItemObj->children()) {
                if (obj->objectName() == nodeItem->objectName()) {
                    obj->setProperty("state", "");
                }
                else if (obj->objectName() == sensorItem->objectName()) {
                    obj->setProperty("state", "");
                }
            }
        }
    }
}

void View::nodeItemMouseEntered()
{
    rangeOverlay(SHOW_OVERLAY, QStringList() << sender()->objectName());
}

void View::nodeItemMouseExited()
{
    rangeOverlay(HIDE_OVERLAY, QStringList() << sender()->objectName());
}

void View::itemModelPropertyChanged(ItemModel *item, const char *property, QVariant value)
{
    QDeclarativeItem *contentItem;
    bool editable = false, deployment = false;

    contentItem = this->contentItem();

    editable = m_view->rootObject()->property("editable").toBool();
    deployment = m_view->rootObject()->property("deployment").toBool();

    if (QString::fromLatin1(property) == "state") {
        if (editable || deployment)
            return;
    }
    foreach(QObject *child, contentItem->children()) {
        if (child->objectName() == item->objectName()) {
            child->setProperty(property, value);
        }
    }
    // Coordinates for @item have changed, we need to recompute the area
    if (QString::fromLatin1(property) == "x" || QString::fromLatin1(property) == "y") {
        AreaItemModel *areaItem = NULL;
        NodeItemModel *nodeItem = NULL;
        SensorItemModel *sensorItem = qobject_cast<SensorItemModel *>(item);

        nodeItem = qobject_cast<NodeItemModel *>(sensorItem->root());
        if (nodeItem)
            areaItem = qobject_cast<AreaItemModel *>(nodeItem->root());
        if (areaItem)
            addArea(areaItem, true);
    } else if (QString::fromLatin1(property) == "phyX" || QString::fromLatin1(property) == "phyY") {
        AreaItemModel *areaItem;
        QDeclarativeItem *node;
        NodeItemModel *nodeItem = qobject_cast<NodeItemModel *>(item);

        areaItem = qobject_cast<AreaItemModel *>(nodeItem->root());
        node = contentItem->findChild<QDeclarativeItem *>(nodeItem->objectName());
        node->setProperty(QString(property) == "phyX" ? "x": "y", QString(property) == "phyX" ? value.toInt() - node->width() / 2 : value.toInt() - node->height() / 2);

        if (areaItem)
            addArea(areaItem, true);
    } else if (QString::fromLatin1(property) == "rootName") {
        // Refresh all areas
        for (int i = 0; i < m_model->areasCount(); i++)
            addArea(m_model->areaAt(i), true);
    }
}

QDeclarativeItem *View::contentItem() const
{
    QDeclarativeItem *view;

    view = m_view->rootObject()->findChild<QDeclarativeItem *>(QString::fromLatin1("mapView"));
    return qvariant_cast<QDeclarativeItem *>(view->property("contentItem"));
}

qreal View::contentX() const
{
    return m_view->rootObject()->findChild<QDeclarativeItem *>("mapView")->property("contentX").toReal();
}

void View::setContentX(qreal x)
{
    QDeclarativeItem *mapView;

    mapView = m_view->rootObject()->findChild<QDeclarativeItem *>(QString::fromLatin1("mapView"));
    mapView->setProperty("contentX", x);
}

qreal View::contentY() const
{
    return m_view->rootObject()->findChild<QDeclarativeItem *>("mapView")->property("contentY").toReal();
}

void View::setContentY(qreal y)
{
    QDeclarativeItem *mapView;

    mapView = m_view->rootObject()->findChild<QDeclarativeItem *>(QString::fromLatin1("mapView"));
    mapView->setProperty("contentY", y);
}


void View::addOverlay(Overlay *overlay)
{
    if (!m_externalOverlays.contains(overlay->objectName())) {
        m_externalOverlays[overlay->objectName()] = overlay;
        overlay->addView(m_view);
    }
}

void View::showOverlay(const QString &name)
{
    bool success = false;

    success = QMetaObject::invokeMethod(this,
                                        QString(name+"Overlay").toLocal8Bit().data(),
                                        Qt::DirectConnection,
                                        Q_ARG(int, SHOW_OVERLAY));
    if (!success) {

        if (m_externalOverlays.contains(name))
            m_externalOverlays[name]->showOverlay();
        else
            qWarning() << "Error: No such overlay" << name;
    }
}

void View::hideOverlay(const QString &name)
{
    bool success = false;

    success = QMetaObject::invokeMethod(this,
                                        QString(name+"Overlay").toLocal8Bit().data(),
                                        Qt::DirectConnection,
                                        Q_ARG(int, HIDE_OVERLAY));
    if (!success) {
        if (m_externalOverlays.contains(name))
            m_externalOverlays[name]->hideOverlay();
        else
            qWarning() << "Error: No such overlay" << name;
    }
}

void View::sensorOverlay(int mode, const QStringList &nodes)
{
    QDeclarativeItem *contentItemObj, *item;

    Q_UNUSED(nodes);

    contentItemObj = contentItem();
    switch(mode) {
        case SHOW_OVERLAY:
            foreach(QObject *obj, contentItemObj->children()) {
                if (obj->property("className") == "Sensor") {
                    item = qobject_cast<QDeclarativeItem *>(obj);
                    item->show();
                }
            }
            break;
        case HIDE_OVERLAY:
            foreach(QObject *obj, contentItemObj->children()) {
                if (obj->property("className") == "Sensor") {
                    item = qobject_cast<QDeclarativeItem *>(obj);
                    item->hide();
                }
            }
            break;
    }
}

void View::areaOverlay(int mode, const QStringList &nodes)
{
    QDeclarativeItem *contentItemObj;

    Q_UNUSED(nodes);

    contentItemObj = contentItem();
    switch(mode) {
        case SHOW_OVERLAY:
            foreach(QObject *obj, contentItemObj->children()) {
                QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(obj);
                if (item->property("className").toString() == "Area")
                    item->show();
            }
            break;
        case HIDE_OVERLAY:
            foreach(QObject *obj, contentItemObj->children()) {
                QDeclarativeItem *item = qobject_cast<QDeclarativeItem *>(obj);
                if (item->property("className").toString() == "Area")
                    item->hide();
            }
            break;
    }
}

void View::rangeOverlay(int mode, const QStringList &nodes)
{
    QDeclarativeItem *contentItemObj, *item;

    contentItemObj = contentItem();
    switch(mode) {
    case SHOW_OVERLAY:

        // If nodes is a non-empty list, use a suboverlay
        if (!nodes.isEmpty()) {

            if (m_globalRangeActivated)
                return;

            foreach (QString node, nodes) {
                foreach(QDeclarativeItem *nodeItem, contentItemObj->findChildren<QDeclarativeItem*>(node)) {
                    if (nodeItem->property("className") == "Node")
                        rangeOverlayForNode(nodeItem);
                }
            }
            return;
        }

        foreach(QObject *sender, contentItemObj->children()) {
            if (sender->property("className") != "Node")
                continue;
            rangeOverlayForNode(qobject_cast<QDeclarativeItem *>(sender));
        }
        m_globalRangeActivated = true;
        break;
    case HIDE_OVERLAY:
        if (!nodes.isEmpty()) {

            if (m_globalRangeActivated)
                return;

            foreach (QString node, nodes) {
                QObject *nodeItem = contentItemObj->findChild<QObject *>(node);
                NodeItemModel *nodeItemModel = m_model->nodeAt(nodeItem->property("modelIndex").toInt());
                QString id = QString::number(nodeItemModel->nodeId());

                foreach(QObject *obj, contentItemObj->children()) {
                    QString name = obj->objectName();
                    if (obj->property("className") == "Range"
                        && (name.contains("-"+id) || name.contains(id+"-"))) {
                        item = qobject_cast<QDeclarativeItem *>(obj);
                        item->hide();
                    }
                }
            }
            return;
        }

        foreach(QObject *obj, contentItemObj->children()) {
            item = qobject_cast<QDeclarativeItem *>(obj);
            if (item->property("className").toString() == "Range")
                item->hide();
        }
        m_globalRangeActivated = false;
        break;
    }
}

void View::rangeOverlayForNode(QDeclarativeItem *nodeItem, bool useCache)
{
    QDeclarativeItem *contentItemObj, *range;
    NodeItemModel *nodeItemModel, *currNode;

    contentItemObj = contentItem();
    nodeItemModel = m_model->nodeAt(nodeItem->property("modelIndex").toInt());

    for(int i = 0; i < m_model->nodesCount(); i++) {
        currNode = m_model->nodeAt(i);

        if (currNode == nodeItemModel)
            continue;

        int senderId = nodeItemModel->nodeId();
        int targetId = currNode->nodeId();
        int nodeIdMin = qMin(senderId, targetId);
        int nodeIdMax = qMax(senderId, targetId);
        QString RangeName = QString::number(nodeIdMin)+"-"+QString::number(nodeIdMax);

        if (useCache) {
            if ((range = contentItemObj->findChild<QDeclarativeItem*>(RangeName)) != NULL) {
                range->show();
                continue;
            }
            range = loadQMLComponent("Range");
        } else {
            if ((range = contentItemObj->findChild<QDeclarativeItem*>(RangeName)) == NULL) {
                range = loadQMLComponent("Range");
            }
        }

        int sender_x = nodeItemModel->phyX();
        int sender_y = nodeItemModel->phyY();
        int target_x = currNode->phyX();
        int target_y = currNode->phyY();
        int sender_range = nodeItemModel->range();
        int target_range = currNode->range();
        int minRange = qMin(sender_range, target_range);
        float distance = sqrt((sender_x - target_x) * (sender_x - target_x) + (sender_y - target_y) * (sender_y - target_y));
        float ratio = distance / minRange;

        if (distance < minRange) {

            range->setObjectName(RangeName);
            range->setProperty("x1", sender_x);
            range->setProperty("y1", sender_y);
            range->setProperty("x2", target_x);
            range->setProperty("y2", target_y);
            range->setProperty("color", QColor(ratio*255, (1-ratio) * 255, 0));
            range->setProperty("distance", QString::number(ratio * 100, 'g', 4) + "%");
        }
    }
}

void View::nodesOverlay(int mode, const QStringList &nodes)
{
    QDeclarativeItem *contentItemObj, *item;

    Q_UNUSED(nodes);

    contentItemObj = contentItem();
    switch(mode) {
        case SHOW_OVERLAY:
            foreach(QObject *obj, contentItemObj->children()) {
                item = qobject_cast<QDeclarativeItem *>(obj);
                if (obj->property("className") == "Node") {
                    item->show();
                }
            }
            break;
        case HIDE_OVERLAY:
            foreach(QObject *obj, contentItemObj->children()) {
                item = qobject_cast<QDeclarativeItem *>(obj);
                if (obj->property("className") == "Node") {
                    item->hide();
                }
            }
            break;
    }
}

QDeclarativeItem *View::loadQMLComponent(const QString &componentName)
{
    QDeclarativeEngine *engine;
    QDeclarativeItem *instance, *contentItemObj;

    engine = m_view->engine();
    contentItemObj = contentItem();
    QDeclarativeComponent component(engine, QString::fromLatin1("qml/diase/") + componentName + QString::fromLatin1(".qml"));

    if (component.status() == QDeclarativeComponent::Error) {
        foreach(QDeclarativeError error, component.errors())
            qWarning() << "QML error: " << error.description() << "[component=" << componentName << "]";
        return NULL;
    }

    instance = qobject_cast<QDeclarativeItem *>(component.create());
    if (!instance) {
        qWarning() << "QML error: Unable instanciate object from component" << "[component=" << componentName << "]";
        return NULL;
    }
    instance->setParentItem(contentItemObj);
    instance->setParent(contentItemObj);

    return instance;
}
