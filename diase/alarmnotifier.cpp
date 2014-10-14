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
#include "alarmnotifier.h"

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

AlarmNotifier::AlarmNotifier(QObject *parent) :
    QObject(parent)
  , m_socket(new QTcpSocket(this))
{
    connect(m_socket, SIGNAL(readyRead()), SLOT(readyNodeId()));
}

void AlarmNotifier::connectToServer(const QString &address, quint16 port)
{
    if (m_socket->state() == QTcpSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        m_socket->waitForDisconnected();
    }
    m_socket->connectToHost(address, port);
    m_socket->waitForConnected();
}

void AlarmNotifier::readyNodeId()
{
    QDataStream iStream(m_socket);
    quint8 type = 0;
    quint8 nodesNb = 0;
    quint16 nodeId = 0;
    quint32 duration = 0;
    QList<int> nodesId;

    iStream >> type;

    qDebug() << "AlarmNotifier: type" << type;

    if (type == 1) {
        iStream >> nodesNb;
        for (int i = 0; i < nodesNb; i++) {
            iStream >> nodeId;
            nodesId << nodeId;
        }
        qDebug() << "RECEIVED ALARM" << nodesId;
        iStream >> duration;
        emit nodesAlarm(nodesId, duration);
    } else if (type == 2) {
        iStream >> nodeId;
        qDebug() << "NODEID" << nodeId << "FAILED!! It does not respond !!";

        emit nodeFailed(nodeId);
    }
}
