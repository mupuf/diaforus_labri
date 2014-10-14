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
#include "monitoringview.h"

#include "sensormodel.h"
#include "coapinterface.h"
#include "coapentity.h"
#include "resourceshelper.h"
#include "bargraph.h"
#include <QtDeclarative/QtDeclarative>
#include <QtScript/QScriptEngine>

MonitoringView::MonitoringView(QObject *parent, QDeclarativeView *view):
    QObject(parent)
    , m_view(view)
{
    QObject *tileGridView;

    m_view->setSource(QUrl("qrc:/qml/diase/MonitoringView.qml"));
    m_view->setResizeMode(QDeclarativeView::SizeRootObjectToView);

    // Sharing the same model
    tileGridView = this->tileGridView();


    m_view->rootObject()->setProperty("currentIndexView", 0);
    m_models.append(QPair<QObject *, QObject *>(tileGridModel(), graphGridModel()));

    connect(tileGridView, SIGNAL(removeResource(int)), SLOT(removeResource(int)));
    connect(graphGridView(), SIGNAL(removeResource(int)), SLOT(removeResource(int)));
    connect(m_view->rootObject(), SIGNAL(nodeClicked(int)), SLOT(nodeClicked(int)));
    connect(m_view->rootObject(), SIGNAL(coapResourceClicked(QString)), SLOT(coapResourceClicked(QString)));
    connect(m_view->rootObject(), SIGNAL(viewClicked(int)), SLOT(viewClicked(int)));
    connect(m_view->rootObject(), SIGNAL(createView()), SLOT(createNewView()));
    connect(m_view->rootObject(), SIGNAL(titleChanged(int, QString)), SLOT(titleViewChanged(int,QString)));
    connect(m_view->rootObject(), SIGNAL(drawingTypeClicked(QString)), SLOT(drawingTypeClicked(QString)));

    connect(tileGridModel(), SIGNAL(update(QVariant, QVariant, QVariant)), SLOT(updateView(QVariant,QVariant,QVariant)));
    connect(graphGridModel(), SIGNAL(update(QVariant, QVariant, QVariant)), SLOT(updateView(QVariant,QVariant,QVariant)));
}

void MonitoringView::setModel(SensorsModel *model)
{
    m_model = model;
}

QObject * MonitoringView::loadQMLListModel(const QString &componentName)
{
    QDeclarativeEngine *engine;
    QObject *instance;

    engine = m_view->engine();
    QDeclarativeComponent component(engine, QString::fromLatin1("qml/diase/") + componentName + QString::fromLatin1(".qml"));

    if (component.status() == QDeclarativeComponent::Error) {
        foreach(QDeclarativeError error, component.errors())
            qWarning() << "QML error: " << error.description() << "[component=" << componentName << "]";
        return NULL;
    }

    instance = qobject_cast<QObject *>(component.create());
    if (!instance) {
        qWarning() << "QML error: Unable instanciate object from component" << "[component=" << componentName << "]";
        return NULL;
    }

    return instance;
}

void MonitoringView::addCoapGroup(const QString &groupName, const QStringList &resources)
{
    if (!m_coapGroups.contains(groupName))
        m_coapGroups.insert(groupName, resources);
}

void MonitoringView::addNodeToCoapGroup(NodeItemModel *nodeItem, const QString &groupName)
{
    QStringList groupList;
    if (!m_nodeGroups.contains(nodeItem->objectName())) {
        m_nodeGroups.insert(nodeItem->objectName(), QStringList() << groupName);
    } else {
        groupList = m_nodeGroups[nodeItem->objectName()];
        groupList << groupName;
        m_nodeGroups[nodeItem->objectName()] = groupList;
    }
}

QDeclarativeItem *MonitoringView::nodeListView() const
{
    QDeclarativeItem *leftBox;

    leftBox = m_view->rootObject()->findChild<QDeclarativeItem *>(QString::fromLatin1("leftBox"));
    return leftBox->findChild<QDeclarativeItem *>(QString::fromLatin1("nodeListView"));
}

QDeclarativeItem *MonitoringView::wellKnownListView() const
{
    QDeclarativeItem *rightBox;

    rightBox = m_view->rootObject()->findChild<QDeclarativeItem *>(QString::fromLatin1("rightBox"));
    return rightBox->findChild<QDeclarativeItem *>(QString::fromLatin1("wellKnownListView"));
}

