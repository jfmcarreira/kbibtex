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

#include <QDropEvent>
#include <QTimer>

#include <KDialog>
#include <KLocale>
#include <KMessageBox>
#include <KGuiItem>
#include <KConfigGroup>
#include <KDebug>

#include "elementeditor.h"
#include "entry.h"
#include "macro.h"
#include "bibtexfilemodel.h"
#include "fileexporterbibtex.h"
#include "valuelistmodel.h"
#include "bibtexeditor.h"

/**
 * Specialized dialog for element editing. It will check if the used
 * element editor widget has unapplied changes and ask the user if
 * he/she actually wants to discard those changes before closing this
 * dialog.
 *
 * @author Thomas Fischer
 */
class ElementEditorDialog : public KDialog
{
private:
    ElementEditor *elementEditor;
    static const QString configGroupNameWindowSize;
    KConfigGroup configGroup;

public:
    ElementEditorDialog(QWidget *parent)
            : KDialog(parent), elementEditor(NULL) {
        /// restore window size
        KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
        configGroup = KConfigGroup(config, configGroupNameWindowSize);
        restoreDialogSize(configGroup);
    }

    /**
     * Store element editor widget for future reference.
     */
    void setElementEditor(ElementEditor *elementEditor) {
        this->elementEditor = elementEditor;
        setMainWidget(elementEditor);
    }

protected:
    virtual void closeEvent(QCloseEvent *e) {
        /// strangely enough, close events have always to be rejected ...
        e->setAccepted(false);
        KDialog::closeEvent(e);
    }

protected Q_SLOTS:
    /// Will be triggered when closing the dialog
    /// given a re-implementation of closeEvent as above
    virtual void slotButtonClicked(int button) {
        /// save window size of Ok is clicked
        if (button == KDialog::Ok)
            saveDialogSize(configGroup);

        /// ignore button event if it is from the Cancel button
        /// and the user does not want to discard his/her changes
        if (button != KDialog::Cancel || allowedToClose())
            KDialog::slotButtonClicked(button);
    }

private:
    bool allowedToClose() {
        /// save window size
        saveDialogSize(configGroup);

        /// if there unapplied changes in the editor widget ...
        /// ... ask user for consent to discard changes ...
        /// only the allow to close this dialog
        return !elementEditor->elementUnapplied() || KMessageBox::warningContinueCancel(this, i18n("The current entry has been modified. Do you want do discard your changes?"), i18n("Discard changes?"), KGuiItem(i18n("Discard"), "edit-delete-shred"), KGuiItem(i18n("Continue Editing"), "edit-rename")) == KMessageBox::Continue;
    }
};

const QString ElementEditorDialog::configGroupNameWindowSize = QLatin1String("ElementEditorDialog");

BibTeXEditor::BibTeXEditor(const QString &name, QWidget *parent)
        : BibTeXFileView(name, parent), m_isReadOnly(false), m_current(NULL), m_filterBar(NULL), m_elementEditorDialog(NULL), m_elementEditor(NULL)
{
    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(itemActivated(QModelIndex)));
}

void BibTeXEditor::viewCurrentElement()
{
    viewElement(currentElement());
}

void BibTeXEditor::viewElement(const QSharedPointer<Element> element)
{
    prepareEditorDialog(DialogTypeView);
    m_elementEditor->setElement(element, bibTeXModel()->bibTeXFile());

    m_elementEditorDialog->exec();
}

void BibTeXEditor::editCurrentElement()
{
    editElement(currentElement());
}

bool BibTeXEditor::editElement(QSharedPointer<Element> element)
{
    prepareEditorDialog(DialogTypeEdit);
    m_elementEditor->setElement(element, bibTeXModel()->bibTeXFile());

    m_elementEditorDialog->exec();

    if (!isReadOnly()) {
        bool changed = m_elementEditor->elementChanged();
        if (changed) {
            emit currentElementChanged(currentElement(), bibTeXModel()->bibTeXFile());
            emit selectedElementsChanged();
            emit modified();
        }
        return changed;
    } else
        return false;
}

const QList<QSharedPointer<Element> > &BibTeXEditor::selectedElements() const
{
    return m_selection;
}

void BibTeXEditor::setSelectedElements(QList<QSharedPointer<Element> > &list)
{
    m_selection = list;

    QItemSelectionModel *selModel = selectionModel();
    selModel->clear();
    for (QList<QSharedPointer<Element> >::ConstIterator it = list.constBegin(); it != list.constEnd(); ++it) {
        int row = bibTeXModel()->row(*it);
        for (int col = model()->columnCount(QModelIndex()) - 1; col >= 0; --col) {
            QModelIndex idx = model()->index(row, col);
            selModel->setCurrentIndex(idx, QItemSelectionModel::Select);
        }
    }
}

void BibTeXEditor::setSelectedElement(QSharedPointer<Element> element)
{
    m_selection.clear();
    m_selection << element;

    QItemSelectionModel *selModel = selectionModel();
    selModel->clear();
    int row = bibTeXModel()->row(element);
    for (int col = model()->columnCount(QModelIndex()) - 1; col >= 0; --col) {
        QModelIndex idx = model()->index(row, col);
        selModel->setCurrentIndex(idx, QItemSelectionModel::Select);
    }
}

const QSharedPointer<Element> BibTeXEditor::currentElement() const
{
    return m_current;
}

QSharedPointer<Element> BibTeXEditor::currentElement()
{
    return m_current;
}

