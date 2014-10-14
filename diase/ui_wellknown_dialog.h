/********************************************************************************
** Form generated from reading UI file 'wellknown_dialog.ui'
**
** Created: Thu Mar 29 17:47:57 2012
**      by: Qt User Interface Compiler version 4.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WELLKNOWN_DIALOG_H
#define UI_WELLKNOWN_DIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListWidget>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_WellKnownDialog
{
public:
    QVBoxLayout *verticalLayout;
    QLabel *label;
    QListWidget *listWidget;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *WellKnownDialog)
    {
        if (WellKnownDialog->objectName().isEmpty())
            WellKnownDialog->setObjectName(QString::fromUtf8("WellKnownDialog"));
        WellKnownDialog->resize(320, 240);
        verticalLayout = new QVBoxLayout(WellKnownDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        label = new QLabel(WellKnownDialog);
        label->setObjectName(QString::fromUtf8("label"));

        verticalLayout->addWidget(label);

        listWidget = new QListWidget(WellKnownDialog);
        listWidget->setObjectName(QString::fromUtf8("listWidget"));

        verticalLayout->addWidget(listWidget);

        buttonBox = new QDialogButtonBox(WellKnownDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(WellKnownDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), WellKnownDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), WellKnownDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(WellKnownDialog);
    } // setupUi

    void retranslateUi(QDialog *WellKnownDialog)
    {
        WellKnownDialog->setWindowTitle(QApplication::translate("WellKnownDialog", "Dialog", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("WellKnownDialog", "Please select a service for node %1", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class WellKnownDialog: public Ui_WellKnownDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WELLKNOWN_DIALOG_H
