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
#include "coapentity.h"
#include "coapinterface.h"
#include "resourceshelper.h"
#include <QtDeclarative/QtDeclarative>

CoapEntity::CoapEntity(int nodeId, const QString &resourceName, QObject *parent):
    QObject(parent)
    , m_iface(new CoapInterface(nodeId, this))
    , m_timer(new QTimer(this))
    , m_resourceName(resourceName)
{
    m_timer->setInterval(2000);
    m_timer->setSingleShot(false);
    connect(m_timer, SIGNAL(timeout()), SLOT(updateModels()));
    connect(m_iface, SIGNAL(responsed(QByteArray&)), SLOT(handleResponse(QByteArray&)));
    m_timer->start();
}

void CoapEntity::handleResponse(QByteArray &payload)
{
    QString type, key, value;

    key = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0, 'f');

    type = resourceType();

    if (type == "bytearray")
        value = bytearrayToString(payload);
    else if (type == "shortarray")
        value = shortarrayToString(payload);
    else if (type == "short")
        value = shortToString(payload);
    else if (type == "byte")
        value = bytearrayToString(payload);
    else if (type == "string")
        value = payload;

    foreach (QObject *model, m_entries.keys()) {
        foreach(int entryIndex, m_entries[model]) {
            qDebug() << "CoapEntity updating" << model << entryIndex;
            QMetaObject::invokeMethod(model, "updateResource", Qt::DirectConnection,
                                      Q_ARG(QVariant, entryIndex),
                                      Q_ARG(QVariant, key),
                                      Q_ARG(QVariant, value));
        }
    }
}

QString CoapEntity::resourceType() const
{
    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);
    QString type;

    resourceMapper.beginGroup(m_resourceName);
    type = resourceMapper.value("type").toString();
    resourceMapper.endGroup();

    return type;
}

void CoapEntity::setRefreshInterval(int interval)
{
    m_timer->setInterval(interval);
}

QString CoapEntity::resourceName() const
{
    return m_resourceName;
}

int CoapEntity::nodeId() const
{
    return m_iface->nodeId();
}

void CoapEntity::addMonitoringModelEntry(QObject *model, int entryIndex)
{
    if (m_entries.contains(model)) {
        //qDebug() << "CoapEntity::addMonitoringModelEntry adding entry" << entryIndex << "for model" << model;
        QList<int> &list = m_entries[model];
        list << entryIndex;
        return;
    }
    //qDebug() << "CoapEntity::addMonitoringModelEntry inserting model" << model;
    m_entries.insert(model, QList<int>() << entryIndex);
}

void CoapEntity::removeMonitoringModelEntry(QObject *model, int entryIndex)
{
    if (m_entries.contains(model)) {
        m_entries[model].removeAll(entryIndex);

        if (m_entries[model].isEmpty())
            m_entries.remove(model);
    }
}

void CoapEntity::updateModels()
{
    if (m_entries.isEmpty() || m_resourceName.isEmpty())
        return;

    if (resourceType() != "multipart") {
        m_iface->callAsync(m_resourceName);
    }
    else {
        QString keys, values;
        multipartResource(m_iface->nodeId(), m_resourceName, keys, values);

        foreach (QObject *model, m_entries.keys()) {
            foreach(int entryIndex, m_entries[model]) {
                //qDebug() << "updating multipart resource" << model << entryIndex;
                QMetaObject::invokeMethod(model, "updateResource", Qt::DirectConnection,
                                          Q_ARG(QVariant, entryIndex),
                                          Q_ARG(QVariant, keys),
                                          Q_ARG(QVariant, values));
            }
        }
    }
}
