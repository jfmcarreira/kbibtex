/***************************************************************************
*   Copyright (C) 2004-2010 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#include <QList>
#include <QLabel>
#include <QLayout>
#include <QListWidget>
#include <QTextEdit>
#include <QBuffer>

#include <KDebug>
#include <KComboBox>
#include <KLineEdit>
#include <KLocale>
#include <KPushButton>

#include <bibtexentries.h>
#include <bibtexfields.h>
#include <fileexporterbibtex.h>
#include <fieldinput.h>
#include <value.h>
#include <entry.h>
#include <entrylayout.h>
#include "entryviewer.h"

class EntryViewer::EntryViewerPrivate
{
private:
    const Entry *entry;
    EntryViewer *p;

public:
    EntryViewerPrivate(const Entry *e, EntryViewer *parent)
            : entry(e), p(parent) {
        // TODO
    }

    void createGUI() {
        createReferenceGUI();

        BibTeXFields *bf = BibTeXFields::self();
        EntryLayout *el = EntryLayout::self();

        for (EntryLayout::ConstIterator elit = el->constBegin(); elit != el->constEnd(); ++elit) {
            EntryTabLayout etl = *elit;
            QWidget *container = new QWidget(p);
            p->addTab(container, etl.uiCaption);

            QGridLayout *layout = new QGridLayout(container);

            int mod = etl.singleFieldLayouts.size() / etl.columns;
            if (etl.singleFieldLayouts.size() % etl.columns > 0)
                ++mod;
            layout->setRowStretch(mod, 1);

            int col = 0, row = 0;
            for (QList<SingleFieldLayout>::ConstIterator sflit = etl.singleFieldLayouts.constBegin(); sflit != etl.singleFieldLayouts.constEnd(); ++sflit) {
                QLabel *label = new QLabel((*sflit).uiLabel + ":", container);
                layout->addWidget(label, row, col, 1, 1);
                label->setAlignment(Qt::AlignTop | Qt::AlignRight);

                const FieldDescription *fd = bf->find((*sflit).bibtexLabel);
                KBibTeX::TypeFlags typeFlags = fd == NULL ? KBibTeX::tfSource : fd->typeFlags;
                FieldInput *fieldInput = new FieldInput((*sflit).fieldInputLayout, typeFlags, container);
                layout->addWidget(fieldInput, row, col + 1, 1, 1);
                layout->setColumnStretch(col, 0);
                if ((*sflit).fieldInputLayout == KBibTeX::MultiLine || (*sflit).fieldInputLayout == KBibTeX::List)
                    layout->setRowStretch(row, 100);
                else {
                    layout->setRowStretch(row, 0);
                    layout->setAlignment(fieldInput, Qt::AlignTop);
                }
                layout->setColumnStretch(col + 1, 2);
                label->setBuddy(fieldInput);

                p->bibtexKeyToWidget.insert((*sflit).bibtexLabel, fieldInput);

                ++row;
                if (row >= mod) {
                    col += 2;
                    row = 0;
                }
            }
        }

        createOtherFieldsGUI();
        createSourceGUI();
    }

    void reset() {
        reset(entry);
    }

    void reset(const Entry *entry) {
        p->entryId->setText(entry->id());
        BibTeXEntries *be = BibTeXEntries::self();
        QString type = be->format(entry->type(), KBibTeX::cUpperCamelCase);
        p->comboboxType->lineEdit()->setText(type);
        type = type.toLower();
        for (BibTeXEntries::ConstIterator it = be->constBegin(); it != be->constEnd(); ++it)
            if (type == it->upperCamelCase.toLower()) {
                p->comboboxType->lineEdit()->setText(it->label);
                break;
            }

        for (QMap<QString, FieldInput*>::Iterator it = p->bibtexKeyToWidget.begin(); it != p->bibtexKeyToWidget.end(); ++it)
            it.value()->clear();

        for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it) {
            const QString key = it.key().toLower();
            if (p->bibtexKeyToWidget.contains(key)) {
                FieldInput *fieldInput = p->bibtexKeyToWidget[key];
                fieldInput->setValue(it.value());
            }
        }

        resetSource(entry);
    }

    void resetSource() {
        resetSource(entry);
    }

    void resetSource(const Entry *entry) {
        FileExporterBibTeX exporter;
        QBuffer textBuffer;
        textBuffer.open(QIODevice::WriteOnly);
        exporter.save(&textBuffer, entry, NULL);
        textBuffer.close();
        textBuffer.open(QIODevice::ReadOnly);
        QTextStream ts(&textBuffer);
        QString text = ts.readAll();
        p->sourceEdit->document()->setPlainText(text);
    }

    void setReadOnly(bool isReadOnly) {
        p->entryId->setReadOnly(isReadOnly);
        p->comboboxType->lineEdit()->setReadOnly(isReadOnly);
        for (QMap<QString, FieldInput*>::Iterator it = p->bibtexKeyToWidget.begin(); it != p->bibtexKeyToWidget.end(); ++it)
            it.value()->setReadOnly(isReadOnly);
    }

private:
    void createReferenceGUI() {
        QWidget *container = new QWidget(p);
        p->addTab(container, i18n("Reference"));
        QGridLayout *layout = new QGridLayout(container);
        layout->setColumnStretch(0, 0);
        layout->setColumnStretch(1, 1);
        layout->setRowStretch(0, 0);
        layout->setRowStretch(1, 0);
        layout->setRowStretch(2, 1);

        QLabel *label = new QLabel(i18n("Type:"), container);
        layout->addWidget(label, 0, 0, 1, 1);
        p->comboboxType = new KComboBox(container);
        p->comboboxType->setEditable(true);
        layout->addWidget(p->comboboxType, 0, 1, 1, 1);
        label->setBuddy(p->comboboxType);

        label = new QLabel(i18n("Id:"), container);
        layout->addWidget(label, 1, 0, 1, 1);
        p->entryId = new KLineEdit(container);
        layout->addWidget(p->entryId, 1, 1, 1, 1);
        label->setBuddy(p->entryId);

        BibTeXEntries *be = BibTeXEntries::self();
        for (BibTeXEntries::ConstIterator it = be->constBegin(); it != be->constEnd(); ++it)
            p->comboboxType->addItem(it->label, it->upperCamelCase);
    }

    void createOtherFieldsGUI() {
        QWidget *container = new QWidget(p);
        p->addTab(container, i18n("Other Fields"));
        QGridLayout *layout = new QGridLayout(container);
        layout->setColumnStretch(0, 0);
        layout->setColumnStretch(1, 1);
        layout->setColumnStretch(2, 0);
        layout->setRowStretch(0, 0);
        layout->setRowStretch(1, 1);
        layout->setRowStretch(2, 0);
        layout->setRowStretch(3, 0);
        layout->setRowStretch(4, 1);

        QLabel *label = new QLabel(i18n("Name:"), container);
        layout->addWidget(label, 0, 0, 1, 1);
        p->otherFieldsName = new KLineEdit(container);
        layout->addWidget(p->otherFieldsName, 0, 1, 1, 1);
        label->setBuddy(p->otherFieldsName);

        KPushButton *buttonAddApply = new KPushButton(i18n("Add/Apply"), container);
        layout->addWidget(buttonAddApply, 0, 2, 1, 1);

        label = new QLabel(i18n("Content:"), container);
        layout->addWidget(label, 1, 0, 1, 1);
        p->otherFieldsContent = new FieldInput(KBibTeX::MultiLine, KBibTeX::tfSource, container);
        layout->addWidget(p->otherFieldsContent, 1, 1, 1, 2);
        label->setBuddy(p->otherFieldsContent);

        label = new QLabel(i18n("List:"), container);
        layout->addWidget(label, 2, 0, 3, 1);
        p->otherFieldsList = new QListWidget(container);
        layout->addWidget(p->otherFieldsList, 2, 1, 3, 1);
        label->setBuddy(p->otherFieldsList);
        KPushButton *buttonDelete = new KPushButton(i18n("Delete"), container);
        layout->addWidget(buttonDelete, 2, 2, 1, 1);
        KPushButton *buttonOpen = new KPushButton(i18n("Open"), container);
        layout->addWidget(buttonOpen, 3, 2, 1, 1);
    }

    void createSourceGUI() {
        QWidget *container = new QWidget(p);
        p->addTab(container, i18n("Source"));
        QGridLayout *layout = new QGridLayout(container);
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 0);
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 0);

        p->sourceEdit = new QTextEdit(container);
        layout->addWidget(p->sourceEdit, 0, 0, 1, 2);
        p->sourceEdit->document()->setDefaultFont(KGlobalSettings::fixedFont());

        KPushButton *buttonRestore = new KPushButton(i18n("Restore"), container);
        layout->addWidget(buttonRestore, 1, 1, 1, 1);
        connect(buttonRestore, SIGNAL(clicked()), p, SLOT(resetSource()));
    }
};

EntryViewer::EntryViewer(const Entry *entry, QWidget *parent)
        : QTabWidget(parent), d(new EntryViewerPrivate(entry, this))
{
    d->createGUI();
    setReadOnly(true);
}

void EntryViewer::reset()
{
    d->reset();
}

void EntryViewer::setReadOnly(bool isReadOnly)
{
    d->setReadOnly(isReadOnly);
}

void EntryViewer::resetSource(const Entry *entry)
{
    d->resetSource(entry);
}

void EntryViewer::resetSource()
{
    d->resetSource();
}
