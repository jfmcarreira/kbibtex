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

#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QStringList>
#include <QRegExp>

#include <KDebug>
#include <KSharedConfig>
#include <KConfigGroup>

#include "preferences.h"
#include "file.h"
#include "entry.h"
#include "element.h"
#include "macro.h"
#include "comment.h"

const QString File::Url = QLatin1String("Url");
const QString File::Encoding = QLatin1String("Encoding");
const QString File::StringDelimiter = QLatin1String("StringDelimiter");
const QString File::QuoteComment = QLatin1String("QuoteComment");
const QString File::KeywordCasing = QLatin1String("KeywordCasing");
const QString File::ProtectCasing = QLatin1String("ProtectCasing");
const QString File::NameFormatting = QLatin1String("NameFormatting");

const quint64 valid = 0x08090a0b0c0d0e0f;
const quint64 invalid = 0x0102030405060708;

class File::FilePrivate
{
private:
    File *p;
    quint64 validInvalidField;
    static quint64 instanceCounter;
    const quint64 instanceNumber;

    KSharedConfigPtr config;
    const QString configGroupName;

public:
    QMap<QString, QVariant> properties;

    FilePrivate(File *parent)
            : p(parent), validInvalidField(valid), instanceNumber(++instanceCounter), config(KSharedConfig::openConfig(QLatin1String("kbibtexrc"))), configGroupName(QLatin1String("FileExporterBibTeX")) {
        kDebug() << "Creating File instance" << instanceNumber;
        loadConfiguration();
    }

    ~FilePrivate() {
        kDebug() << "Deleting File instance" << instanceNumber;
        validInvalidField = invalid;
    }

    void loadConfiguration() {
        /// Load and set configuration as stored in settings
        KConfigGroup configGroup(config, configGroupName);
        properties.insert(File::Encoding, configGroup.readEntry(Preferences::keyEncoding, Preferences::defaultEncoding));
        properties.insert(File::StringDelimiter, configGroup.readEntry(Preferences::keyStringDelimiter, Preferences::defaultStringDelimiter));
        properties.insert(File::QuoteComment, (Preferences::QuoteComment)configGroup.readEntry(Preferences::keyQuoteComment, (int)Preferences::defaultQuoteComment));
        properties.insert(File::KeywordCasing, (KBibTeX::Casing)configGroup.readEntry(Preferences::keyKeywordCasing, (int)Preferences::defaultKeywordCasing));
        properties.insert(File::NameFormatting,  configGroup.readEntry(Person::keyPersonNameFormatting, ""));
        properties.insert(File::ProtectCasing, configGroup.readEntry(Preferences::keyProtectCasing, Preferences::defaultProtectCasing));
    }

    bool checkValidity() {
        return validInvalidField == valid;
    }
};

quint64 File::FilePrivate::instanceCounter = 99999;

File::File()
        : QList<QSharedPointer<Element> >(), d(new FilePrivate(this))
{
    // nothing
}

File::File(const File &other)
        : QList<QSharedPointer<Element> >(other), d(new FilePrivate(this))
{
    // nothing
}

File::~File()
{
    Q_ASSERT_X(d->checkValidity(), "File::~File()", "This File object is not valid");
    delete d;
}

const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const
{
    Q_ASSERT_X(d->checkValidity(), "const QSharedPointer<Element> File::containsKey(const QString &key, ElementTypes elementTypes) const", "This File object is not valid");
    foreach(const QSharedPointer<Element> element, *this) {
        const QSharedPointer<Entry> entry = elementTypes.testFlag(etEntry) ? element.dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull()) {
            if (entry->id() == key)
                return entry;
        } else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(etMacro) ? element.dynamicCast<Macro>() : QSharedPointer<Macro>();
            if (!macro.isNull()) {
                if (macro->key() == key)
                    return macro;
            }
        }
    }

    return QSharedPointer<Element>();
}

