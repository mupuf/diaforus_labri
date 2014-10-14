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
#ifndef EVENTNOTIFIER_H
#define EVENTNOTIFIER_H

#include <QtCore/QObject>

namespace Ui {
class MainWindow;
}

class QString;
class QTcpSocket;

/*
 * The class EventNotifier is used to send external events when a sensor made a detection.
 * When a such event happens, EventNotifier sends a message to an external entity (like a simulator)
 */
class EventNotifier : public QObject
{
    Q_OBJECT
public:
    EventNotifier(QWidget *parent, Ui::MainWindow *ui);

    /*
     * Notify an external entity that a sensor attached to a node made a detection
     *
     * @param nodeid The node identifier containing the sensor
     * @param sensorid The sensor involved in the detection
     * @param value The value detected by the sensor
     */
    void notify(int nodeid, int sensorid, QString type, int value);

public Q_SLOTS:
    void connectToDispatcher(const QString &address = "127.0.0.1", int port = 1234);

private Q_SLOTS:
    void sendSocketKind();

private:
    Ui::MainWindow *ui;

    QTcpSocket *m_socket;
};

#endif // EVENTNOTIFIER_H
