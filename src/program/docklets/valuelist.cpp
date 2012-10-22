/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
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

#include <typeinfo>

#include <QTreeView>
#include <QHeaderView>
#include <QGridLayout>
#include <QStringListModel>
#include <QScrollBar>
#include <QTimer>
#include <QSortFilterProxyModel>

#include <KComboBox>
#include <KStandardDirs>
#include <KConfigGroup>
#include <KLocale>
#include <KAction>
#include <KToggleAction>

#include "bibtexfields.h"
#include "entry.h"
#include "bibtexeditor.h"
#include "valuelistmodel.h"
#include "valuelist.h"

class ValueList::ValueListPrivate
{
private:
    ValueList *p;
    ValueListDelegate *delegate;

public:
    KSharedConfigPtr config;
    const QString configGroupName;
    const QString configKeyFieldName, configKeyShowCountColumn, configKeySortByCountAction, configKeyHeaderState;

    BibTeXEditor *editor;
    QTreeView *treeviewFieldValues;
    ValueListModel *model;
    QSortFilterProxyModel *sortingModel;
    KComboBox *comboboxFieldNames;
    const int countWidth;
    KToggleAction *showCountColumnAction;
    KToggleAction *sortByCountAction;

    ValueListPrivate(ValueList *parent)
            : p(parent), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("Value List Docklet")),
          configKeyFieldName(QLatin1String("FieldName")), configKeyShowCountColumn(QLatin1String("ShowCountColumn")),
          configKeySortByCountAction(QLatin1String("SortByCountAction")), configKeyHeaderState(QLatin1String("HeaderState")),
          model(NULL), sortingModel(NULL), countWidth(8 + parent->fontMetrics().width(i18n("Count"))) {
        setupGUI();
        initialize();
    }

    void setupGUI() {
        QGridLayout *layout = new QGridLayout(p);
        layout->setMargin(0);

        comboboxFieldNames = new KComboBox(true, p);
        layout->addWidget(comboboxFieldNames, 0, 0, 1, 1);

        treeviewFieldValues = new QTreeView(p);
        layout->addWidget(treeviewFieldValues, 1, 0, 1, 1);
        treeviewFieldValues->setEditTriggers(QAbstractItemView::EditKeyPressed);
        treeviewFieldValues->setSortingEnabled(true);
        delegate = new ValueListDelegate(treeviewFieldValues);
        treeviewFieldValues->setItemDelegate(delegate);
        treeviewFieldValues->setRootIsDecorated(false);
        treeviewFieldValues->setSelectionMode(QTreeView::ExtendedSelection);
        treeviewFieldValues->setAlternatingRowColors(true);

        treeviewFieldValues->setContextMenuPolicy(Qt::ActionsContextMenu);
        /// create context menu item to start renaming
        KAction *action = new KAction(KIcon("edit-rename"), i18n("Replace all occurrences"), p);
        connect(action, SIGNAL(triggered()), p, SLOT(startItemRenaming()));
        treeviewFieldValues->addAction(action);
        /// create context menu item to delete value
        action = new KAction(KIcon("edit-table-delete-row"), i18n("Delete all occurrences"), p);
        connect(action, SIGNAL(triggered()), p, SLOT(deleteAllOccurrences()));
        treeviewFieldValues->addAction(action);
        /// create context menu item to search for multiple selections
        action = new KAction(KIcon("edit-find"), i18n("Search for selected values"), p);
        connect(action, SIGNAL(triggered()), p, SLOT(searchSelection()));
        treeviewFieldValues->addAction(action);

        p->setEnabled(false);

        connect(comboboxFieldNames, SIGNAL(activated(int)), p, SLOT(update()));
        connect(treeviewFieldValues, SIGNAL(activated(const QModelIndex &)), p, SLOT(listItemActivated(const QModelIndex &)));
        connect(delegate, SIGNAL(closeEditor(QWidget *)), treeviewFieldValues, SLOT(reset()));

        /// add context menu to header
        treeviewFieldValues->header()->setContextMenuPolicy(Qt::ActionsContextMenu);
        showCountColumnAction = new KToggleAction(i18n("Show Count Column"), treeviewFieldValues);
        connect(showCountColumnAction, SIGNAL(triggered()), p, SLOT(showCountColumnToggled()));
        treeviewFieldValues->header()->addAction(showCountColumnAction);

        sortByCountAction = new KToggleAction(i18n("Sort by Count"), treeviewFieldValues);
        connect(sortByCountAction, SIGNAL(triggered()), p, SLOT(sortByCountToggled()));
        treeviewFieldValues->header()->addAction(sortByCountAction);
    }

    void setComboboxFieldNamesCurrentItem(const QString &text) {
        int index = comboboxFieldNames->findText(text, Qt::MatchExactly);
        if (index < 0) index = comboboxFieldNames->findText(text, Qt::MatchStartsWith);
        if (index < 0) index = comboboxFieldNames->findText(text, Qt::MatchContains);
        if (index >= 0) comboboxFieldNames->setCurrentIndex(index);
    }

    void initialize() {
        const BibTeXFields *bibtexFields = BibTeXFields::self();

        comboboxFieldNames->clear();
        foreach(const FieldDescription *fd, *bibtexFields) {
            if (!fd->upperCamelCaseAlt.isEmpty()) continue; /// keep only "single" fields and not combined ones like "Author or Editor"
            if (fd->upperCamelCase.startsWith('^')) continue; /// skip "type" and "id"
            comboboxFieldNames->addItem(fd->label, fd->upperCamelCase);
        }

        KConfigGroup configGroup(config, configGroupName);
        QString fieldName = configGroup.readEntry(configKeyFieldName, QString(Entry::ftAuthor));
        setComboboxFieldNamesCurrentItem(fieldName);
        showCountColumnAction->setChecked(configGroup.readEntry(configKeyShowCountColumn, true));
        sortByCountAction->setChecked(configGroup.readEntry(configKeySortByCountAction, false));
        sortByCountAction->setEnabled(!showCountColumnAction->isChecked());
        QByteArray headerState = configGroup.readEntry(configKeyHeaderState, QByteArray());
        treeviewFieldValues->header()->restoreState(headerState);

        connect(treeviewFieldValues->header(), SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), p, SLOT(columnsChanged()));
    }

    void update() {
        QVariant var = comboboxFieldNames->itemData(comboboxFieldNames->currentIndex());
        QString text = var.toString();
        if (text.isEmpty()) text = comboboxFieldNames->currentText();

        delegate->setFieldName(text);
        model = editor == NULL ? NULL : editor->valueListModel(text);
        QAbstractItemModel *usedModel = model;
        if (usedModel != NULL) {
            model->setShowCountColumn(showCountColumnAction->isChecked());
            model->setSortBy(sortByCountAction->isChecked() ? ValueListModel::SortByCount : ValueListModel::SortByText);

            if (sortingModel != NULL) delete sortingModel;
            sortingModel = new QSortFilterProxyModel(p);
            sortingModel->setSourceModel(model);
            if (treeviewFieldValues->header()->isSortIndicatorShown())
                sortingModel->sort(treeviewFieldValues->header()->sortIndicatorSection(), treeviewFieldValues->header()->sortIndicatorOrder());
            else
                sortingModel->sort(1, Qt::DescendingOrder);
            sortingModel->setSortRole(SortRole);
            usedModel = sortingModel;
        }
        treeviewFieldValues->setModel(usedModel);
        treeviewFieldValues->header()->setResizeMode(QHeaderView::Fixed);

        KConfigGroup configGroup(config, configGroupName);
        configGroup.writeEntry(configKeyFieldName, text);
        config->sync();
    }
};

