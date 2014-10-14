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
#ifndef MONITORINGVIEW_H
#define MONITORINGVIEW_H

#include <QtCore/QObject>
#include <QtCore/QVector>
#include <QtCore/QPair>
#include <QtDeclarative/QtDeclarative>

class QDeclarativeView;
class SensorsModel;
class NodeItemModel;
class CoapEntity;
class BarGraph;

class MonitoringView : public QObject
{
    Q_OBJECT
public:
    explicit MonitoringView(QObject *parent, QDeclarativeView *view);

    void setModel(SensorsModel *model);

public Q_SLOTS:
    void addNodeToListView(NodeItemModel *nodeItem);
    void addCoapGroup(const QString &groupName, const QStringList &resources);
    void addNodeToCoapGroup(NodeItemModel *nodeItem, const QString &groupName);

private Q_SLOTS:
    void nodeClicked(int);
    void coapResourceClicked(const QString &name);
    void drawingTypeClicked(const QString &name);
    void removeResource(int index);
    void viewClicked(int index);
    void titleViewChanged(int index, const QString &title);
    QDeclarativeItem *gridViewGetItemAt(QObject *gridView, int index);
    void updateView(const QVariant &index, const QVariant &keys, const QVariant &values, QObject *model = NULL);
    void createNewView();

private:
    QObject * loadQMLListModel(const QString &componentName);
    QDeclarativeItem *nodeListView() const;
    QDeclarativeItem *wellKnownListView() const;
    QDeclarativeItem *viewsListView() const;
    QObject *tileGridView() const;
    QObject *graphGridView() const;
    QObject *tileGridModel() const;
    QObject *graphGridModel() const;
    QString resourceName(const QString &resourceName) const;
    QString resourceType(const QString &resourceName) const;
    void displayResource(int nodeId, const QString &name, const QString &drawing);
    void displayMultiPartResource(int nodeId, const QString &name, const QString &drawing);
    void itemAddedToGridView(QObject *gridView);
    void runDelegatedPlugin(QDeclarativeItem *item, const QString &fileName, const QVariant &index, const QVariant &keys, const QVariant &values);
    QString delegatedPlugin(QDeclarativeItem *item);

private:
    QDeclarativeView *m_view;
    SensorsModel *m_model;
    QVector<CoapEntity *> m_entities;
    QVector< QPair<QObject *, QObject *> > m_models;
    QObject *m_nodeModel;
    QMap<QString, QScriptProgram *> m_scripts;
    QMap<QString, QStringList> m_coapGroups;
    QMap<QString, QStringList> m_nodeGroups;
};

#endif // MONITORINGVIEW_H