QStringList File::allKeys(ElementTypes elementTypes) const
{
    Q_ASSERT_X(d->checkValidity(), "QStringList File::allKeys(ElementTypes elementTypes) const", "This File object is not valid");
    QStringList result;

    foreach(const QSharedPointer<Element> element, *this) {
        const QSharedPointer<Entry> entry = elementTypes.testFlag(etEntry) ? element.dynamicCast<Entry>() : QSharedPointer<Entry>();
        if (!entry.isNull())
            result.append(entry->id());
        else {
            const QSharedPointer<Macro> macro = elementTypes.testFlag(etMacro) ? element.dynamicCast<Macro>() : QSharedPointer<Macro>();
            if (!macro.isNull())
                result.append(macro->key());
        }
    }

    return result;
}

QSet<QString> File::uniqueEntryValuesSet(const QString &fieldName) const
{
    Q_ASSERT_X(d->checkValidity(), "QSet<QString> File::uniqueEntryValuesSet(const QString &fieldName) const", "This File object is not valid");
    QSet<QString> valueSet;
    const QString lcFieldName = fieldName.toLower();

    foreach(const QSharedPointer<Element> element, *this) {
        const QSharedPointer<Entry> entry = element.dynamicCast<Entry>();
        if (!entry.isNull())
            for (Entry::ConstIterator it = entry->constBegin(); it != entry->constEnd(); ++it)
                if (it.key().toLower() == lcFieldName)
                    foreach(const QSharedPointer<ValueItem> &valueItem, it.value()) {
                    /// Check if ValueItem to process points to a person
                    const QSharedPointer<Person> person = valueItem.dynamicCast<Person>();
                    if (!person.isNull()) {
                        /// Assemble a list of formatting templates for a person's name
                        static QStringList personNameFormattingList; ///< use static to do pattern assembly only once
                        if (personNameFormattingList.isEmpty()) {
                            /// Use the two default patterns last-name-first and first-name-first
                            personNameFormattingList << QLatin1String("<%l><, %s><, %f>") << QLatin1String("<%f ><%l>< %s>");
                            /// Check configuration if user-specified formatting template is different
                            KSharedConfigPtr config(KSharedConfig::openConfig(QLatin1String("kbibtexrc")));
                            KConfigGroup configGroup(config, "General");
                            QString personNameFormatting = configGroup.readEntry(Person::keyPersonNameFormatting, Person::defaultPersonNameFormatting);
                            /// Add user's template if it differs from the two specified above
                            if (!personNameFormattingList.contains(personNameFormatting))
                                personNameFormattingList << personNameFormatting;
                        }
                        /// Add person's name formatted using each of the templates assembled above
                        foreach(const QString &personNameFormatting, personNameFormattingList) {
                            valueSet.insert(Person::transcribePersonName(person.data(), personNameFormatting));
                        }
                    } else {
                        /// Default case: use PlainTextValue::text to translate ValueItem
                        /// to a human-readable text
                        valueSet.insert(PlainTextValue::text(*valueItem, this));
                    }
                }
    }

    return valueSet;
}

QStringList File::uniqueEntryValuesList(const QString &fieldName) const
{
    Q_ASSERT_X(d->checkValidity(), "QStringList File::uniqueEntryValuesList(const QString &fieldName) const", "This File object is not valid");
    QSet<QString> valueSet = uniqueEntryValuesSet(fieldName);
    QStringList list = valueSet.toList();
    list.sort();
    return list;
}

void File::setProperty(const QString &key, const QVariant &value)
{
    Q_ASSERT_X(d->checkValidity(), "void File::setProperty(const QString &key, const QVariant &value)", "This File object is not valid");
    d->properties.insert(key, value);
}

QVariant File::property(const QString &key) const
{
    Q_ASSERT_X(d->checkValidity(), "QVariant File::property(const QString &key) const", "This File object is not valid");
    return d->properties.contains(key) ? d->properties.value(key) : QVariant();
}

QVariant File::property(const QString &key, const QVariant &defaultValue) const
{
    Q_ASSERT_X(d->checkValidity(), "QVariant File::property(const QString &key, const QVariant &defaultValue) const", "This File object is not valid");
    return d->properties.value(key, defaultValue);
}

bool File::hasProperty(const QString &key) const
{
    Q_ASSERT_X(d->checkValidity(), "bool File::hasProperty(const QString &key) const", "This File object is not valid");
    return d->properties.contains(key);
}

void File::setPropertiesToDefault()
{
    Q_ASSERT_X(d->checkValidity(), "void File::setPropertiesToDefault()", "This File object is not valid");
    d->loadConfiguration();
}