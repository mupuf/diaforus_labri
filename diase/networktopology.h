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
#ifndef NETWORKTOPOLOGY_H
#define NETWORKTOPOLOGY_H

#include "ui_building.h"
#include "ui_nodes_dialog.h"
#include "ui_hybrid_dialog.h"
#include <QtCore/QObject>
#include <QtCore/QFile>
#include <QtCore/QList>
#include <QtCore/QProcess>
#include <QtXml/QDomDocument>

namespace Ui {
class MainWindow;
}
class QDialog;
class QTimer;
class ItemModel;
class AreaItemModel;
class NodeItemModel;
class SensorItemModel;

class NetworkTopology : public QObject
{
    Q_OBJECT
public:
    explicit NetworkTopology(QWidget *parent, Ui::MainWindow *ui);
    virtual ~NetworkTopology();

    QString filePath() const;

public Q_SLOTS:
    void openTopology(const QString &filePath = QString());

Q_SIGNALS:
    void areaAdded(AreaItemModel *area);
    void nodeAdded(NodeItemModel *node);
    void sensorAdded(SensorItemModel *sensor);
    void coapGroupAdded(const QString &groupname, const QStringList &resources);
    void nodeAddedToCoapGroup(NodeItemModel *node, const QString &groupname);
    void clear();
    void networkStarted();

private Q_SLOTS:
    void saveXMLDocument();
    void itemModelPropertyChanged(ItemModel *item, const char *property, QVariant value);
    void buildNetwork();
    void toggleHybridNetwork();
    void toggleNetwork();
    void dispatcherIsReady();
    void reasoningNodesAreReady();
    void buildNetworkProcessFinished(int exitCode);
    void showNodesDialog();
    void displayNodeOutput(QProcess *process = NULL);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void startNodes(const QStringList &nodes);
    void startNetwork(const QString &iface = QString(), int speed = 0);
    void stopNetwork();
    QString dispatcherDirectory(bool hybrid = false) const;

private:
    Ui::MainWindow *ui;
    Ui::BuildingDialog ui_building;
    Ui::NodesDialog ui_nodesDialog;
    Ui::hybridDialog ui_hybridDialog;

    QDialog *m_building_dialog;
    QDialog *m_nodesDialog;
    QDialog *m_hybridDialog;
    QProcess *m_buildNetworkProcess;
    QList<QProcess *> *m_nodesProcessList;
    QTimer *m_dispTimer;
    QTimer *m_reasoningTimer;
    QDomDocument m_dom;
    QFile m_xmlDoc;
};

#endif // NETWORKTOPOLOGY_H