QDeclarativeItem *MonitoringView::viewsListView() const
{
    QDeclarativeItem *leftBox;

    leftBox = m_view->rootObject()->findChild<QDeclarativeItem *>(QString::fromLatin1("viewBox"));
    return leftBox->findChild<QDeclarativeItem *>(QString::fromLatin1("viewsListView"));
}

QString MonitoringView::resourceName(const QString &resourceName) const
{
    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);
    QString name;

    resourceMapper.beginGroup(resourceName);
    name = resourceMapper.value("name").toString();
    resourceMapper.endGroup();

    return name;
}

QString MonitoringView::resourceType(const QString &resourceName) const
{
    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);
    QString type;

    resourceMapper.beginGroup(resourceName);
    type = resourceMapper.value("type").toString();
    resourceMapper.endGroup();

    return type;
}

QObject *MonitoringView::tileGridModel() const
{
    return m_view->rootObject()->findChild<QDeclarativeItem *>("tileGridView")->property("model").value<QObject *>();
}

QObject *MonitoringView::tileGridView() const
{
    return m_view->rootObject()->findChild<QObject *>("tileGridView");
}

QObject *MonitoringView::graphGridModel() const
{
    return m_view->rootObject()->findChild<QDeclarativeItem *>("graphGridView")->property("model").value<QObject *>();
}

QObject *MonitoringView::graphGridView() const
{
    return m_view->rootObject()->findChild<QObject *>("graphGridView");
}

// This is the slot called when a data has been added to the model (for a coap resource) and the corresponding
// graphical item has been added to the grid view.
// This slot get the lastest graphical item added to the gridView and create a CoapEntity (object entity that handles automatic updates).
// Finally the item is associated to the CoapEntity.
void MonitoringView::itemAddedToGridView(QObject *gridView)
{
    QObject *model;
    QDeclarativeItem *item;
    QString resourceName, interval, drawing;
    CoapEntity *entity;
    int count, nodeId, entryIndex;

    model = gridView->property("model").value<QObject *>();
    count = gridView->property("count").toInt();
    item = qobject_cast<QDeclarativeItem *>(gridViewGetItemAt(gridView, count - 1));
    nodeId = item->property("nodeId").toInt();
    resourceName = item->property("resourceName").toString();
    item->setProperty("modelEntryIndex", count - 1);
    entryIndex = item->property("modelEntryIndex").toInt();

    qDebug() << "item" << item << "added for node" << nodeId << "resource" << resourceName << "entryIndex" << entryIndex;
    if (resourceName.isEmpty())
        return;

    item->setProperty("associatedModel", QVariant::fromValue(model));
    foreach (entity, m_entities) {
        if (entity->resourceName() == resourceName && entity->nodeId() == nodeId) {
            entity->addMonitoringModelEntry(model, entryIndex);
            goto out;
        }
    }
    m_entities.append(new CoapEntity(nodeId, resourceName, this));
    m_entities.last()->addMonitoringModelEntry(model, entryIndex);
    entity = m_entities.last();

out:
    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);
    resourceMapper.beginGroup(resourceName);
    interval = resourceMapper.value("interval").toString();
    drawing = resourceMapper.value("drawing").toString();

    if (!interval.isEmpty())
        entity->setRefreshInterval(interval.toInt());
    if (drawing.startsWith("plugin::")) {
        QMetaObject::invokeMethod(model, "setDelegatedPluginPath", Qt::DirectConnection,
                                  Q_ARG(QVariant, entryIndex),
                                  Q_ARG(QVariant, drawing.split("::").at(2)));
    }
    resourceMapper.endGroup();
}

