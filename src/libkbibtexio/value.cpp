/***************************************************************************
*   Copyright (C) 2004-2009 by Thomas Fischer                             *
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
#include <QString>
#include <QStringList>
#include <QDebug>

#include <file.h>
#include "value.h"

using namespace KBibTeX::IO;


Keyword::Keyword(const Keyword& other)
        : m_text(other.m_text)
{
    // nothing
}

Keyword::Keyword(const QString& text)
        : m_text(text)
{
    // nothing
}

void Keyword::setText(const QString& text)
{
    m_text = text;
}

QString Keyword::text() const
{
    return m_text;
}

void Keyword::replace(const QString &before, const QString &after)
{
    if (m_text == before)
        m_text = after;
}

bool Keyword::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    return m_text.contains(pattern, caseSensitive);
}


KeywordContainer::KeywordContainer()
{
    // nothing
}

KeywordContainer::KeywordContainer(const KeywordContainer& other)
        : QLinkedList<Keyword*>()
{
    for (QLinkedList<Keyword*>::ConstIterator it = other.begin(); it != other.end(); ++it) {
        append(new Keyword(**it));
    }
}

void KeywordContainer::replace(const QString &before, const QString &after)
{
    for (QLinkedList<Keyword*>::Iterator it = begin(); it != end(); ++it) {
        (*it)->replace(before, after);
    }
}

bool KeywordContainer::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    bool result = false;
    for (QLinkedList<Keyword*>::ConstIterator it = begin(); !result && it != end(); ++it) {
        result |= (*it)->containsPattern(pattern, caseSensitive);
    }
    return result;
}


Person::Person(const QString& firstName, const QString& lastName, const QString& prefix, const QString& suffix)
        : m_firstName(firstName), m_lastName(lastName), m_prefix(prefix), m_suffix(suffix)
{
    // nothing
}

Person::Person(const Person& other)
        : m_firstName(other.firstName()), m_lastName(other.lastName()), m_prefix(other.prefix()), m_suffix(other.suffix())
{
    // nothing
}

void Person::setName(const QString& firstName, const QString& lastName, const QString& prefix, const QString& suffix)
{
    m_firstName = firstName;
    m_lastName = lastName;
    m_prefix = prefix;
    m_suffix = suffix;
}

QString Person::firstName() const
{
    return m_firstName;
}

QString Person::lastName() const
{
    return m_lastName;
}

QString Person::prefix() const
{
    return m_prefix;
}

QString Person::suffix() const
{
    return m_suffix;
}

void Person::replace(const QString &before, const QString &after)
{
    if (m_firstName == before)
        m_firstName = after;
    if (m_lastName == before)
        m_lastName = after;
    if (m_prefix == before)
        m_prefix = after;
    if (m_suffix == before)
        m_suffix = after;
}

bool Person::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    return m_firstName.contains(pattern, caseSensitive) ||  m_firstName.contains(pattern, caseSensitive) ||  m_prefix.contains(pattern, caseSensitive) ||  m_suffix.contains(pattern, caseSensitive);
}


PersonContainer::PersonContainer()
{
    // nothing
}

PersonContainer::PersonContainer(const PersonContainer& other)
        : QLinkedList<Person*>()
{
    for (QLinkedList<Person*>::ConstIterator it = other.begin(); it != other.end(); ++it) {
        append(new Person(**it));
    }
}

void PersonContainer::replace(const QString &before, const QString &after)
{
    for (QLinkedList<Person*>::Iterator it = begin(); it != end(); ++it) {
        (*it)->replace(before, after);
    }
}

bool PersonContainer::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    bool result = false;
    for (QLinkedList<Person*>::ConstIterator it = begin(); !result && it != end(); ++it) {
        result |= (*it)->containsPattern(pattern, caseSensitive);
    }
    return result;
}

const QRegExp MacroKey::validMakroKeyChars = QRegExp("![-.:/+_a-zA-Z0-9]");

MacroKey::MacroKey(const MacroKey& other)
        : m_text(other.m_text)
{
    // nothing
}

MacroKey::MacroKey(const QString& text)
        : m_text(text)
{
    // nothing
}

void MacroKey::setText(const QString& text)
{
    m_text = text;
}

QString MacroKey::text() const
{
    return m_text;
}

bool MacroKey::isValid()
{
    return !text().contains(validMakroKeyChars);
}

void MacroKey::replace(const QString &before, const QString &after)
{
    if (m_text == before)
        m_text = after;
}

bool MacroKey::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    return m_text.contains(pattern, caseSensitive);
}


PlainText::PlainText(const PlainText& other)
        : m_text(other.text())
{
    // nothing
}

PlainText::PlainText(const QString& text)
        : m_text(text)
{
    // nothing
}

void PlainText::setText(const QString& text)
{
    m_text = text;
}

QString PlainText::text() const
{
    return m_text;
}

void PlainText::replace(const QString &before, const QString &after)
{
    if (m_text == before)
        m_text = after;
}

bool PlainText::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    return m_text.contains(pattern, caseSensitive);
}


Value::Value()
{
    // nothing
}

Value::Value(const Value& other)
        : QLinkedList<ValueItem*>()
{
    for (QLinkedList<ValueItem*>::ConstIterator it = other.begin(); it != other.end(); ++it) {
        PlainText *plainText = dynamic_cast<PlainText*>(*it);
        if (plainText != NULL)
            append(new PlainText(*plainText));
    }
}

void Value::replace(const QString &before, const QString &after)
{
    for (QLinkedList<ValueItem*>::Iterator it = begin(); it != end(); ++it)
        (*it)->replace(before, after);
}

bool Value::containsPattern(const QString &pattern, Qt::CaseSensitivity caseSensitive) const
{
    bool result = false;
    for (QLinkedList<ValueItem*>::ConstIterator it = begin(); !result && it != end(); ++it) {
        result |= (*it)->containsPattern(pattern, caseSensitive);
    }
    return result;
}

QRegExp PlainTextValue::removeCurlyBrackets = QRegExp("(^|[^\\\\])[{}]");

QString PlainTextValue::text(const Value& value, const File* file)
{
    QString result = "";
    for (QLinkedList<ValueItem*>::ConstIterator it = value.begin(); it != value.end(); ++it) {
        QString nextText = text(**it, file);
        if (!nextText.isNull()) {
            if (!result.isEmpty())
                result.append(" ");
            result.append(nextText);
        }
    }
    return result;
}

QString PlainTextValue::text(const ValueItem& valueItem, const File* /*file*/)
{
    QString result = QString::null;

    const PlainText *plainText = dynamic_cast<const PlainText*>(&valueItem);
    if (plainText != NULL)
        result = plainText->text();
    else {
        const MacroKey *macroKey = dynamic_cast<const MacroKey*>(&valueItem);
        if (macroKey != NULL)
            result = macroKey->text(); // TODO Use File to resolve key to full text
        else {
            const PersonContainer *personContainer = dynamic_cast<const PersonContainer*>(&valueItem);
            if (personContainer != NULL) {
                result = "";
                int count = 0;
                for (PersonContainer::ConstIterator it = personContainer->begin(); it != personContainer->end(); ++it, ++count) {
                    if (count > 0) {
                        if (count < personContainer->size() - 1)
                            result += ", ";
                        else if (personContainer->size() > 2)
                            result += ", and ";
                        else
                            result += " and ";
                    }
                    result += (*it)->firstName() + " " + (*it)->lastName();
                }
            } else {
                const KeywordContainer *keywordContainer = dynamic_cast<const KeywordContainer*>(&valueItem);
                if (keywordContainer != NULL) {
                    result = "";
                    bool first = true;
                    for (KeywordContainer::ConstIterator it = keywordContainer->begin(); it != keywordContainer->end(); ++it) {
                        if (!first) result += "; ";
                        first = false;
                        result += (*it)->text();
                    }
                }
            }
        }
    }

    int i = -1;
    while ((i = result.indexOf(removeCurlyBrackets, i + 1)) >= 0) {
        result = result.replace(removeCurlyBrackets.cap(0), removeCurlyBrackets.cap(1));
    }

    return result;
}


