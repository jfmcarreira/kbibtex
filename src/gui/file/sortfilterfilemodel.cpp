/***************************************************************************
 *   Copyright (C) 2004-2015 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/

#include "sortfilterfilemodel.h"

#include <typeinfo>

#include <KSharedConfig>
#include <KConfigGroup>

#include "bibtexfields.h"
#include "bibtexentries.h"
#include "macro.h"
#include "preamble.h"
#include "comment.h"
#include "fileinfo.h"

static const QRegExp curlyRegExp(QLatin1String("[{}]+"));

const QString SortFilterFileModel::configGroupName = QLatin1String("User Interface");

SortFilterFileModel::SortFilterFileModel(QObject *parent)
        : QSortFilterProxyModel(parent), m_internalModel(NULL), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")))
{
    m_filterQuery.combination = AnyTerm;
    loadState();
    setSortRole(FileModel::SortRole);
}

void SortFilterFileModel::setSourceModel(QAbstractItemModel *model)
{
    QSortFilterProxyModel::setSourceModel(model);
    m_internalModel = dynamic_cast<FileModel *>(model);
}

FileModel *SortFilterFileModel::fileSourceModel() const
{
    return m_internalModel;
}

void SortFilterFileModel::updateFilter(SortFilterFileModel::FilterQuery filterQuery)
{
    m_filterQuery = filterQuery;
    m_filterQuery.field = filterQuery.field.toLower(); /// required for comparison in filter code
    invalidate();
}

bool SortFilterFileModel::simpleLessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const QString leftString = left.data(Qt::DisplayRole).toString().toLower();
    const QString rightString = right.data(Qt::DisplayRole).toString().toLower();
    const int cmp = QString::localeAwareCompare(leftString, rightString);
    if (cmp == 0)
        return left.row() < right.row();
    else
        return cmp < 0;
}

bool SortFilterFileModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    int column = left.column();
    Q_ASSERT_X(left.column() == right.column(), "bool SortFilterFileModel::lessThan(const QModelIndex &left, const QModelIndex &right) const", "Not comparing items in same column"); ///< assume that we only sort by column

    const BibTeXFields *bibtexFields = BibTeXFields::self();
    const FieldDescription *fd = bibtexFields->at(column);

    if (column == right.column() && (fd->upperCamelCase == QLatin1String("Author") || fd->upperCamelCase == QLatin1String("Editor"))) {
        /// special sorting for authors or editors: check all names,
        /// compare last and then first names

        /// first, check if two entries (and not e.g. comments) are to be compared
        QSharedPointer<Entry> entryA = m_internalModel->element(left.row()).dynamicCast<Entry>();
        QSharedPointer<Entry> entryB = m_internalModel->element(right.row()).dynamicCast<Entry>();
        if (entryA.isNull() || entryB.isNull())
            return simpleLessThan(left, right);

        /// retrieve values of both cells
        Value valueA = entryA->value(fd->upperCamelCase);
        Value valueB = entryB->value(fd->upperCamelCase);
        if (valueA.isEmpty())
            valueA = entryA->value(fd->upperCamelCaseAlt);
        if (valueB.isEmpty())
            valueB = entryB->value(fd->upperCamelCaseAlt);

        /// if either value is empty, use default implementation
        if (valueA.isEmpty() || valueB.isEmpty())
            return simpleLessThan(left, right);

        /// compare each person in both values
        for (Value::Iterator itA = valueA.begin(), itB = valueB.begin(); itA != valueA.end() &&  itB != valueB.end(); ++itA, ++itB) {
            QSharedPointer<Person>  personA = (*itA).dynamicCast<Person>();
            QSharedPointer<Person>  personB = (*itB).dynamicCast<Person>();
            /// not a Person object in value? fall back to default implementation
            if (personA.isNull() || personB.isNull()) return QSortFilterProxyModel::lessThan(left, right);

            /// get both values' next persons' last names for comparison
            QString nameA = personA->lastName().remove(curlyRegExp).toLower();
            QString nameB = personB->lastName().remove(curlyRegExp).toLower();
            int cmp = QString::localeAwareCompare(nameA, nameB);
            if (cmp < 0) return true;
            if (cmp > 0) return false;

            /// if last names were inconclusive ...
            /// get both values' next persons' first names for comparison
            nameA = personA->firstName().remove(curlyRegExp).toLower();
            nameB = personB->firstName().remove(curlyRegExp).toLower();
            cmp = QString::localeAwareCompare(nameA, nameB);
            if (cmp < 0) return true;
            if (cmp > 0) return false;

            // TODO Check for suffix and prefix?
        }

        /// comparison by names did not work (was not conclusive)
        /// fall back to default implementation
        return simpleLessThan(left, right);
    } else {
        /// if comparing two numbers, do not perform lexicographical sorting (i.e. 13 < 2),
        /// but numerical sorting instead (i.e. 13 > 2)
        const QString textLeft = left.data(Qt::DisplayRole).toString();
        const QString textRight = right.data(Qt::DisplayRole).toString();
        bool okLeft = false, okRight = false;
        int numberLeft = textLeft.toInt(&okLeft);
        int numberRight = textRight.toInt(&okRight);
        if (okLeft && okRight)
            return numberLeft < numberRight;


        /// everything else can be sorted by default implementation
        /// (i.e. alphabetically or lexicographically)
        return simpleLessThan(left, right);
    }
}

bool SortFilterFileModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_parent)

    QSharedPointer<Element> rowElement = m_internalModel->element(source_row);
    Q_ASSERT_X(!rowElement.isNull(), "bool SortFilterFileModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const", "rowElement is NULL");

    /// check if showing comments is disabled
    if (!m_showComments && typeid(*rowElement) == typeid(Comment))
        return false;
    /// check if showing macros is disabled
    if (!m_showMacros && typeid(*rowElement) == typeid(Macro))
        return false;

    if (m_filterQuery.terms.isEmpty()) return true; /// empty filter query

    bool *eachTerm = new bool[m_filterQuery.terms.count()];
    for (int i = m_filterQuery.terms.count() - 1; i >= 0; --i)
        eachTerm[i] = false;

    QSharedPointer<Entry> entry = rowElement.dynamicCast<Entry>();
    if (!entry.isNull()) {
        /// if current row contains an Entry ...

        if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^id")) {
            /// Check entry's id
            const QString id = entry->id();
            int i = 0;
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                eachTerm[i] |= (*itsl).isEmpty() ? true : id.contains(*itsl, Qt::CaseInsensitive);
        }

        if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
            /// Check entry's type
            const QString type = entry->type();
            /// Check type's description ("Journal Article")
            const QString label = BibTeXEntries::self()->label(type);
            // TODO test for internationalized variants like "Artikel" or "bok" as well?
            int i = 0;
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                eachTerm[i] |= (*itsl).isEmpty() ? true : type.contains(*itsl, Qt::CaseInsensitive) || label.contains(*itsl, Qt::CaseInsensitive);
        }

        for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it)
            if (m_filterQuery.field.isEmpty() || m_filterQuery.field == it.key().toLower()) {
                int i = 0;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                    eachTerm[i] |= (*itsl).isEmpty() ? true : it.value().containsPattern(*itsl);
            }

        /// Test associated PDF files
        if (m_filterQuery.searchPDFfiles && m_filterQuery.field.isEmpty()) ///< not filtering for any specific field
            foreach (const QUrl &url, FileInfo::entryUrls(entry.data(), fileSourceModel()->bibliographyFile()->property(File::Url, QUrl()).toUrl(), FileInfo::TestExistenceYes)) {
                if (url.isLocalFile() && url.fileName().endsWith(QLatin1String(".pdf"))) {
                    // FIXME if you have a large collection of PDF files and the text version
                    // has not been generated yet, this will freeze KBibTeX for some time
                    const QString text = FileInfo::pdfToText(url.url(QUrl::PreferLocalFile));
                    int i = 0;
                    for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                        eachTerm[i] |= (*itsl).isEmpty() ? true : text.contains(*itsl, Qt::CaseInsensitive);
                }
            }

        int i = 0;
        if (m_filterQuery.field.isEmpty())
            for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                eachTerm[i] |= (*itsl).isEmpty() ? true : entry->id().contains(*itsl, Qt::CaseInsensitive);
    } else {
        QSharedPointer<Macro> macro = rowElement.dynamicCast<Macro>();
        if (!macro.isNull()) {
            if (m_filterQuery.field.isEmpty()) {
                int i = 0;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                    eachTerm[i] |= macro->value().containsPattern(*itsl, Qt::CaseInsensitive) || macro->key().contains(*itsl, Qt::CaseInsensitive);
            }

            if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
                static const QString label = QLatin1String("macro");
                int i = 0;
                for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                    eachTerm[i] = eachTerm[i] || label.contains(*itsl, Qt::CaseInsensitive);
            }
        } else {
            QSharedPointer<Comment> comment = rowElement.dynamicCast<Comment>();
            if (!comment.isNull()) {
                if (m_filterQuery.field.isEmpty()) {
                    int i = 0;
                    for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                        eachTerm[i] |= (*itsl).isEmpty() ? true : comment->text().contains(*itsl, Qt::CaseInsensitive);
                }

                if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
                    static const QString label = QLatin1String("comment");
                    int i = 0;
                    for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                        eachTerm[i] = eachTerm[i] || label.contains(*itsl, Qt::CaseInsensitive);
                }
            } else {
                QSharedPointer<Preamble> preamble = rowElement.dynamicCast<Preamble>();
                if (!preamble.isNull()) {
                    if (m_filterQuery.field.isEmpty()) {
                        int i = 0;
                        for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                            eachTerm[i] |= preamble->value().containsPattern(*itsl, Qt::CaseInsensitive);
                    }

                    if (m_filterQuery.field.isEmpty() || m_filterQuery.field == QLatin1String("^type")) {
                        static const QString label = QLatin1String("preamble");
                        int i = 0;
                        for (QStringList::ConstIterator itsl = m_filterQuery.terms.constBegin(); itsl != m_filterQuery.terms.constEnd(); ++itsl, ++i)
                            eachTerm[i] = eachTerm[i] || label.contains(*itsl, Qt::CaseInsensitive);
                    }
                }
            }
        }
    }

    bool every = true, any = false;
    for (int i = m_filterQuery.terms.count() - 1; i >= 0; --i) {
        every &= eachTerm[i];
        any |= eachTerm[i];
    }
    delete[] eachTerm;

    if (m_filterQuery.combination == SortFilterFileModel::AnyTerm)
        return any;
    else
        return every;
}

void SortFilterFileModel::loadState()
{
    KConfigGroup configGroup(config, configGroupName);
    m_showComments = configGroup.readEntry(FileModel::keyShowComments, FileModel::defaultShowComments);
    m_showMacros = configGroup.readEntry(FileModel::keyShowMacros, FileModel::defaultShowMacros);
}