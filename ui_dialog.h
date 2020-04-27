/********************************************************************************
** Form generated from reading UI file 'dialog.ui'
**
** Created by: Qt User Interface Compiler version 5.13.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOG_H
#define UI_DIALOG_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextBrowser>

QT_BEGIN_NAMESPACE

class Ui_Dialog
{
public:
    QPushButton *cancel;
    QLabel *label;
    QTextBrowser *logshow;
    QPushButton *sender;

    void setupUi(QDialog *Dialog)
    {
        if (Dialog->objectName().isEmpty())
            Dialog->setObjectName(QString::fromUtf8("Dialog"));
        Dialog->resize(693, 516);
        cancel = new QPushButton(Dialog);
        cancel->setObjectName(QString::fromUtf8("cancel"));
        cancel->setGeometry(QRect(560, 470, 112, 32));
        label = new QLabel(Dialog);
        label->setObjectName(QString::fromUtf8("label"));
        label->setGeometry(QRect(20, 20, 58, 16));
        logshow = new QTextBrowser(Dialog);
        logshow->setObjectName(QString::fromUtf8("logshow"));
        logshow->setGeometry(QRect(20, 40, 651, 411));
        sender = new QPushButton(Dialog);
        sender->setObjectName(QString::fromUtf8("sender"));
        sender->setGeometry(QRect(430, 470, 112, 32));

        retranslateUi(Dialog);
        QObject::connect(cancel, SIGNAL(clicked()), Dialog, SLOT(close()));

        QMetaObject::connectSlotsByName(Dialog);
    } // setupUi

    void retranslateUi(QDialog *Dialog)
    {
        Dialog->setWindowTitle(QCoreApplication::translate("Dialog", "FTP\346\234\215\345\212\241\345\231\250", nullptr));
        cancel->setText(QCoreApplication::translate("Dialog", "\351\200\200\345\207\272", nullptr));
        label->setText(QCoreApplication::translate("Dialog", "\346\227\245\345\277\227", nullptr));
        sender->setText(QCoreApplication::translate("Dialog", "\345\217\221\351\200\201", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Dialog: public Ui_Dialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOG_H