QSharedPointer<Element> BibTeXEditor::elementAt(const QModelIndex &index)
{
    return bibTeXModel()->element(sortFilterProxyModel()->mapToSource(index).row());
}

void BibTeXEditor::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    QTreeView::currentChanged(current, previous); // FIXME necessary?

    m_current = bibTeXModel()->element(sortFilterProxyModel()->mapToSource(current).row());
    emit currentElementChanged(m_current, bibTeXModel()->bibTeXFile());
}

void BibTeXEditor::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QTreeView::selectionChanged(selected, deselected);

    QModelIndexList set = selected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.append(bibTeXModel()->element((*it).row()));
    }
    if (m_current == NULL && !set.isEmpty())
        m_current = bibTeXModel()->element(set.first().row());

    set = deselected.indexes();
    for (QModelIndexList::Iterator it = set.begin(); it != set.end(); ++it) {
        m_selection.removeOne(bibTeXModel()->element((*it).row()));
    }

    emit selectedElementsChanged();
}

void BibTeXEditor::selectionDelete()
{
    QModelIndexList mil = selectionModel()->selectedRows();
    QList<int> rows;
    foreach(QModelIndex idx, mil)
    rows << sortFilterProxyModel()->mapToSource(idx).row();

    bibTeXModel()->removeRowList(rows);

    emit modified();
}

void BibTeXEditor::externalModification()
{
    emit modified();
}

void BibTeXEditor::setReadOnly(bool isReadOnly)
{
    m_isReadOnly = isReadOnly;
}

bool BibTeXEditor::isReadOnly() const
{
    return m_isReadOnly;
}

ValueListModel *BibTeXEditor::valueListModel(const QString &field)
{
    BibTeXFileModel *bibteXModel = bibTeXModel();
    if (bibteXModel != NULL) {
        ValueListModel *result = new ValueListModel(bibteXModel->bibTeXFile(), field, this);
        /// Keep track of external changes through modifications in this ValueListModel instance
        connect(result, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(externalModification()));
        return result;
    }

    return NULL;
}

void BibTeXEditor::setFilterBar(FilterBar *filterBar)
{
    m_filterBar = filterBar;
}

void BibTeXEditor::setFilterBarFilter(SortFilterBibTeXFileModel::FilterQuery fq)
{
    if (m_filterBar != NULL)
        m_filterBar->setFilter(fq);
}

void BibTeXEditor::mouseMoveEvent(QMouseEvent *event)
{
    emit editorMouseEvent(event);
}

void BibTeXEditor::dragEnterEvent(QDragEnterEvent *event)
{
    emit editorDragEnterEvent(event);
}

void BibTeXEditor::dropEvent(QDropEvent *event)
{
    if (event->source() != this)
        emit editorDropEvent(event);
}

void BibTeXEditor::dragMoveEvent(QDragMoveEvent *event)
{
    emit editorDragMoveEvent(event);
}

void BibTeXEditor::itemActivated(const QModelIndex &index)
{
    emit elementExecuted(bibTeXModel()->element(sortFilterProxyModel()->mapToSource(index).row()));
}

void BibTeXEditor::prepareEditorDialog(DialogType dialogType)
{
    if (dialogType != DialogTypeView && isReadOnly()) {
        kWarning() << "In read-only mode, you may only view elements, not edit them";
        dialogType = DialogTypeView;
    }

    /// Create both the dialog window and the editing widget only once
    if (m_elementEditorDialog == NULL)
        m_elementEditorDialog = new ElementEditorDialog(this);
    if (m_elementEditor == NULL) {
        m_elementEditor = new ElementEditor(m_elementEditorDialog);
        m_elementEditorDialog->setElementEditor(m_elementEditor);
    }

    /// Disconnect all signals that could modify current element
    /// (safety precaution for read-only mode, signals will be
    /// re-established for edit mode further below)
    disconnect(m_elementEditor, SIGNAL(modified(bool)), m_elementEditorDialog, SLOT(enableButtonApply(bool)));
    disconnect(m_elementEditorDialog, SIGNAL(applyClicked()), m_elementEditor, SLOT(apply()));
    disconnect(m_elementEditorDialog, SIGNAL(okClicked()), m_elementEditor, SLOT(apply()));
    disconnect(m_elementEditorDialog, SIGNAL(resetClicked()), m_elementEditor, SLOT(reset()));

    if (dialogType == DialogTypeView) {
        /// View mode, as use in read-only situations
        m_elementEditor->setReadOnly(true);
        m_elementEditorDialog->setCaption(i18n("View Element"));
        m_elementEditorDialog->setButtons(KDialog::Close);
    } else if (dialogType == DialogTypeEdit) {
        /// Edit mode, used in normal operations
        m_elementEditor->setReadOnly(false);
        m_elementEditorDialog->setCaption(i18n("Edit Element"));
        m_elementEditorDialog->setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel | KDialog::Reset);
        m_elementEditorDialog->enableButton(KDialog::Apply, false);

        /// Establish signal-slot connections for modification/editing events
        connect(m_elementEditor, SIGNAL(modified(bool)), m_elementEditorDialog, SLOT(enableButtonApply(bool)));
        connect(m_elementEditorDialog, SIGNAL(applyClicked()), m_elementEditor, SLOT(apply()));
        connect(m_elementEditorDialog, SIGNAL(okClicked()), m_elementEditor, SLOT(apply()));
        connect(m_elementEditorDialog, SIGNAL(resetClicked()), m_elementEditor, SLOT(reset()));
    }
}