ValueList::ValueList(QWidget *parent)
        : QWidget(parent), d(new ValueListPrivate(this))
{
    QTimer::singleShot(500, this, SLOT(delayedResize()));
}

ValueList::~ValueList()
{
    delete d;
}

void ValueList::setEditor(BibTeXEditor *editor)
{
    d->editor = editor;
    update();
    resizeEvent(NULL);
}

void ValueList::update()
{
    d->update();
    setEnabled(d->editor != NULL);
}

void ValueList::resizeEvent(QResizeEvent *)
{
    int widgetWidth = d->treeviewFieldValues->size().width() - d->treeviewFieldValues->verticalScrollBar()->size().width() - 8;
    d->treeviewFieldValues->setColumnWidth(0, widgetWidth - d->countWidth);
    d->treeviewFieldValues->setColumnWidth(1, d->countWidth);
}

void ValueList::listItemActivated(const QModelIndex &index)
{
    QString itemText = d->sortingModel->mapToSource(index).data(SearchTextRole).toString();
    QVariant fieldVar = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex());
    QString fieldText = fieldVar.toString();
    if (fieldText.isEmpty()) fieldText = d->comboboxFieldNames->currentText();

    SortFilterBibTeXFileModel::FilterQuery fq;
    fq.terms << itemText;
    fq.combination = SortFilterBibTeXFileModel::EveryTerm;
    fq.field = fieldText;
    fq.searchPDFfiles = false;

    d->editor->setFilterBarFilter(fq);
}

