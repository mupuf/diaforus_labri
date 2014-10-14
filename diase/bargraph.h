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
#ifndef GRAPH_H
#define GRAPH_H

#include <QtCore/QVector>
#include <QtCore/QMap>
#include <QtDeclarative/QDeclarativeItem>

class QCustomPlot;
class QGraphicsProxyWidget;
class QCPBars;
class QCPGraph;
class QPushButton;

class CoapInterface;

class BarGraph : public QDeclarativeItem
{
    Q_OBJECT
    Q_PROPERTY(QString resourceName READ resourceName WRITE setResourceName)
    Q_PROPERTY(int nodeId READ nodeId WRITE setNodeId)
    Q_PROPERTY(int modelEntryIndex READ modelEntryIndex WRITE setModelEntryIndex)
public:
    explicit BarGraph(int nodeId = 0, QWidget *parent = 0);

    void setResourceName(const QString &resourceName);
    QString resourceName(void) const;

    void setNodeId(int nodeId);
    int nodeId() const;

    void setModelEntryIndex(int index);
    int modelEntryIndex() const;

    void setXInterval(double lower, double upper);
    void setYInterval(double lower, double upper);

    virtual void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);

    Q_INVOKABLE void updateContent(const QVariant &index, const QVariant &keys, const QVariant &values);
    Q_INVOKABLE bool isEmpty() const;
    Q_INVOKABLE void newBar(const QString &name);
    Q_INVOKABLE void newGraph(const QString &name, const QString &color);
    Q_INVOKABLE void addKeys(const QString &name, const QString &keys);
    Q_INVOKABLE void addKey(const QString &name, const QString &key);
    Q_INVOKABLE void addValues(const QString &name, const QString &values);
    Q_INVOKABLE void addValue(const QString &name, const QString &value);
    Q_INVOKABLE void redraw();
    Q_INVOKABLE void setYAxisLabel(const QString &label);

private Q_SLOTS:
    void toggleTimer();

private:
    QCPBars * addBar(const QString &name);
    QColor pickNewColor();

private:
    QCustomPlot *m_customPlot;
    QGraphicsProxyWidget *m_proxy;
    QString m_resourceName;
    QMap<QCPBars *, QVector<double> > m_mapper;
    QMap<QCPGraph *, QVector<double> > m_graphs;
    QVector<double> m_ticks;
    QPushButton *m_refreshButton;
    int m_modelEntryIndex;
    int m_nodeId;
    int m_colorId;
    bool m_active;
};

#endif // GRAPH_H
