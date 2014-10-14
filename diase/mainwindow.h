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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_diase.h"
#include "ui_connection_dialog.h"

class QDeclarativeItem;
class FancyButton;
class EventNotifier;
class AlarmNotifier;
class NetworkTopology;
class View;
class MonitoringView;
class SensorsModel;
class BarGraph;
class QDialog;
class AreaItemModel;
class DeploymentSettings;
class Overlay;

/*
 * The class MainWindow represents the GUI main window, this is the controller
 * for the views (deployment, scenario, c2) and the GUI.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

private Q_SLOTS:
    void showEditView();
    void showDeploymentView();
    void showC2View();
    void showMonitoring();
    void itemAddedToView(QDeclarativeItem *item);
    void sensorAddedToView(QDeclarativeItem *item);
    void itemIntrusionDetected(int value);
    void nodesAlarm(QList<int> nodesId, quint32 duration);
    void nodeFailed(quint16 nodeId);
    void connectToRemoteNetwork();
    void displaySharedOverlay(bool checked);
    void displayRangeOverlay(bool checked);
    void openOverlay();
    void openScenario();
    void saveScenario();
    void removeItem(QDeclarativeItem *item);

private:
    FancyButton *addFancyButton(const QString & text);
    void initFancyBar();
    void initFileMenu();
    void initNetworkMenu();
    void initDisplayMenu();
    void loadExternalOverlays();

private:
    Ui::MainWindow ui;
    Ui::ConnectionDialog ui_connection;

    QDialog *m_connectionDialog;
    EventNotifier *m_eventnotifier;
    AlarmNotifier *m_alarmnotifier;
    NetworkTopology *m_networktopology;

    SensorsModel *m_sensorsModel;

    View *m_editView;
    View *m_deploymentView;
    View *m_c2View;
    MonitoringView *m_monitoringView;
    DeploymentSettings *m_deploymentSettings;
    View *m_currentView;

    FancyButton *m_editViewButton;
    FancyButton *m_deploymentViewButton;
    FancyButton *m_c2ViewButton;
    FancyButton *m_monitoringButton;
};

#endif // MAINWINDOW_H
