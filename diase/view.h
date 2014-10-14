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
#ifndef VIEW_H
#define VIEW_H

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QLine>
#include <QtCore/QPoint>
#include <QtCore/QMap>

class QDeclarativeView;
class QDeclarativeItem;
class SensorsModel;
class ItemModel;
class AreaItemModel;
class NodeItemModel;
class SensorItemModel;
class Overlay;

/*
 * The class View is the native representation of all the QML views (except monitoring). Basically,
 * it handles everything about drawing, overlays, intrusions.
 */
class View : public QObject
{
    Q_OBJECT
public:

    explicit View(QObject *parent, QDeclarativeView *view, bool editable = false, bool deployment = false);

    void setModel(SensorsModel *model);

    void setContentX(qreal x);
    qreal contentX() const;

    void setContentY(qreal y);
    qreal contentY() const;

    void addIntruder(const QString &type, int x, int y);
    QList< QPair<QString, QPoint> > intruders() const;

    void addPath(int x1, int y1, int x2, int y2);
    QList<QLine> paths() const;

    void addOverlay(Overlay *overlay);
    void showOverlay(const QString &name);
    void hideOverlay(const QString &name);

    Q_INVOKABLE QVariant transformCoordinates(QVariant item, qreal x, qreal y);

public Q_SLOTS:
    void itemModelPropertyChanged(ItemModel *item, const char *property, QVariant value);

Q_SIGNALS:
    void itemCreated(QDeclarativeItem *item);
    void addedSensortoView(QDeclarativeItem *item);
    void sensorItemClicked(SensorItemModel *item);
    void nodeItemClicked(NodeItemModel *item);
    void remove(QDeclarativeItem *item);

private Q_SLOTS:
    void addArea(AreaItemModel *area, bool update = false);
    void removeArea(AreaItemModel *area);
    void addNode(NodeItemModel *node);
    void removeNode(NodeItemModel *node);
    void addSensor(SensorItemModel *sensor);
    void removeSensor(SensorItemModel *sensor);
    void qmlItemCreated(QVariant obj);
    void nodeItemMouseEntered();
    void nodeItemMouseExited();
    void restartScenario();
    void ack(QVariant obj);
    void startAnimation();
    void checkCollisions(QVariant obj);
    void moveSensorTo(int x, int y);
    void moveNodeTo(int x, int y);
    void mouseAcquiredBySensorOrNode(bool acquired);
    void sensorClicked();
    void nodeClicked();
    void removeItem(QVariant item);

private:
    Q_INVOKABLE void areaOverlay(int mode, const QStringList &nodes = QStringList());
    Q_INVOKABLE void rangeOverlay(int mode, const QStringList &nodes = QStringList());
    Q_INVOKABLE void nodesOverlay(int mode, const QStringList &nodes = QStringList());
    Q_INVOKABLE void sensorOverlay(int mode, const QStringList &nodes = QStringList());
    QDeclarativeItem *loadQMLComponent(const QString &componentName);
    QDeclarativeItem *contentItem() const;
    void rangeOverlayForNode(QDeclarativeItem *node, bool useCache = true);
    bool isDeploymentView() const;
    bool isScenarioView() const;
    bool isC2View() const;
    void assignRoute(QDeclarativeItem *walker);

private:
    QDeclarativeView *m_view;
    SensorsModel *m_model;
    bool m_globalRangeActivated;
    QMap<QString, Overlay *> m_externalOverlays;
};

#endif // VIEW_H
