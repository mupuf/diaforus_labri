# Add more folders to ship with the application, here
folder_01.source = qml/diase
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =


# The .cpp file which was generated for your project. Feel free to hack it.
SOURCES += \
    main.cpp \
    declarative/line.cpp \
    3rdparty/qcustomplot.cpp \
    mainwindow.cpp \
    eventnotifier.cpp \
    networktopology.cpp \
    sensormodel.cpp \
    view.cpp \
    coapinterface.cpp \
    fancybutton.cpp \
    fancytoolbar.cpp \
    gateway.cpp \
    alarmnotifier.cpp \
    monitoringview.cpp \
    coapentity.cpp \
    bargraph.cpp \
    overlay.cpp \
    deploymentsettings.cpp

HEADERS += declarative/line.h \
    3rdparty/qcustomplot.h \
    mainwindow.h \
    eventnotifier.h \
    networktopology.h \
    sensormodel.h \
    view.h \
    coapinterface.h \
    fancybutton.h \
    fancytoolbar.h \
    helpers.h \
    gateway.h \
    alarmnotifier.h \
    monitoringview.h \
    coapentity.h \
    resourceshelper.h \
    bargraph.h \
    overlay.h \
    deploymentsettings.h

QT += declarative \
    network \
    xml \
    script

OTHER_FILES += \
    qml/diase/main.qml \
    qml/diase/ScrollBar.qml \
    qml/diase/Button.qml \
    qml/diase/Line.qml \
    qml/diase/PIR.qml \
    qml/diase/SPIRIT.qml \
    qml/diase/SEISMIC.qml \
    qml/diase/MapView.qml \
    qml/diase/Path.qml \
    qml/diase/Intruder.qml \
    qml/diase/Sensor.qml \
    qml/diase/Area.qml \
    qml/diase/Range.qml \
    qml/diase/Node.qml \
    qml/diase/MonitoringView.qml \
    qml/diase/NodeModel.qml \
    qml/diase/WellKnownModel.qml \
    qml/diase/ResourceTile.qml \
    qml/diase/ResourceModel.qml \
    qml/diase/ViewModel.qml \
    qml/diase/ResourceView.qml \
    qml/diase/GraphBar.qml

RESOURCES += \
    diase.qrc

FORMS += \
    diase.ui \
    connection_dialog.ui \
    building.ui \
    building_error.ui \
    nodes_dialog.ui \
    hybrid_dialog.ui \
    deployment_table.ui

