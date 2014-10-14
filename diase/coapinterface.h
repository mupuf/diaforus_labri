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
#ifndef COAPINTERFACE_H
#define COAPINTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QEventLoop>
#include <QtCore/QVariant>
#include <QtCore/QByteArray>

class QUdpSocket;

/*
 * The class CoapInterface is an abstraction to call a remote CoAP resource.
 */
class CoapInterface : public QObject
{
    Q_OBJECT
public:
    typedef enum {
        OK_200 = 80,
        CREATED_201 = 81,
        NOT_MODIFIED_304 = 124,
        BAD_REQUEST_400 = 160,
        NOT_FOUND_404 = 164,
        METHOD_NOT_ALLOWED_405 = 165,
        UNSUPPORTED_MEDIA_TYPE_415 = 175,
        INTERNAL_SERVER_ERROR_500 = 200,
        BAD_GATEWAY_502 = 202,
        GATEWAY_TIMEOUT_504 = 204
    } StatusCode;

    /*
     * Construct a CoapInterface for a node , described by nodeId
     * @param nodeId The remote node to contact through CoAP
     */
    explicit CoapInterface(int nodeId = 0, QObject *parent = 0);
    
    /*
     * Call a method synchronously
     *
     * @param method The resource to call remotely
     * @param args The arguments for the method described by @method, when
     * this list is empty, the called method is a GET method, otherwise this is a POST method.
     * @return The response returned by the remote CoAP resource
     */
    QByteArray call(const QString &method, const QList<QVariant> &args = QList<QVariant>());

    /*
     * Call a method asynchronously
     *
     * When the response has been received, the signal responsed is emitted (see below)
     *
     * @param method The resource to call remotely
     * @param args The arguments for the method described by @method, when
     * this list is empty, the called method is a GET method, otherwise this is a POST method.
     */
    void callAsync(const QString &method, const QList<QVariant> &args = QList<QVariant>());

    /*
     * Get the status code returned by the last call
     * @return a status code corresponding the last request status
     */
    StatusCode code() const;

    /*
     * Change the node id attached to this CoapInterface
     */
    void setNodeId(int nodeId);

    /*
     * Get the node id attached to this CoapInterface
     */
    int nodeId() const;

Q_SIGNALS:

    /*
     * This signal is emitted when a response for an asynchronous call has been received.
     * If the last call is synchronous the result is returned directly and this signal is never emitted.
     *
     * @param payload The response returned by the remote CoAP resource
     */
    void responsed(QByteArray & payload);

private Q_SLOTS:
    void recvData(quint16 sourceNode, const QString &tagName);

private:
    void getResponse(QByteArray &payload);

private:
    int m_nodeId;
    StatusCode m_code;
    QEventLoop m_localLoop;
    QString m_currentMethod;
};

#endif // COAPINTERFACE_H
