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
#ifndef RESOURCESHELPER_H
#define RESOURCESHELPER_H

#include <QtCore/QDataStream>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

#define RESOURCE_INI_FILENAME "diase.ini"

static inline QString bytearrayToString(const QByteArray &array)
{
    QDataStream in(array);
    QString value;

    while (!in.atEnd()) {
        quint8 byte;
        in >> byte;
        value += (QString::number(byte) + ",");
    }
    value.remove(value.count() - 1, 1);
    return value;
}

static inline QString labelizeStringArray(const QString &resourceName, const QString &str)
{
    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);
    QStringList values;
    QString value;

    resourceMapper.beginGroup(resourceName);
    int size = resourceMapper.beginReadArray("shortarray");

    values = str.split(",");
    if (values.size() == size) {
        for (int i = 0; i < size; i++) {
            resourceMapper.setArrayIndex(i);
            value += resourceMapper.value("name").toString() + ":" + values.at(i) + "\n";
        }
    }
    resourceMapper.endArray();
    resourceMapper.endGroup();
    return value;
}

static inline QString shortarrayToString(const QByteArray &array)
{
    QDataStream in(array);
    QString value;
    quint16 ushort;

    while (!in.atEnd()) {
        in >> ushort;
        value += QString::number(ushort) + ",";
    }
    value.remove(value.count() - 1, 1);
    return value;
}

static inline QString shortToString(const QByteArray &array)
{
    QDataStream in(array);
    quint16 ushort;

    in >> ushort;
    return QString::number(ushort);
}

static inline QString byteToString(const QByteArray &array)
{
    QDataStream in(array);
    quint8 byte;

    in >> byte;
    return QString::number(byte);
}

static inline void multipartResource(int nodeId, const QString &name, QString &keysModel, QString &valuesModel)
{
    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);
    CoapInterface iface(nodeId);
    QByteArray res;
    QStringList keysContent, valuesContent;
    QVector<QPair<QStringList, QStringList> > multipart_array;
    QVector< QPair<int, int> > keys, values;
    int size, i = 0, j = 0;

    resourceMapper.beginGroup(name);
    size = resourceMapper.beginReadArray("multipart");

    // Check if the model description is present (required for multipart resources)
    if (size >= 1) {
        resourceMapper.setArrayIndex(0);

        if (resourceMapper.value("keys").canConvert<QStringList>())
            keysContent = resourceMapper.value("keys").toStringList();
        else
            keysContent = QStringList(resourceMapper.value("keys").toString());

        if (resourceMapper.value("values").canConvert<QStringList>())
            valuesContent = resourceMapper.value("values").toStringList();
        else
            valuesContent = QStringList(resourceMapper.value("values").toString());

        if (keysContent.isEmpty()) {
            qWarning() << "WARNING: missing attribute \"keys\" in multipart array for coap resource" << name;
            qWarning() << "This attribute should be located in the first entry of the multipart array";
            return;
        }
        if (valuesContent.isEmpty()) {
            qWarning() << "WARNING: missing attribute \"values\" in multipart array for coap resource" << name;
            qWarning() << "This attribute should be located in the first entry of the multipart array";
            return;
        }
    }

    // Parse keys
    foreach(QString key, keysContent) {
        QString entry = key.split(".").at(0);
        QString type  = key.split(".").at(1);
        QPair<int, int> pair;

        pair.first = entry.remove("multipart[").remove("]").toInt() - 1;
        pair.second = type.remove("type[").remove("]").toInt();
        keys.append(pair);
    }

    // Parse values
    foreach(QString value, valuesContent) {
        QString entry = value.split(".").at(0);
        QString type  = value.split(".").at(1);
        QPair<int, int> pair;

        pair.first = entry.remove("multipart[").remove("]").toInt() - 1;
        pair.second = type.remove("type[").remove("]").toInt();
        values.append(pair);
    }

    // Parse resources and types
    for (i = 0; i < size; i++) {
        QStringList names, types;

        resourceMapper.setArrayIndex(i);
        names = resourceMapper.value("name").toStringList();
        types = resourceMapper.value("type").toStringList();

        if (names.isEmpty() || types.isEmpty())
            continue;
        multipart_array.append(QPair<QStringList, QStringList>(names, types));
    }
    resourceMapper.endArray();
    resourceMapper.endGroup();

    for(i = 0; i < multipart_array.count(); i++) {
        QStringList names, types;
        QPair<QStringList, QStringList> pair;

        pair  = multipart_array.at(i);
        names = pair.first;
        types = pair.second;

        // Unify COAP resources on the same line
        res.clear();
        foreach(QString name, names)
            res += iface.call(name);

        QDataStream in(res);
        while(!in.atEnd()) {

            for (j = 0; j < types.count(); j++) {

                QString type = types.at(j);
                QString data;
                QPair<int, int> p;

                // Convert the network payload as described by the Ini file
                if (type == "int") {
                    quint32 integer;
                    in >> integer;
                    data = QString::number(integer);
                } else if (type == "byte") {
                    quint8 byte;
                    in >> byte;
                    data = QString::number(byte);
                } else if (type == "short") {
                    quint16 Short;
                    in >> Short;
                    data = QString::number(Short);
                }

                /* Model storage */

                // Check if this data goes into keys
                foreach(p, keys)
                    if (p.first == i && p.second == j)
                        keysModel += data + ":";
                // Check if this data goes into values
                foreach(p, values)
                    if (p.first == i && p.second == j)
                        valuesModel += data + ":";
            }
        }
    }
    keysModel.remove(keysModel.count() - 1, 1);
    valuesModel.remove(valuesModel.count() - 1, 1);
}



#endif // RESOURCESHELPER_H
