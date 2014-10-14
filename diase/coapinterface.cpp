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
#include "coapinterface.h"
#include "gateway.h"
#include <stdint.h>
#include <arpa/inet.h>
#include <QtNetwork/QUdpSocket>
#include <QtCore/QCoreApplication>

#define COAP_DATA_MAX_SIZE 350
#define COAP_PORT 61617

CoapInterface::CoapInterface(int nodeId, QObject *parent) :
    QObject(parent)
    , m_nodeId(nodeId)
{
    connect(Gateway::instance(), SIGNAL(readyRead(quint16,QString)), SLOT(recvData(quint16,QString)));
}

void CoapInterface::getResponse(QByteArray &payload)
{
    QByteArray pkt;
    quint16 nodeid;

    Gateway::instance()->readDatagram(nodeid, pkt);

    m_code = (StatusCode)pkt.at(1);
    payload = pkt.mid(4);
}

void CoapInterface::recvData(quint16 sourceNode, const QString &tagName)
{
    QByteArray payload;

    if (sourceNode != m_nodeId)
        return;
    if (tagName != m_currentMethod)
        return;
    if (m_localLoop.isRunning()) {
        m_localLoop.exit();
        return;
    }
    getResponse(payload);
    emit responsed(payload);
}

void CoapInterface::callAsync(const QString &method, const QList<QVariant> &args)
{
    QByteArray pkt;

    pkt += 0x51; // V = 1, T = Non-confirmable, and OC = 1
    if (args.length() != 0)
        pkt += 0x02; // method POST
    else
        pkt += 0x01; // method GET
    pkt += (char)0x00; // transaction ID
    pkt += (char)0x00; // transaction ID

    if (method.length() < 15) {
        pkt += (0x90 + method.length());
        pkt += method;
    }
    else {
        pkt += 0x9f;
        pkt += (method.length() - 15);
        pkt += method;
    }

    if(args.length() != 0 && args.at(0).canConvert(QVariant::String))
        pkt += args.at(0).toString();
    m_currentMethod = method;
    Gateway::instance()->writeDatagram(m_nodeId, pkt, method);
}

QByteArray CoapInterface::call(const QString &method, const QList<QVariant> &args)
{
    QByteArray payload;

    callAsync(method, args);

    // Get method
    if (args.length() == 0) {
        m_localLoop.exec();
        getResponse(payload);
    }
    return payload;
}

int CoapInterface::nodeId() const
{
    return m_nodeId;
}

void CoapInterface::setNodeId(int nodeId)
{
    m_nodeId = nodeId;
}

CoapInterface::StatusCode CoapInterface::code() const
{
    return m_code;
}
