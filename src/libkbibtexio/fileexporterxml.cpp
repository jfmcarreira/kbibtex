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
#include <QRegExp>
#include <QStringList>

#include <file.h>
#include <element.h>
#include <entry.h>
#include <encoderxml.h>
#include <macro.h>
#include <comment.h>
#include "iocommon.h"
#include "fileexporterxml.h"

using namespace KBibTeX::IO;

FileExporterXML::FileExporterXML()
        : FileExporter()
{
    // nothing
}

FileExporterXML::~FileExporterXML()
{
    // nothing
}

bool FileExporterXML::save(QIODevice* iodevice, const File* bibtexfile, QStringList * /*errorLog*/)
{
    m_mutex.lock();
    bool result = true;
    m_cancelFlag = false;
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX/KDE4 -->" << endl;
    stream << "<!-- http://home.gna.org/kbibtex/ -->" << endl;
    stream << "<bibliography>" << endl;

    for (File::ConstIterator it = bibtexfile->begin(); it != bibtexfile->end() && result && !m_cancelFlag; ++it)
        write(stream, *it, bibtexfile);

    stream << "</bibliography>" << endl;

    m_mutex.unlock();
    return result && !m_cancelFlag;
}

bool FileExporterXML::save(QIODevice* iodevice, const Element* element, QStringList * /*errorLog*/)
{
    QTextStream stream(iodevice);
    stream.setCodec("UTF-8");

    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    stream << "<!-- XML document written by KBibTeXIO as part of KBibTeX/KDE4 -->" << endl;
    stream << "<!-- http://home.gna.org/kbibtex/ -->" << endl;
    return write(stream, element);
}

void FileExporterXML::cancel()
{
    m_cancelFlag = true;
}

bool FileExporterXML::write(QTextStream& stream, const Element* element, const File* bibtexfile)
{
    bool result = FALSE;

    const Entry *entry = dynamic_cast<const Entry*>(element);
    if (entry != NULL) {
        if (bibtexfile != NULL) {
// FIXME                entry = bibtexfile->completeReferencedFieldsConst( entry );
            entry = new Entry(*entry);
        }
        result |= writeEntry(stream, entry);
        if (bibtexfile != NULL)
            delete entry;
    } else {
        const Macro * macro = dynamic_cast<const Macro*>(element);
        if (macro != NULL)
            result |= writeMacro(stream, macro);
        else {
            const Comment * comment = dynamic_cast<const Comment*>(element);
            if (comment != NULL)
                result |= writeComment(stream, comment);
            else {
                // preambles are ignored, make no sense in XML files
            }
        }
    }

    return result;
}

bool FileExporterXML::writeEntry(QTextStream &stream, const Entry* entry)
{
    stream << " <entry id=\"" << EncoderXML::currentEncoderXML() ->encode(entry->id()) << "\" type=\"" << entry->type().toLower() << "\">" << endl;
    for (Entry::ConstIterator it = entry->begin(); it != entry->end(); ++it) {
        const QString key = it.key().toLower();
        const Value value = it.value();

        if (key == Entry::ftAuthor || key == Entry::ftEditor) {
            stream << "  <" << key << "s>" << endl;
            stream << valueToXML(value, key) << endl;
            stream << "  </" << key << "s>" << endl;
        } else if (key == Entry::ftMonth) {
            stream << "  <month";
            bool ok = FALSE;

            int month = -1;
            QString tag = "";
            QString content = "";
            for (QList<ValueItem*>::ConstIterator it = value.begin(); it != value.end(); ++it) {
                MacroKey*  macro = dynamic_cast<MacroKey*>(*it);
                if (macro != NULL)
                    for (int i = 0; i < 12; i++) {
                        if (QString::compare(macro->text(), MonthsTriple[ i ]) == 0) {
                            if (month < 1) {
                                tag = MonthsTriple[ i ];
                                month = i + 1;
                            }
                            content.append(Months[ i ]);
                            ok = true;
                            break;
                        }
                    }
                else
                    content.append(PlainTextValue::text(**it));
            }

            if (!ok)
                content = valueToXML(value) ;
            if (!tag.isEmpty())
                stream << " tag=\"" << key << "\"";
            if (month > 0)
                stream << " month=\"" << month << "\"";
            stream << '>' << content;
            stream << "</month>" << endl;
        } else {
            stream << "  <" << key << ">" << valueToXML(value) << "</" << key << ">" << endl;
        }

    }
    stream << " </entry>" << endl;

    return true;
}

bool FileExporterXML::writeMacro(QTextStream &stream, const Macro* macro)
{
    stream << " <string key=\"" << macro->key() << "\">";
    stream << valueToXML(macro->value());
    stream << "</string>" << endl;

    return true;
}

bool FileExporterXML::writeComment(QTextStream &stream, const Comment* comment)
{
    stream << " <comment>" ;
    stream << EncoderXML::currentEncoderXML() ->encode(comment->text());
    stream << "</comment>" << endl;

    return true;
}

QString FileExporterXML::valueToXML(const Value& value, const QString&)
{
    QString result;
    bool isFirst = true;

    for (QList<ValueItem*>::ConstIterator it = value.begin(); it != value.end(); ++it) {
        if (!isFirst)
            result.append(' ');
        isFirst = FALSE;

        ValueItem *item = *it;

        PlainText *plainText = dynamic_cast<PlainText*>(item);
        if (plainText != NULL)
            result.append("<text>" + EncoderXML::currentEncoderXML() ->encode(PlainTextValue::text(*item)) + "</text>");
        else {
            Person *p = dynamic_cast<Person*>(item);
            if (p != NULL) {
                result.append("<person>");
                if (!p->prefix().isEmpty())
                    result.append("<prefix>" + EncoderXML::currentEncoderXML() ->encode(p->lastName()) + "</prefix>");
                if (!p->firstName().isEmpty())
                    result.append("<firstname>" + EncoderXML::currentEncoderXML() ->encode(p->firstName()) + "</firstname>");
                if (!p->lastName().isEmpty())
                    result.append("<lastname>" + EncoderXML::currentEncoderXML() ->encode(p->lastName()) + "</lastname>");
                if (!p->suffix().isEmpty())
                    result.append("<suffix>" + EncoderXML::currentEncoderXML() ->encode(p->suffix()) + "</suffix>");
                result.append("</person>");
            }
            // TODO: Other data types
            else
                result.append("<text>" + EncoderXML::currentEncoderXML() ->encode(PlainTextValue::text(*item)) + "</text>");
        }
    }

    return result;
}

