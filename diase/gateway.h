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
#ifndef GATEWAY_H
#define GATEWAY_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QQueue>
#include <QtCore/QPair>
#include <QtNetwork/QUdpSocket>

class QTimer;

/*
 * The class Gateway is used to make communication through a IPv6 gateway running on the current machine.
 * It provides a simple way to send/receive IPv6/UDP datagrams over or from a gateway, also it provides an automatic
 * simple re-emission handling.
 */
class Gateway : public QObject
{
    Q_OBJECT
public:

    /*
     * Get the singleton instance
     * @return the unique instance of Gateway
     */
    static Gateway *instance();

    /*
     * Send a datagram over the gateway to node "targetNode"
     * The destination address is built according the @targetNode
     *
     * @targetNode The node identifier destination
     * @datagram The payload to send over the network
     * @tagName a unique name used to identify the datagram. This is required
     * because internally each datagram is sent one by one, the next one is sent
     * only when the current one has been succesfull sent and got an response. In order to identify
     * each packet, tagName are needed.
     */
    void writeDatagram (quint16 targetNode, const QByteArray &datagram, const QString &tagName);

    /*
     * Receive a datagram from the gateway
     *
     * @param sourceName The sender node identifier
     * @param datagram the received payload
     * @return the size of the received payload
     */
    qint64 readDatagram(quint16 &sourceNode, QByteArray &datagram);

    /*
     * Get the error of the last sent datagram
     */
    QAbstractSocket::SocketError error() const;

Q_SIGNALS:
    void readyRead(quint16 nodeId, const QString &tagName);

private Q_SLOTS:
    void recvData();
    void resendPacket();

private:
    Q_DISABLE_COPY(Gateway)
    explicit Gateway(QObject *parent = 0);

private:

    typedef QPair<QByteArray, QString> Pair;

    static Gateway *s_instance;
    quint16 m_pendingSourceNode;
    QByteArray m_pendingDatagram;
    QUdpSocket *m_socket;
    QQueue<Pair> m_sendingQueue;
    QTimer *m_resendTimer;
};

#endif // GATEWAY_H