void MonitoringView::runDelegatedPlugin(QDeclarativeItem *item, const QString &fileName, const QVariant &index, const QVariant &keys, const QVariant &values)
{
    QFile scriptFile;
    QScriptEngine engine;
    QScriptValue func, result, object;
    QScriptProgram *program;
    QString type;

    if (!m_scripts.contains(fileName)) {
        scriptFile.setFileName(fileName);
        if (!scriptFile.open(QIODevice::ReadOnly))
            return;
        QTextStream in(&scriptFile);
        program = new QScriptProgram(in.readAll());
        m_scripts.insert(fileName, program);
    }
    //FIXME: Free program when the graphical resource is removed
    type = item->property("type").toString();
    program = m_scripts[fileName];
    engine.evaluate(*program);
    engine.globalObject().setProperty("INTERNAL_MODEL_SEPARATOR", ":", QScriptValue::ReadOnly);
    engine.globalObject().setProperty("CURRENT_DATE_TIME", QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0, QScriptValue::ReadOnly);

    if (type == "graph") {
        object = engine.newQObject(item);
    }

    if (!item->property("initDelegatedPlugin").isValid() && (type == "graph")) {
        item->setProperty("initDelegatedPlugin", true);

        func = engine.globalObject().property("initContentGraph");
        result = func.call(engine.globalObject(), QScriptValueList() << object);

        if (engine.hasUncaughtException()) {
            QMessageBox::critical(NULL, "Plugin" + fileName + "crashed during initialization", engine.uncaughtException().toString());
            return;
        }
    }

    func = engine.globalObject().property(type == "text" ? "updateContentText" : "updateContentGraph");
    qDebug() << "calling script with key=" << keys.toString() << ",values=" << values.toString();
    result = func.call(engine.globalObject(), QScriptValueList() << object << index.toInt() << keys.toString() << values.toString());

    if (engine.hasUncaughtException()) {
        QMessageBox::critical(NULL, "Plugin" + fileName + "crashed", engine.uncaughtException().toString());
        return;
    }

    item->setProperty("value", result.toString());
}

void MonitoringView::updateView(const QVariant &index, const QVariant &keys, const QVariant &values, QObject *model)
{
    QDeclarativeItem *item;
    QString labelizedValues, delegatedPlugin;
    QObject *gridView, *currentModel, *associatedModel;
    int i = 0, count = 0;

    // Dispatching event to all tile views
    gridView = tileGridView();
    count = gridView->property("count").toInt();
    currentModel = model ? model : sender();

    for (i = 0; i < count; i++) {
        item = gridViewGetItemAt(gridView, i);
        item->setProperty("type", "text");
        associatedModel = item->property("associatedModel").value<QObject *>();

        if (associatedModel != currentModel)
            continue;
        delegatedPlugin = this->delegatedPlugin(item);
        if (!delegatedPlugin.isEmpty()) {
            runDelegatedPlugin(item, "diaseplugin_" + delegatedPlugin + ".js", index, keys, values);
            continue;
        }
        //qDebug() << "updating simple tile view" << item->property("nodeId").toInt() << item->property("resourceName").toString();

        foreach(QString value, values.toString().split("|")) {
            QString result = labelizeStringArray(item->property("resourceName").toString(), value);
            if (!result.isEmpty()) {
                labelizedValues += ("|" + result);
            }
        }
        QMetaObject::invokeMethod(item, "updateContent", Qt::DirectConnection,
                                  Q_ARG(QVariant, index),
                                  Q_ARG(QVariant, keys),
                                  Q_ARG(QVariant, labelizedValues.isEmpty() ? values : labelizedValues));
    }

    // Dispatching event to all graph views
    gridView = graphGridView();
    count = gridView->property("count").toInt();
    for (i = 0; i < count; i++) {
        item = gridViewGetItemAt(gridView, i);
        item->setProperty("type", "graph");
        associatedModel = item->property("associatedModel").value<QObject *>();

        if (associatedModel != currentModel)
            continue;
        delegatedPlugin = this->delegatedPlugin(item);
        qDebug() << "delegatedPlugin" << delegatedPlugin << ",item=" << item;
        if (!delegatedPlugin.isEmpty()) {
            runDelegatedPlugin(item, "diaseplugin_" + delegatedPlugin + ".js", index, keys, values);
            continue;
        }
        //qDebug() << "updating simple graph view" << item->property("nodeId").toInt() << item->property("resourceName").toString();
        QMetaObject::invokeMethod(item, "updateContent", Qt::DirectConnection,
                                  Q_ARG(QVariant, index),
                                  Q_ARG(QVariant, keys),
                                  Q_ARG(QVariant, values));
    }
}

