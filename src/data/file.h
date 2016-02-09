/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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

#ifndef KBIBTEX_IO_FILE_H
#define KBIBTEX_IO_FILE_H

#include <QList>
#include <QStringList>
#include <QSharedPointer>

#include "element.h"

#include "kbibtexdata_export.h"

class Element;

/**
 * This class represents a bibliographic file such as a BibTeX file
 * (or any other format after proper conversion). The file's content
 * can be accessed using the inherited QList interface (for example
 * list iterators).
 * @see Element
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXDATA_EXPORT File : public QList<QSharedPointer<Element> >
{
public:
    /// enum and flags to differ between entries, macros etc
    /// used for @see #allKeys() and @see #containsKey()
    enum ElementType {
        etEntry = 0x1, etMacro = 0x2, etAll = 0x3
    };
    Q_DECLARE_FLAGS(ElementTypes, ElementType)

    /// used for property map
    const static QString Url;
    const static QString Encoding;
    const static QString StringDelimiter;
    const static QString QuoteComment;
    const static QString KeywordCasing;
    const static QString ProtectCasing;
    const static QString NameFormatting;
    const static QString ListSeparator;

    File();
    File(const File &other);
    ~File();

    /**
      * To fulfill the rule-of-three, here is a copy-assignment operator.
      */
    File &operator= (const File &other);

    /**
     * Check if a given key (e.g. a key for a macro or an id for an entry)
     * is contained in the file object.
     * @see #allKeys() const
     * @return @c the object addressed by the key @c, NULL if no such file has been found
     */
    const QSharedPointer<Element> containsKey(const QString &key, ElementTypes elementTypes = etAll) const;

    /**
     * Retrieves a list of all keys for example from macros or entries.
     * @see #const containsKey(const QString &) const
     * @return list of keys
     */
    QStringList allKeys(ElementTypes elementTypes = etAll) const;

    /**
     * Retrieves a set of all unique values (as text) for a specified
     * field from all entries
     * @param fieldName field name to scan, e.g. "volume"
     * @return list of unique values
     */
    QSet<QString> uniqueEntryValuesSet(const QString &fieldName) const;

    /**
     * Retrieves a list of all unique values (as text) for a specified
     * field from all entries
     * @param fieldName field name to scan, e.g. "volume"
     * @return list of unique values
     */
    QStringList uniqueEntryValuesList(const QString &fieldName) const;

    void setProperty(const QString &key, const QVariant &value);
    QVariant property(const QString &key) const;
    QVariant property(const QString &key, const QVariant &defaultValue) const;
    bool hasProperty(const QString &key) const;
    void setPropertiesToDefault();

    /**
     * Unique numeric identifier for every File instance.
     * @return Unique numeric identifier
     */
    quint64 id() const;

    /**
     * Performs a simple check if the memory of this File
     * instance is properly allocate. No high-level checks,
     * such as on the File instance's content, are performed.
     * @return True if instance's memory is properly allocated
     */
    bool isMemoryValid() const;

private:
    class FilePrivate;
    FilePrivate *d;
};

#endif // KBIBTEX_IO_FILE_H
