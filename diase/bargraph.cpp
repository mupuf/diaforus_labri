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
#include "bargraph.h"
#include "3rdparty/qcustomplot.h"
#include "coapinterface.h"
#include "resourceshelper.h"

#include <QtCore/QTimer>
#include <QtCore/QSettings>
#include <QtGui/QGraphicsProxyWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>

#define DEFAULT_REFRESH_INTERVAL 2000

BarGraph::BarGraph(int nodeId, QWidget *parent) :
    QDeclarativeItem(NULL)
    , m_customPlot(new QCustomPlot(parent))
    , m_proxy(new QGraphicsProxyWidget(this))
    , m_refreshButton(new QPushButton("&Stop refresh", parent))
    , m_colorId(0)
    , m_active(true)
{
    QWidget *widget;
    QVBoxLayout *vLayout;
    QHBoxLayout *hLayout;

    // Important, otherwise the paint method is never called
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setNodeId(nodeId);

    widget = new QWidget(parent);
    widget->setStyleSheet("background-color:#c2bbb8;");

    hLayout = new QHBoxLayout(parent);
    hLayout->addItem(new QSpacerItem(150, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));
    hLayout->addWidget(m_refreshButton);

    vLayout = new QVBoxLayout(parent);
    vLayout->addWidget(m_customPlot);
    vLayout->addItem(hLayout);

    widget->setLayout(vLayout);
    m_proxy->setWidget(widget);

    connect(m_refreshButton, SIGNAL(clicked()), SLOT(toggleTimer()));

    m_customPlot->yAxis->setSubGrid(true);
    m_customPlot->yAxis->setRange(0, 100);

    m_customPlot->xAxis->setGrid(false);
    m_customPlot->xAxis->setSubTickCount(0);
    m_customPlot->xAxis->setTickLength(0, 10);
    m_customPlot->xAxis->setTickLabelType(QCPAxis::ltDateTime);
    m_customPlot->xAxis->setDateTimeFormat("hh:mm:ss");
    m_customPlot->xAxis->setAutoTickStep(false);
    m_customPlot->xAxis->setTickStep(10);
    m_customPlot->setupFullAxesBox();

    m_customPlot->legend->setVisible(true);
    m_customPlot->legend->setPositionStyle(QCPLegend::psTopLeft);
    m_customPlot->legend->setBrush(QBrush(QColor(255, 255, 255, 200)));
}

void BarGraph::setResourceName(const QString &resourceName)
{
    m_resourceName = resourceName;
    m_customPlot->setTitle("Node " + QString::number(m_nodeId) + " : " + m_resourceName);
}

QString BarGraph::resourceName() const
{
    return m_resourceName;
}

void BarGraph::setXInterval(double lower, double upper)
{
    m_customPlot->xAxis->setRange(lower, upper);
}

void BarGraph::setYInterval(double lower, double upper)
{
    m_customPlot->yAxis->setRange(lower, upper);
}

int BarGraph::modelEntryIndex() const
{
   return m_modelEntryIndex;
}

void BarGraph::setModelEntryIndex(int index)
{
    m_modelEntryIndex = index;
}

int BarGraph::nodeId() const
{
    return m_nodeId;
}

void BarGraph::setNodeId(int nodeId)
{
    m_nodeId = nodeId;
    m_customPlot->setTitle("Node " + QString::number(m_nodeId) + " : " + m_resourceName);
}

void BarGraph::addKey(const QString &name, const QString &key)
{
    m_ticks << key.toDouble();
}

void BarGraph::addKeys(const QString &name, const QString &keys)
{
    QStringList k;
    int i;
    double ref = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
    QVector<double> ticks;

    Q_UNUSED(name);

    k = keys.split(":");
    foreach(QString value, k)
        ticks << value.toDouble();
    m_ticks << ticks;
}

void BarGraph::addValue(const QString &name, const QString &value)
{
    foreach(QCPBars *bar, m_mapper.keys()) {
        if (bar->name() == name) {
            m_mapper[bar] << value.toDouble();
            return;
        }
    }
    foreach(QCPGraph *graph, m_graphs.keys()) {
        if (graph->name() == name) {
            m_graphs[graph] << value.toDouble();
            return;
        }
    }
}

void BarGraph::addValues(const QString &name, const QString &values)
{
    QStringList v;
    foreach(QCPBars *bar, m_mapper.keys()) {
        if (bar->name() == name) {
            v = values.split(":");

            foreach(QString value, v) {
                m_mapper[bar] << (value.toDouble());
            }
            qDebug() << "adding values" << values << "to" << name;
            return;
        }
    }
}