QString MonitoringView::delegatedPlugin(QDeclarativeItem *item)
{
    QVariant ret;
    QObject *associatedModel;
    int index;

    index = item->property("modelEntryIndex").toInt();
    associatedModel = item->property("associatedModel").value<QObject *>();
    QMetaObject::invokeMethod(associatedModel, "getDelegatedPluginPath", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, ret),
                              Q_ARG(QVariant, index));
    return ret.toString();
}

QDeclarativeItem *MonitoringView::gridViewGetItemAt(QObject *gridView, int index)
{
    QVariant ret;

    QMetaObject::invokeMethod(gridView, "getItemAt", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, ret),
                              Q_ARG(QVariant, index));
    return qobject_cast<QDeclarativeItem *>(ret.value<QObject *>());
}

void MonitoringView::titleViewChanged(int index, const QString &title)
{
    QObject *viewsModel;

    //qDebug() << "title changed for" << index << "with" << title;
    m_models[index].first->setObjectName(title);
    viewsModel = viewsListView()->property("model").value<QObject *>();
    QMetaObject::invokeMethod(viewsModel, "changeTitle", Qt::DirectConnection,
                              Q_ARG(QVariant, index),
                              Q_ARG(QVariant, title));
}

void MonitoringView::createNewView()
{
    QObject *tileView, *graphView;
    QObject *tileModel, *graphModel, *viewsModel;
    int index;

    tileView = tileGridView();
    graphView = graphGridView();

    // Firstly we instantiate ResourceModel.qml
    tileModel = loadQMLListModel("ResourceModel");
    graphModel = loadQMLListModel("ResourceModel");

    // Adding the new entry in the views menu
    viewsModel = viewsListView()->property("model").value<QObject *>();
    index = m_models.size();
    QMetaObject::invokeMethod(viewsModel, "appendView", Qt::DirectConnection,
                              Q_ARG(QVariant, "New view"),
                              Q_ARG(QVariant, index));
    tileModel->setObjectName("New view");
    m_models.append(QPair<QObject *, QObject *>(tileModel, graphModel));
    m_view->rootObject()->setProperty("currentIndexView", index);
    m_view->rootObject()->setProperty("title", "New view");

    // Setting models
    tileView->setProperty("model", QVariant::fromValue(tileModel));
    graphView->setProperty("model", QVariant::fromValue(graphModel));

    connect(tileGridModel(), SIGNAL(update(QVariant, QVariant, QVariant)), SLOT(updateView(QVariant,QVariant,QVariant)));
    connect(graphGridModel(), SIGNAL(update(QVariant, QVariant, QVariant)), SLOT(updateView(QVariant,QVariant,QVariant)));
}

void MonitoringView::viewClicked(int index)
{
    QDeclarativeItem *item;
    QVariant keys, values;
    QObject *tileModel, *graphModel;
    int i = 0, count = 0, modelEntryIndex = 0;

    tileModel = m_models[index].first;
    graphModel = m_models[index].second;

    m_view->rootObject()->setProperty("title", tileModel->objectName());
    m_view->rootObject()->setProperty("currentIndexView", index);

    tileGridView()->setProperty("model", QVariant::fromValue(tileModel));
    graphGridView()->setProperty("model", QVariant::fromValue(graphModel));

    count = tileGridView()->property("count").toInt();
    for (i = 0; i < count; i++) {
        item = gridViewGetItemAt(tileGridView(), i);
        item->setProperty("associatedModel", QVariant::fromValue(tileModel));
        modelEntryIndex = item->property("modelEntryIndex").toInt();
        QMetaObject::invokeMethod(tileModel, "getKeys", Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariant, keys),
                                  Q_ARG(QVariant, modelEntryIndex));
        QMetaObject::invokeMethod(tileModel, "getValues", Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariant, values),
                                  Q_ARG(QVariant, modelEntryIndex));
        updateView(modelEntryIndex, keys, values, tileModel);
    }
    count = graphGridView()->property("count").toInt();
    for (i = 0; i < count; i++) {
        item = gridViewGetItemAt(graphGridView(), i);
        item->setProperty("associatedModel", QVariant::fromValue(graphModel));
        modelEntryIndex = item->property("modelEntryIndex").toInt();

        QMetaObject::invokeMethod(graphModel, "getKeys", Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariant, keys),
                                  Q_ARG(QVariant, modelEntryIndex));
        QMetaObject::invokeMethod(graphModel, "getValues", Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariant, values),
                                  Q_ARG(QVariant, modelEntryIndex));
        updateView(modelEntryIndex, keys, values, graphModel);
    }
}