void ValueList::searchSelection()
{
    QVariant fieldVar = d->comboboxFieldNames->itemData(d->comboboxFieldNames->currentIndex());
    QString fieldText = fieldVar.toString();
    if (fieldText.isEmpty()) fieldText = d->comboboxFieldNames->currentText();

    SortFilterBibTeXFileModel::FilterQuery fq;
    fq.combination = SortFilterBibTeXFileModel::EveryTerm;
    fq.field = fieldText;
    foreach(const QModelIndex &index, d->treeviewFieldValues->selectionModel()->selectedIndexes()) {
        if (index.column() == 0) {
            QString itemText = index.data(SearchTextRole).toString();
            fq.terms << itemText;
        }
    }
    fq.searchPDFfiles = false;

    if (!fq.terms.isEmpty())
        d->editor->setFilterBarFilter(fq);
}

void ValueList::startItemRenaming()
{
    /// Get current index from sorted model
    QModelIndex sortedIndex = d->treeviewFieldValues->currentIndex();
    /// Make the tree view start and editing delegate on the index
    d->treeviewFieldValues->edit(sortedIndex);
}

void ValueList::deleteAllOccurrences()
{
    /// Get current index from sorted model
    QModelIndex sortedIndex = d->treeviewFieldValues->currentIndex();
    /// Get "real" index from original model, but resort to sibling in first column
    QModelIndex realIndex = d->sortingModel->mapToSource(sortedIndex);
    realIndex = realIndex.sibling(realIndex.row(), 0);

    /// Remove current index from data model
    d->model->removeValue(realIndex);
    /// Notify main editor about change it its data
    d->editor->externalModification();
}

void ValueList::showCountColumnToggled()
{
    if (d->model != NULL)
        d->model->setShowCountColumn(d->showCountColumnAction->isChecked());
    if (d->showCountColumnAction->isChecked())
        resizeEvent(NULL);

    d->sortByCountAction->setEnabled(!d->showCountColumnAction->isChecked());

    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeyShowCountColumn, d->showCountColumnAction->isChecked());
    d->config->sync();
}

void ValueList::sortByCountToggled()
{
    if (d->model != NULL)
        d->model->setSortBy(d->sortByCountAction->isChecked() ? ValueListModel::SortByCount : ValueListModel::SortByText);

    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeySortByCountAction, d->sortByCountAction->isChecked());
    d->config->sync();
}

void ValueList::delayedResize()
{
    resizeEvent(NULL);
}

void ValueList::columnsChanged()
{
    QByteArray headerState = d->treeviewFieldValues->header()->saveState();
    KConfigGroup configGroup(d->config, d->configGroupName);
    configGroup.writeEntry(d->configKeyHeaderState, headerState);
    d->config->sync();

    resizeEvent(NULL);
}
