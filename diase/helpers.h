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
#ifndef HELPERS_H
#define HELPERS_H

#include <QtCore/QObject>
#include <QtCore/QMetaObject>
#include <QtCore/QString>

static inline bool isSensor(const QObject *obj)
{
    const QMetaObject *mo = obj->metaObject();
    return QString::fromLatin1(mo->className()).contains("Sensor") ||
           QString::fromLatin1(mo->superClass()->className()).contains("Sensor");
}

static inline bool isIntruder(const QObject *obj)
{
    return QString::fromLatin1(obj->metaObject()->className()).contains("Intruder");
}

#endif // HELPERS_H