// This is the slot called when the user click on the node on the right
void MonitoringView::nodeClicked(int modelIndex)
{
    NodeItemModel *node;
    QDeclarativeItem *wellKnownListViewItem;
    QObject *model;

    node = m_model->nodeAt(modelIndex);
    qDebug() << node->objectName() << "clicked";
    qDebug() << "node" << node->objectName();
    m_view->rootObject()->setProperty("currentNode", modelIndex);

    wellKnownListViewItem = wellKnownListView();
    wellKnownListViewItem->setProperty("type", "wellknown");
    model = loadQMLListModel("NodeModel");
    wellKnownListViewItem->setProperty("model", QVariant::fromValue(model));

    foreach(QString group, m_nodeGroups[node->objectName()]) {
        foreach (QString resource, m_coapGroups[group]) {
            QMetaObject::invokeMethod(model, "appendResource", Qt::DirectConnection,
                                      Q_ARG(QVariant, resourceName(resource)),
                                      Q_ARG(QVariant, resource));
        }
    }
}

void MonitoringView::removeResource(int index)
{
    QDeclarativeItem *item = NULL;
    QDeclarativeItem *gridView;
    QObject *gridModel;
    QString resourceName;
    int count, nodeId, modelEntryIndex;

    // Get the corresponding view (tile or graph)
    gridView = qobject_cast<QDeclarativeItem *>(sender());
    count = gridView->property("count").toInt();

    // Find the corresponding item
    // and remap all items with their corresponding model entries
    for (int i = 0; i < count; i++) {
        QDeclarativeItem *currentItem = gridViewGetItemAt(gridView, i);
        modelEntryIndex = currentItem->property("modelEntryIndex").toInt();

        if (modelEntryIndex == index) {
            item = currentItem;
        } else if (modelEntryIndex > index) {
            currentItem->setProperty("modelEntryIndex", modelEntryIndex - 1);
        }
    }
    if (!item)
        return;
    // Find the corresponding CoapEntity and remove the model and the entry from it
    resourceName = item->property("resourceName").toString();
    nodeId = item->property("nodeId").toInt();
    gridModel = gridView->property("model").value<QObject *>();

    for (int i = 0; i < m_entities.count(); i++)
        if (m_entities.at(i)->resourceName() == resourceName && m_entities.at(i)->nodeId() == nodeId)
            m_entities.at(i)->removeMonitoringModelEntry(gridModel, index);

    // Removing the data from the model
    QMetaObject::invokeMethod(gridModel, "remove", Qt::DirectConnection, Q_ARG(int, index));
}

void MonitoringView::displayResource(int nodeId, const QString &name, const QString &drawing)
{
    CoapInterface iface(nodeId);
    QString type, key, value;
    QByteArray res;
    QObject *gridView, *model;
    QVariant keys, values;
    int modelEntryIndex = 0;

    if (drawing.contains(" "))
        return;

    if (drawing == "text" || drawing.startsWith("plugin::text")) {
        gridView = tileGridView();
    } else if (drawing == "bargraph" || drawing.startsWith("plugin::bargraph")) {
        gridView = graphGridView();
    }
    model = gridView->property("model").value<QObject *>();
    type = resourceType(name);
    res  = iface.call(name);

    if (type == "bytearray")
        value = bytearrayToString(res);
    else if (type == "shortarray")
        value = shortarrayToString(res);
    else if (type == "short")
        value = shortToString(res);
    else if (type == "byte")
        value = bytearrayToString(res);
    else if (type == "string")
        value = res;

    key = QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0, 'f');
    modelEntryIndex = model->property("count").toInt();
    QMetaObject::invokeMethod(model, "appendResource", Qt::DirectConnection,
                              Q_ARG(QVariant, nodeId),
                              Q_ARG(QVariant, name),
                              Q_ARG(QVariant, key),
                              Q_ARG(QVariant, value));
    QMetaObject::invokeMethod(model, "getKeys", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, keys),
                              Q_ARG(QVariant, modelEntryIndex));
    QMetaObject::invokeMethod(model, "getValues", Qt::DirectConnection,
                              Q_RETURN_ARG(QVariant, values),
                              Q_ARG(QVariant, modelEntryIndex));
    itemAddedToGridView(gridView);
    updateView(modelEntryIndex, keys, values, model);
}

