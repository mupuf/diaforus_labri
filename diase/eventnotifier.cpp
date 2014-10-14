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
#include "eventnotifier.h"
#include "ui_diase.h"

#include <stdint.h>
#include <QtCore/QRegExp>
#include <QtGui/QDialog>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtNetwork/QTcpSocket>

EventNotifier::EventNotifier(QWidget *parent, Ui::MainWindow *ui):
      QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    this->ui = ui;
    connect(m_socket, SIGNAL(connected()), SLOT(sendSocketKind()));
}

void EventNotifier::connectToDispatcher(const QString &address, int port)
{
    m_socket->connectToHost(address, port, QIODevice::WriteOnly);
}

void EventNotifier::notify(int nodeid, int sensorid, QString type, int value)
{
    uint8_t sensorID = 0;
    if (m_socket->state() != QAbstractSocket::ConnectedState)
        return;

    if (type == "PIR")
        sensorID = 0;
    else if (type == "SPIRIT")
        sensorID = 1;
    else if (type == "SEISMIC")
        sensorID = 2;
    sensorID = (sensorID << 4) | (sensorid & 0xf);
    char stimulus[] = { 0x0, 0x4, (char)((nodeid & 0xff00) >> 8), (char)(nodeid & 0xff), (char)(sensorID), (char)(value & 0xff)};

    m_socket->write(stimulus, 6);
}

void EventNotifier::sendSocketKind()
{
    char injector_headers[] = { 0x0, 0x1, 0x2};
    m_socket->write(injector_headers, 3);
}
