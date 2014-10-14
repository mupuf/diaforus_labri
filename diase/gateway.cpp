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
#include "gateway.h"

#include <QtGui/QApplication>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#define DIAFORUS_NET_PREFIX "1180::1063:9FF:FE30:"
#define DIAFORUS_COAP_PORT 61617
#define RESEND_TIMEOUT 5000

Gateway *Gateway::s_instance = NULL;

Gateway::Gateway(QObject *parent) :
    QObject(parent)
    , m_pendingSourceNode(0)
    , m_socket(new QUdpSocket(this))
    , m_resendTimer(new QTimer(parent))
{
    m_socket->bind(QHostAddress::AnyIPv6, DIAFORUS_COAP_PORT);
    m_resendTimer->setInterval(RESEND_TIMEOUT);
    m_resendTimer->setSingleShot(false);

    connect(m_socket, SIGNAL(readyRead()), SLOT(recvData()));
    connect(m_resendTimer, SIGNAL(timeout()), SLOT(resendPacket()));
}

Gateway *Gateway::instance()
{
    if (!s_instance)
        s_instance = new Gateway(qApp);
    return s_instance;
}

void Gateway::writeDatagram(quint16 targetNode, const QByteArray &datagram, const QString &tagName)
{
    m_sendingQueue.enqueue(Pair(datagram, tagName + ":" + QString::number(targetNode)));
    if (m_sendingQueue.size() == 1) {
        //qDebug() << "sending packet explicitly" << tagName << "to node" << targetNode;
        m_socket->writeDatagram(datagram, QHostAddress(DIAFORUS_NET_PREFIX + QString("%1").arg(targetNode, 0, 16)), DIAFORUS_COAP_PORT);
        m_resendTimer->start();
    }
}

void Gateway::recvData()
{
    qint64 pendingDatagramSize;
    QHostAddress peerAddr;
    Pair currentRequest;
    char *data;

    m_pendingDatagram.clear();
    pendingDatagramSize = m_socket->pendingDatagramSize();
    data = (char *)malloc(pendingDatagramSize);
    pendingDatagramSize = m_socket->readDatagram((char *)data, 350, &peerAddr, NULL);
    m_pendingDatagram.append(data, pendingDatagramSize);
    free(data);

    m_pendingSourceNode = peerAddr.toString().replace(DIAFORUS_NET_PREFIX, "").replace("%0", "").toInt(0, 16);

    currentRequest = m_sendingQueue.dequeue();
    m_resendTimer->stop();

    //qDebug() << "received reply for" << m_pendingSourceNode << currentRequest.second.split(":").at(0);
    emit readyRead(m_pendingSourceNode, currentRequest.second.split(":").at(0));

    // Send the next packet in the queue
    if (m_sendingQueue.size() != 0) {
        quint16 targetNode;
        currentRequest = m_sendingQueue.head();
        targetNode = currentRequest.second.split(":").at(1).toInt();
        //qDebug() << "sending next packet" << currentRequest.second.split(":").at(0) << "to node" << targetNode;
        m_socket->writeDatagram(currentRequest.first, QHostAddress(DIAFORUS_NET_PREFIX + QString("%1").arg(targetNode, 0, 16)), DIAFORUS_COAP_PORT);
        m_resendTimer->start();
    }
}

void Gateway::resendPacket()
{
    quint16 targetNode;

    targetNode = m_sendingQueue.head().second.split(":").at(1).toInt();
    //qDebug() << "resending packet for " << targetNode;
    m_socket->writeDatagram(m_sendingQueue.head().first, QHostAddress(DIAFORUS_NET_PREFIX + QString("%1").arg(targetNode, 0, 16)), DIAFORUS_COAP_PORT);
}

qint64 Gateway::readDatagram(quint16 &sourceNode, QByteArray &datagram)
{
    sourceNode = m_pendingSourceNode;
    datagram = m_pendingDatagram;
    return m_pendingDatagram.size();
}

QAbstractSocket::SocketError Gateway::error() const
{
    return m_socket->error();
}
