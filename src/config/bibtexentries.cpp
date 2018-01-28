/***************************************************************************
 *   Copyright (C) 2004-2017 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
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
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "bibtexentries.h"

#include <QStandardPaths>

#ifdef HAVE_KF5
#include <KLocalizedString>
#include <KSharedConfig>
#include <KConfigGroup>
#endif // HAVE_KF5

#include "logging_config.h"

bool operator==(const EntryDescription &a, const EntryDescription &b)
{
    return a.upperCamelCase == b.upperCamelCase;
}
uint qHash(const EntryDescription &a)
{
    return qHash(a.upperCamelCase);
}

class BibTeXEntries::BibTeXEntriesPrivate
{
public:
    static const QVector<EntryDescription> entryDescriptionsBibTeX;
    static const QVector<EntryDescription> entryDescriptionsBibLaTeX;

    static BibTeXEntries *singleton;
};

/// awk -F '=' '/^\[/ && OptionalItems!="" {print "<< EntryDescription{QStringLiteral(\""UpperCamelCase"\"), QStringLiteral(\""UpperCamelCaseAlt"\"), i18n(\""Label"\"),{QStringLiteral(\""RequiredItems"\")},{QStringLiteral(\""OptionalItems"\")}}" ; OptionalItems=""} $1=="UpperCamelCase" {UpperCamelCase=$2;UpperCamelCaseAlt=""} $1=="UpperCamelCaseAlt" {UpperCamelCaseAlt=$2} $1=="Label" {Label=$2} $1=="RequiredItems" {RequiredItems=$2} $1=="OptionalItems" {OptionalItems=$2}' <bibtex.kbstyle | sed -r 's!([a-z]),([a-z])!\1"), QStringLiteral("\2!g;s!QStringLiteral\(""\)!QString()!g;s!\{QString\(\)\}!{}!g'
const QVector<EntryDescription> entryDescriptionsBibTeX = QVector<EntryDescription>()
        << EntryDescription {QStringLiteral("Article"), QString(), i18n("Journal Article"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("journal"), QStringLiteral("year")}, {QStringLiteral("volume"), QStringLiteral("number"), QStringLiteral("pages"), QStringLiteral("month"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("InProceedings"), i18n("Conference"), QStringLiteral("Publication in Conference Proceedings"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year")}, {QStringLiteral("editor"), QStringLiteral("volume^number"), QStringLiteral("series"), QStringLiteral("pages"), QStringLiteral("address"), QStringLiteral("month"), QStringLiteral("organization"), QStringLiteral("publisher"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("Proceedings"), QString(), i18n("Conference or Workshop Proceedings"), {QStringLiteral("title"), QStringLiteral("year")}, {QStringLiteral("editor"), QStringLiteral("volume^number"), QStringLiteral("series"), QStringLiteral("address"), QStringLiteral("month"), QStringLiteral("organization"), QStringLiteral("publisher"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("TechReport"), QString(), i18n("Technical Report"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("institution"), QStringLiteral("year")}, {QStringLiteral("type"), QStringLiteral("number"), QStringLiteral("address"), QStringLiteral("month"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("Misc"), QString(), i18n("Miscellaneous"), {}, {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("howpublished"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("Book"), QString(), i18n("Book"), {QStringLiteral("author^editor"), QStringLiteral("title"), QStringLiteral("publisher"), QStringLiteral("year")}, {QStringLiteral("volume^number"), QStringLiteral("series"), QStringLiteral("address"), QStringLiteral("edition"), QStringLiteral("month"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("InBook"), QString(), i18n("Part of a Book"), {QStringLiteral("author^editor"), QStringLiteral("title"), QStringLiteral("chapter|pages"), QStringLiteral("publisher"), QStringLiteral("year")}, {QStringLiteral("volume^number"), QStringLiteral("series"), QStringLiteral("type"), QStringLiteral("address"), QStringLiteral("edition"), QStringLiteral("month"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("InCollection"), QString(), i18n("Part of a Book with own Title"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("publisher"), QStringLiteral("year")}, {QStringLiteral("editor"), QStringLiteral("volume^number"), QStringLiteral("series"), QStringLiteral("type"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("address"), QStringLiteral("edition"), QStringLiteral("month"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("PhDThesis"), QString(), i18n("PhD Thesis"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("school"), QStringLiteral("year")}, {QStringLiteral("type"), QStringLiteral("address"), QStringLiteral("month"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("MastersThesis"), QString(), i18n("Master's Thesis"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("school"), QStringLiteral("year")}, {QStringLiteral("type"), QStringLiteral("address"), QStringLiteral("month"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("Unpublished"), QString(), i18n("Unpublished Material"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("note")}, {QStringLiteral("month"), QStringLiteral("year")}}
        << EntryDescription {QStringLiteral("Manual"), QString(), i18n("Manual"), {QStringLiteral("title")}, {QStringLiteral("author"), QStringLiteral("organization"), QStringLiteral("address"), QStringLiteral("edition"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("note")}}
        << EntryDescription {QStringLiteral("Booklet"), QString(), i18n("Booklet"), {QStringLiteral("title")}, {QStringLiteral("author"), QStringLiteral("howpublished"), QStringLiteral("address"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("note")}};
const QVector<EntryDescription> entryDescriptionsBibLaTeX = QVector<EntryDescription>()
        << EntryDescription {QStringLiteral("Article"), QString(), i18n("Journal Article"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("journaltitle"), QStringLiteral("year^date")}, {QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("editor"), QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("journalsubtitle"), QStringLiteral("issuetitle"), QStringLiteral("issuesubtitle"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("series"), QStringLiteral("volume"), QStringLiteral("number"), QStringLiteral("eid"), QStringLiteral("issue"), QStringLiteral("month"), QStringLiteral("pages"), QStringLiteral("version"), QStringLiteral("note"), QStringLiteral("issn"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Book"), QString(), i18n("Book"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("editor"), QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("MVBook"), QString(), i18n("Multi-volume Book"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("editor"), QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("InBook"), QString(), i18n("Part of a Book"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year^date")}, {QStringLiteral("bookauthor"), QStringLiteral("editor"), QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("booksubtitle"), QStringLiteral("booktitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("BookInBook"), QString(), i18n("Former Monograph as Part of a Book"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year^date")}, {QStringLiteral("bookauthor"), QStringLiteral("editor"), QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("booksubtitle"), QStringLiteral("booktitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("SuppBook"), QString(), i18n("Supplemental Material in a Book"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year^date")}, {QStringLiteral("bookauthor"), QStringLiteral("editor"), QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("booksubtitle"), QStringLiteral("booktitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Booklet"), QString(), i18n("Booklet"), {QStringLiteral("author^editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("howpublished"), QStringLiteral("type"), QStringLiteral("note"), QStringLiteral("location"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Collection"), QString(), i18n("Single-volume Collection"), {QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("MVCollection"), QString(), i18n("Multi-volume Collection"), {QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("InCollection"), QString(), i18n("Part of a Book with own Title"), {QStringLiteral("author"), QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("booksubtitle"), QStringLiteral("booktitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("SuppCollection"), QString(), i18n("Supplemental Material in a Collection"), {QStringLiteral("author"), QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("booksubtitle"), QStringLiteral("booktitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Manual"), QString(), i18n("Manual"), {QStringLiteral("author^editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("edition"), QStringLiteral("type"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("version"), QStringLiteral("note"), QStringLiteral("organization"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Misc"), QString(), i18n("Miscellaneous"), {QStringLiteral("author^editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("howpublished"), QStringLiteral("type"), QStringLiteral("version"), QStringLiteral("note"), QStringLiteral("organization"), QStringLiteral("location"), QStringLiteral("date"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Online"), QStringLiteral("Electronic"), i18n("Online Resource"), {QStringLiteral("author^editor"), QStringLiteral("title"), QStringLiteral("year^date"), QStringLiteral("url")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("version"), QStringLiteral("note"), QStringLiteral("organization"), QStringLiteral("date"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Patent"), QString(), i18n("Patent"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("number"), QStringLiteral("year^date")}, {QStringLiteral("holder"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("type"), QStringLiteral("version"), QStringLiteral("location"), QStringLiteral("note"), QStringLiteral("date"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Periodical"), QString(), i18n("Periodical (Complete Issue)"), {QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("subtitle"), QStringLiteral("issuetitle"), QStringLiteral("issuesubtitle"), QStringLiteral("language"), QStringLiteral("series"), QStringLiteral("volume"), QStringLiteral("number"), QStringLiteral("issue"), QStringLiteral("date"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("note"), QStringLiteral("issn"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("SuppPeriodical"), QString(), i18n("Supplemental Material in a Periodical"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("journaltitle"), QStringLiteral("year^date")}, {QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("editor"), QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("journalsubtitle"), QStringLiteral("issuetitle"), QStringLiteral("issuesubtitle"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("series"), QStringLiteral("volume"), QStringLiteral("number"), QStringLiteral("eid"), QStringLiteral("issue"), QStringLiteral("month"), QStringLiteral("pages"), QStringLiteral("version"), QStringLiteral("note"), QStringLiteral("issn"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Proceedings"), QString(), i18n("Conference or Workshop Proceedings"), {QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("eventtitle"), QStringLiteral("eventdate"), QStringLiteral("venue"), QStringLiteral("language"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("organization"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("MVProceedings"), QString(), i18n("Multi-volume Proceedings"), {QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("eventtitle"), QStringLiteral("eventdate"), QStringLiteral("venue"), QStringLiteral("language"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("organization"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isbn"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("InProceedings"), QStringLiteral("Conference"), i18n("Publication in Conference Proceedings"), {QStringLiteral(" author"), QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("booksubtitle"), QStringLiteral("booktitleaddon"), QStringLiteral("eventtitle"), QStringLiteral("eventdate"), QStringLiteral("venue"), QStringLiteral("language"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("organization"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Reference"), QString(), i18n("Single-volume Reference (e.g. Encyclopedia)"), {QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("MVReference"), QString(), i18n("Multi-volume Reference (e.g. Encyclopedia)"), {QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("InReference"), QString(), i18n("Part of a Reference"), {QStringLiteral("author"), QStringLiteral("editor"), QStringLiteral("title"), QStringLiteral("booktitle"), QStringLiteral("year^date")}, {QStringLiteral("editora"), QStringLiteral("editorb"), QStringLiteral("editorc"), QStringLiteral("translator"), QStringLiteral("annotator"), QStringLiteral("commentator"), QStringLiteral("introduction"), QStringLiteral("foreword"), QStringLiteral("afterword"), QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("maintitle"), QStringLiteral("mainsubtitle"), QStringLiteral("maintitleaddon"), QStringLiteral("booksubtitle"), QStringLiteral("booktitleaddon"), QStringLiteral("language"), QStringLiteral("origlanguage"), QStringLiteral("volume"), QStringLiteral("part"), QStringLiteral("edition"), QStringLiteral("volumes"), QStringLiteral("series"), QStringLiteral("number"), QStringLiteral("note"), QStringLiteral("publisher"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Report"), QString(), i18n("Report"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("type"), QStringLiteral("institution"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("number"), QStringLiteral("version"), QStringLiteral("note"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isrn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Thesis"), QString(), i18n("Thesis"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("type"), QStringLiteral("institution"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("note"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("Unpublished"), QString(), i18n("Unpublished Material"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("language"), QStringLiteral("howpublished"), QStringLiteral("note"), QStringLiteral("location"), QStringLiteral("isbn"), QStringLiteral("date"), QStringLiteral("month"), QStringLiteral("year"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("MastersThesis"), QString(), i18n("Master's Thesis"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("institution"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("type"), QStringLiteral("language"), QStringLiteral("note"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("PhDThesis"), QString(), i18n("PhD Thesis"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("institution"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("type"), QStringLiteral("language"), QStringLiteral("note"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isbn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}}
        << EntryDescription {QStringLiteral("TechReport"), QString(), i18n("Technical Report"), {QStringLiteral("author"), QStringLiteral("title"), QStringLiteral("institution"), QStringLiteral("year^date")}, {QStringLiteral("subtitle"), QStringLiteral("titleaddon"), QStringLiteral("type"), QStringLiteral("language"), QStringLiteral("number"), QStringLiteral("version"), QStringLiteral("note"), QStringLiteral("location"), QStringLiteral("month"), QStringLiteral("isrn"), QStringLiteral("chapter"), QStringLiteral("pages"), QStringLiteral("pagetotal"), QStringLiteral("addendum"), QStringLiteral("pubstate"), QStringLiteral("doi"), QStringLiteral("eprint"), QStringLiteral("eprintclass"), QStringLiteral("eprinttype"), QStringLiteral("url"), QStringLiteral("urldate")}};

BibTeXEntries *BibTeXEntries::BibTeXEntriesPrivate::singleton = nullptr;


BibTeXEntries::BibTeXEntries(const QVector<EntryDescription> &other)
        : QVector<EntryDescription>(other), d(new BibTeXEntriesPrivate())
{
    /// nothing
}

BibTeXEntries::~BibTeXEntries()
{
    delete d;
}

const BibTeXEntries *BibTeXEntries::self()
{
    if (BibTeXEntriesPrivate::singleton == nullptr) {
        KSharedConfigPtr config(KSharedConfig::openConfig(QStringLiteral("kbibtexrc")));
        KConfigGroup configGroup(config, QStringLiteral("User Interface"));
        const QString stylefile = configGroup.readEntry("CurrentStyle", "bibtex");
        BibTeXEntriesPrivate::singleton = stylefile == QStringLiteral("biblatex") ? new BibTeXEntries(entryDescriptionsBibLaTeX) : new BibTeXEntries(entryDescriptionsBibTeX);
    }
    return BibTeXEntriesPrivate::singleton;
}

QString BibTeXEntries::format(const QString &name, KBibTeX::Casing casing) const
{
    QString iName = name.toLower();

    switch (casing) {
    case KBibTeX::cLowerCase: return iName;
    case KBibTeX::cUpperCase: return name.toUpper();
    case KBibTeX::cInitialCapital:
        iName[0] = iName[0].toUpper();
        return iName;
    case KBibTeX::cLowerCamelCase: {
        for (const auto &ed : const_cast<const BibTeXEntries &>(*this)) {
            /// configuration file uses camel-case
            QString itName = ed.upperCamelCase.toLower();
            if (itName == iName) {
                iName = ed.upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toLower();
        return iName;
    }
    case KBibTeX::cUpperCamelCase: {
        for (const auto &ed : const_cast<const BibTeXEntries &>(*this)) {
            /// configuration file uses camel-case
            QString itName = ed.upperCamelCase.toLower();
            if (itName == iName) {
                iName = ed.upperCamelCase;
                break;
            }
        }

        /// make an educated guess how camel-case would look like
        iName[0] = iName[0].toUpper();
        return iName;
    }
    }
    return name;
}

QString BibTeXEntries::label(const QString &name) const
{
    const QString iName = name.toLower();

    for (const auto &ed : const_cast<const BibTeXEntries &>(*this)) {
        /// Configuration file uses camel-case, convert this to lower case for faster comparison
        QString itName = ed.upperCamelCase.toLower();
        if (itName == iName || (!(itName = ed.upperCamelCaseAlt.toLower()).isEmpty() && itName == iName))
            return ed.label;
    }
    return QString();
}