void MonitoringView::displayMultiPartResource(int nodeId, const QString &name, const QString &drawing)
{
    QObject *gridView = NULL, *model = NULL;
    QString keys, values;

    multipartResource(nodeId, name, keys, values);

    if (drawing == "text" || drawing.startsWith("plugin::text")) {
        gridView = tileGridView();
    } else if (drawing == "bargraph" || drawing.startsWith("plugin::bargraph")) {
        gridView = graphGridView();
    }

    model = gridView->property("model").value<QObject *>();
    QMetaObject::invokeMethod(model, "appendResource", Qt::DirectConnection,
                              Q_ARG(QVariant, nodeId),
                              Q_ARG(QVariant, name),
                              Q_ARG(QVariant, keys),
                              Q_ARG(QVariant, values));
    itemAddedToGridView(gridView);
}

// This is the slot called when the user click on the coap resource on the right
void MonitoringView::coapResourceClicked(const QString &name)
{
    NodeItemModel *node;
    QObject *wellKnownListViewModel;
    QString drawing, type;

    qDebug() << "name" << name << "clicked";

    node = m_model->nodeAt(m_view->rootObject()->property("currentNode").toInt());

    QSettings resourceMapper(RESOURCE_INI_FILENAME, QSettings::IniFormat);

    resourceMapper.beginGroup(name);
    drawing = resourceMapper.value("drawing").toString();
    type = resourceType(name);

    // Simple Resources
    if (type != "multipart") {
        displayResource(node->nodeId(), name, drawing);

        if (drawing.contains(" ")) {
            m_view->rootObject()->setProperty("currentCoapResource", name);
            wellKnownListView()->setProperty("type", "drawing");

            // Clearing and refreshing view to show the available drawing capabilities
            wellKnownListViewModel = wellKnownListView()->property("model").value<QObject *>();
            QMetaObject::invokeMethod(wellKnownListViewModel, "clear", Qt::DirectConnection);

            foreach(QString drawingType, drawing.split(" ")) {
                QMetaObject::invokeMethod(wellKnownListViewModel, "appendDrawingType", Qt::DirectConnection,
                            Q_ARG(QVariant, drawingType));
            }
        } else {
            wellKnownListView()->setProperty("type", "node");
            wellKnownListView()->setProperty("model", QVariant::fromValue(m_nodeModel));
        }
    } else { // Multipart resources
        displayMultiPartResource(node->nodeId(), name, drawing);
    }
    resourceMapper.endGroup();
}

// This is the slot called when the user click on the drawing type on the right
void MonitoringView::drawingTypeClicked(const QString &name)
{
    NodeItemModel *node;
    QString resourceName;

    node = m_model->nodeAt(m_view->rootObject()->property("currentNode").toInt());
    resourceName = m_view->rootObject()->property("currentCoapResource").toString();

    if (!name.isEmpty())
        displayResource(node->nodeId(), resourceName, name);

    wellKnownListView()->setProperty("type", "node");
    wellKnownListView()->setProperty("model", QVariant::fromValue(m_nodeModel));
}

void MonitoringView::addNodeToListView(NodeItemModel *nodeItem)
{
    QDeclarativeItem *wellKnownListView;

    wellKnownListView = this->wellKnownListView();
    m_nodeModel = wellKnownListView->property("model").value<QObject *>();
    wellKnownListView->setProperty("type", "node");
    QMetaObject::invokeMethod(m_nodeModel, "appendNode", Qt::DirectConnection,
                              Q_ARG(QVariant, QString("node %1").arg(QString::number(nodeItem->nodeId()))),
                              Q_ARG(QVariant, m_model->nodesCount() - 1));
}
