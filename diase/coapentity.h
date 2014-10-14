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
#ifndef COAPENTITY_H
#define COAPENTITY_H

#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QList>

class CoapInterface;
class QTimer;

/*
 * The class CoapEntity is used to keep a CoAP resource up-to-date.
 * Periodically, it calls the resource described by "resourceName" for the node "nodeId",
 * then it parses and extracts the response and saves it into the model(s) associated to the resource.
 * A CoapEntity always updates one resource for a given node, it can change one or many
 * model entries associated to the resource, for example because the same resource is present in differents views.
 */
class CoapEntity : public QObject
{
    Q_OBJECT
public:
    explicit CoapEntity(int nodeId, const QString &resourceName, QObject *parent = 0);

    /*
     * Get the name of CoAP resource
     * @return the resource name
     */
    QString resourceName() const;

    /*
     * Get the nodeId of the monitored node
     * @return the monitored nodeId
     */
    int nodeId() const;

    /*
     * Add a new model entry to change on update
     *
     * @param model A model containing the data to update for this node
     * @param entryIndex The location of the data in @model
     */
    void addMonitoringModelEntry(QObject *model, int entryIndex);

    /*
     * Remove a model entry from this entity.
     *
     * When a such model entry is removed , it's no longer updated.
     * When all the entries for a given model have been removed, the model
     * is removed from this entity.
     *
     * @param model a model containing the data to update for this node
     * @param entryIndex the location of the date in @model
     */
    void removeMonitoringModelEntry(QObject *model, int entryIndex);

    /*
     * Change the update interval
     *
     * @param interval the update periodicity in milliseconds
     */
    void setRefreshInterval(int interval);

private Q_SLOTS:
    void handleResponse(QByteArray & payload);
    void updateModels();

private:
    QString resourceType() const;

private:
    CoapInterface *m_iface;
    QTimer *m_timer;
    QString m_resourceName;
    QMap<QObject *, QList<int> > m_entries;
};

#endif // COAPENTITY_H