void BarGraph::updateContent(const QVariant &index, const QVariant &keys, const QVariant &values)
{
    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);
    QStringList k, v, names;
    int i, size;

    if (index.toInt() != m_modelEntryIndex || !m_active)
        return;
   k = keys.toString().split(",");
   v = values.toString().split("|");
   m_ticks.clear();
   m_mapper.clear();
   foreach(QString value, k)
       m_ticks << value.toDouble();

   resourceMapper.beginGroup(m_resourceName);
   size = resourceMapper.beginReadArray("shortarray");
   for (i = 0; i < size; i++) {
       resourceMapper.setArrayIndex(i);
       names << resourceMapper.value("name").toString();
   }
   resourceMapper.endArray();
   resourceMapper.endGroup();

   if (m_customPlot->plottableCount() == 0) {
       for (i = 0; i < names.count(); i++) {
           m_mapper.insert(addBar(""), QVector<double>());
           if (i != 0)
               qobject_cast<QCPBars *>(m_customPlot->plottable(i))->moveAbove(qobject_cast<QCPBars *>(m_customPlot->plottable(i - 1)));
       }
   }

   foreach(QString data, v) {
       QStringList values = data.split(",");
       for (i = 0; i < m_customPlot->plottableCount(); i++) {
           QCPBars *bar = qobject_cast<QCPBars *>(m_customPlot->plottable(i));
           bar->setName(names[i]);
           m_mapper[bar] << (values.at(i).toDouble());
       }
   }
   redraw();
}

bool BarGraph::isEmpty() const
{
    return m_customPlot->plottableCount() == 0;
}

void BarGraph::newBar(const QString &name)
{
    m_mapper.insert(addBar(name), QVector<double>());
}

void BarGraph::newGraph(const QString &name, const QString &color)
{
    QCPGraph *graph;

    m_customPlot->addGraph();
    graph = m_customPlot->graph(m_customPlot->graphCount() - 1);
    graph->setPen(QColor(color));
    graph->setName(name);

    m_graphs.insert(graph, QVector<double>());
}

void BarGraph::setYAxisLabel(const QString &label)
{
    m_customPlot->yAxis->setLabel(label);
}

QCPBars *BarGraph::addBar(const QString &name)
{
    QCPBars *bar;
    QColor color;
    QPen pen;

    bar = new QCPBars(m_customPlot->xAxis, m_customPlot->yAxis);
    bar->setName(name);
    pen.setWidthF(1.2);
    color = pickNewColor();
    pen.setColor(QColor(color.red(), color.green(), color.blue())) ;
    bar->setPen(pen);
    bar->setBrush(QBrush(color));

    m_customPlot->addPlottable(bar);
    qDebug() << "adding bar" << name;
    m_customPlot->replot();
    return bar;
}

QColor BarGraph::pickNewColor()
{
    QColor ret;
    QVector<QColor> colors;

    colors << QColor(150, 222, 0, 70)
           << QColor(1, 92, 191, 50)
           << QColor(255, 131, 0, 50)
           << QColor(201, 103, 246, 50)
           << QColor(0, 0, 255, 50)
           << QColor(255, 246, 0, 50)
           << QColor(255, 0, 0, 50)
           << QColor(0, 255, 0, 50);

    ret = colors[m_colorId];
    m_colorId = (m_colorId + 1) % colors.size();
    return ret;
}

void BarGraph::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(oldGeometry)
    m_proxy->widget()->resize(newGeometry.width(), newGeometry.height());
    m_customPlot->resize(newGeometry.width(), newGeometry.height());
}

void BarGraph::toggleTimer()
{
    if (!m_active) {
       m_refreshButton->setText("&Stop refresh");
       m_active = true;
       return;
    }
    m_refreshButton->setText("&Start refresh");
    m_active = false;
}

void BarGraph::redraw()
{
    int i;

    if (!m_active)
        return;

    foreach(QCPBars *bar, m_mapper.keys()) {
        bar->setData(m_ticks, m_mapper[bar]);
        bar->rescaleValueAxis();
    }
    foreach(QCPGraph *graph, m_graphs.keys()) {
        graph->setData(m_ticks, m_graphs[graph]);
        graph->rescaleValueAxis();
    }

    // make key axis range scroll with the data (at a constant range size of 8):
    m_customPlot->xAxis->setRange(m_ticks.last() + 0.5, 20, Qt::AlignRight);
    m_customPlot->setRangeDrag(Qt::Horizontal);
    m_customPlot->setRangeZoom(Qt::Horizontal);
    m_customPlot->replot();
}
