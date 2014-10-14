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
#ifndef C2NOTIFIER_H
#define C2NOTIFIER_H

#include <QtCore/QObject>
#include <QtNetwork/QAbstractSocket>

class QTcpSocket;

/*
 * The class AlarmNotifier is responsible to notify
 * the application when an alarm is sent by a node in a real
 * or simulated network. It is also used to notify the application
 * when a node has died or failed.
 */
class AlarmNotifier : public QObject
{
    Q_OBJECT
public:
    explicit AlarmNotifier(QObject *parent = 0);

public Q_SLOTS:
    void connectToServer(const QString &address = "127.0.0.1", quint16 port = 4242);

Q_SIGNALS:

    /*
     * This signal is emitted when an alarm has been received
     * for one or more nodes.
     *
     * @param nodesId A List containing ids of involved nodes
     * @param duration the time of the intrusion in milliseconds
     */
    void nodesAlarm(QList<int> nodesId, quint32 duration);

    /*
     * This signal is emitted when a node is dead or has failed
     * @param nodeId the failing node
     */
    void nodeFailed(quint16 nodeId);

private Q_SLOTS:
    void readyNodeId();

private:
    QTcpSocket *m_socket;
};

#endif
