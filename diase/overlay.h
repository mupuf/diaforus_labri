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
#ifndef OVERLAY_H
#define OVERLAY_H

#include <QtCore/QObject>

class QDeclarativeView;
class QMenu;
class SensorsModel;

class Overlay : public QObject
{
    Q_OBJECT
public:

    enum Option {
        RenderInDeploymentView = 0,
        RenderInScenarioView = 1,
        RenderInC2View = 2
    };
    Q_DECLARE_FLAGS(RenderingOptions, Option)

public:

    virtual RenderingOptions options() const = 0;
    virtual bool alwaysVisible() const;

    void addView(QDeclarativeView *view);
    void setModel(SensorsModel *model);

public Q_SLOTS:
    virtual void showOverlay() = 0;
    virtual void hideOverlay() = 0;

protected:
    explicit Overlay(QObject *parent);

protected:
    QList<QDeclarativeView *> m_views;
    SensorsModel *m_model;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(Overlay::RenderingOptions)
#endif // OVERLAY_H
