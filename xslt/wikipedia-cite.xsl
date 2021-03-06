<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as generated by KBibTeX into nice HTML code.
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -->

<xsl:output method="text" standalone="yes" omit-xml-declaration="yes" encoding="UTF-8" media-type="text/plain" />

<xsl:template match="entry">
<xsl:text>{{cite </xsl:text>

<xsl:choose>
<xsl:when test="doi/text">
<xsl:text>doi|</xsl:text>
<xsl:value-of select="doi/text" />
</xsl:when>
<xsl:when test="jstor_id/text">
<xsl:text>jstor|</xsl:text>
<xsl:value-of select="jstor_id/text" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://www.jstor.org/stable/')">
<xsl:text>jstor|</xsl:text>
<xsl:value-of select="substring(url/text, 29)" />
</xsl:when>
<xsl:when test="starts-with(@id, 'jstor')">
<xsl:text>jstor|</xsl:text>
<xsl:value-of select="substring(@id, 6)" />
</xsl:when>
<xsl:when test="pubmed/text">
<xsl:text>pmid|</xsl:text>
<xsl:value-of select="pubmed/text" />
</xsl:when>
<xsl:when test="pmid/text">
<xsl:text>pmid|</xsl:text>
<xsl:value-of select="pmid/text" />
</xsl:when>
<xsl:when test="starts-with(@id, 'pmid')">
<xsl:text>pmid|</xsl:text>
<xsl:value-of select="substring(@id, 5)" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://hdl.handle.net/')">
<xsl:text>hdl|</xsl:text>
<xsl:value-of select="substring(url/text, 23)" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://www.jstor.org/stable/')">
<xsl:text>jstor|</xsl:text>
<xsl:value-of select="substring(url/text, 28)" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://doi.acm.org/')">
<xsl:text>doi|</xsl:text>
<xsl:value-of select="substring(url/text, 19)" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://doi.wiley.com/')">
<xsl:text>doi|</xsl:text>
<xsl:value-of select="substring(url/text, 21)" />
</xsl:when>
<xsl:when test="isbn/text">
<xsl:text>isbn|</xsl:text>
<xsl:value-of select="isbn/text" />
</xsl:when>

<xsl:when test="starts-with(eprint/text, 'arXiv:')">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="substring(eprint/text, 7)" />
</xsl:when>
<xsl:when test="starts-with(arxivId/text, 'arXiv:')">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="substring(arxivId/text, 7)" />
</xsl:when>
<xsl:when test="arxivid/text">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="arxivid/text" />
</xsl:when>
<xsl:when test="starts-with(url/text, 'http://arxiv.org/abs/') or starts-with(url/text, 'http://arxiv.org/pdf/')">
<xsl:text>arXiv</xsl:text>
<xsl:text>|eprint=</xsl:text><xsl:value-of select="substring(url/text, 22)" />
</xsl:when>

<xsl:when test="@type='article'">
<xsl:text>journal</xsl:text>
<xsl:if test="title/text"><xsl:text>|title=</xsl:text><xsl:value-of select="title/text" /></xsl:if>
<xsl:if test="url/text"><xsl:text>|url=</xsl:text><xsl:value-of select="url/text" /></xsl:if>
<xsl:if test="year/text"><xsl:text>|year=</xsl:text><xsl:value-of select="year/text" /></xsl:if>
<xsl:if test="journal/text"><xsl:text>|journal=</xsl:text><xsl:value-of select="journal/text" /></xsl:if>
<xsl:if test="volume"><xsl:text>|volume=</xsl:text><xsl:value-of select="volume" /></xsl:if>
<xsl:if test="pages/text"><xsl:text>|pages=</xsl:text><xsl:value-of select="pages/text" /></xsl:if>
<xsl:if test="number"><xsl:text>|issue=</xsl:text><xsl:value-of select="number" /></xsl:if>
<xsl:if test="publisher/text"><xsl:text>|publisher=</xsl:text><xsl:value-of select="publisher/text" /></xsl:if>
<xsl:if test="month"><xsl:text>|month=</xsl:text><xsl:value-of select="month" /></xsl:if>
<xsl:if test="authors/person/lastname">
<xsl:for-each select="authors/person">
<xsl:text>|last</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="lastname" />
<xsl:text>|first</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="firstname" />
</xsl:for-each>
</xsl:if>
</xsl:when>

<xsl:when test="@type='book'">
<xsl:text>book</xsl:text>
<xsl:if test="title/text"><xsl:text>|title=</xsl:text><xsl:value-of select="title/text" /></xsl:if>
<xsl:if test="url/text"><xsl:text>|url=</xsl:text><xsl:value-of select="url/text" /></xsl:if>
<xsl:if test="year/text"><xsl:text>|year=</xsl:text><xsl:value-of select="year/text" /></xsl:if>
<xsl:if test="month"><xsl:text>|month=</xsl:text><xsl:value-of select="month" /></xsl:if>
<xsl:if test="edition"><xsl:text>|edition=</xsl:text><xsl:value-of select="edition" /></xsl:if>
<xsl:if test="publisher/text"><xsl:text>|publisher=</xsl:text><xsl:value-of select="publisher/text" /></xsl:if>
<xsl:if test="series/text"><xsl:text>|series=</xsl:text><xsl:value-of select="series/text" /></xsl:if>
<xsl:if test="volume"><xsl:text>|volume=</xsl:text><xsl:value-of select="volume" /></xsl:if>
<xsl:if test="isbn"><xsl:text>|isbn=</xsl:text><xsl:value-of select="isbn" /></xsl:if>
<xsl:if test="authors/person/lastname">
<xsl:for-each select="authors/person">
<xsl:text>|last</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="lastname" />
<xsl:text>|first</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="firstname" />
</xsl:for-each>
</xsl:if>
</xsl:when>

<xsl:when test="@type='inproceedings' or @type='incollection'">
<xsl:text>conference</xsl:text>
<xsl:if test="title/text"><xsl:text>|title=</xsl:text><xsl:value-of select="title/text" /></xsl:if>
<xsl:if test="booktitle/text"><xsl:text>|booktitle=</xsl:text><xsl:value-of select="booktitle/text" /></xsl:if>
<xsl:if test="url/text"><xsl:text>|conference=</xsl:text><xsl:value-of select="url/text" /></xsl:if>
<xsl:if test="year/text"><xsl:text>|year=</xsl:text><xsl:value-of select="year/text" /></xsl:if>
<xsl:if test="month"><xsl:text>|month=</xsl:text><xsl:value-of select="month" /></xsl:if>
<xsl:if test="volume"><xsl:text>|volume=</xsl:text><xsl:value-of select="volume" /></xsl:if>
<xsl:if test="authors/person/lastname">
<xsl:for-each select="authors/person">
<xsl:text>|last</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="lastname" />
<xsl:text>|first</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="firstname" />
</xsl:for-each>
</xsl:if>
</xsl:when>

<xsl:when test="@type='techreport'">
<xsl:text>techreport</xsl:text>
<xsl:if test="title/text"><xsl:text>|title=</xsl:text><xsl:value-of select="title/text" /></xsl:if>
<xsl:if test="url/text"><xsl:text>|url=</xsl:text><xsl:value-of select="url/text" /></xsl:if>
<xsl:if test="year/text"><xsl:text>|year=</xsl:text><xsl:value-of select="year/text" /></xsl:if>
<xsl:if test="month"><xsl:text>|month=</xsl:text><xsl:value-of select="month" /></xsl:if>
<xsl:if test="number"><xsl:text>|issue=</xsl:text><xsl:value-of select="number" /></xsl:if>
<xsl:if test="institution/text"><xsl:text>|institution=</xsl:text><xsl:value-of select="institution/text" /></xsl:if>
<xsl:if test="authors/person/lastname">
<xsl:for-each select="authors/person">
<xsl:text>|last</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="lastname" />
<xsl:text>|first</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="firstname" />
</xsl:for-each>
</xsl:if>
</xsl:when>

<xsl:when test="@type='phdthesis' or @type='mastersthesis'">
<xsl:text>thesis</xsl:text>
<xsl:choose>
<xsl:when test="@type='phdthesis'"><xsl:text>|type=Ph.D.</xsl:text></xsl:when>
<xsl:when test="@type='mastersthesis'"><xsl:text>|type=M.Sc.</xsl:text></xsl:when>
</xsl:choose>
<xsl:if test="title/text"><xsl:text>|title=</xsl:text><xsl:value-of select="title/text" /></xsl:if>
<xsl:if test="year/text"><xsl:text>|year=</xsl:text><xsl:value-of select="year/text" /></xsl:if>
<xsl:if test="school/text"><xsl:text>|publisher=</xsl:text><xsl:value-of select="school/text" /></xsl:if>
<xsl:if test="authors/person/lastname">
<xsl:for-each select="authors/person">
<xsl:text>|last</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="lastname" />
<xsl:text>|first</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="firstname" />
</xsl:for-each>
</xsl:if>
</xsl:when>

<xsl:when test="url/text">
<xsl:text>web</xsl:text>
<xsl:text>|url=</xsl:text><xsl:value-of select="url/text" />
<xsl:if test="title/text"><xsl:text>|title=</xsl:text><xsl:value-of select="title/text" /></xsl:if>
<xsl:if test="year/text"><xsl:text>|year=</xsl:text><xsl:value-of select="year/text" /></xsl:if>
<xsl:if test="month"><xsl:text>|month=</xsl:text><xsl:value-of select="month" /></xsl:if>
<xsl:if test="authors/person/lastname">
<xsl:for-each select="authors/person">
<xsl:text>|last</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="lastname" />
<xsl:text>|first</xsl:text><xsl:value-of select="position()"/><xsl:text>=</xsl:text><xsl:value-of select="firstname" />
</xsl:for-each>
</xsl:if>

</xsl:when>
<xsl:otherwise><!-- BEGIN unknown type of citation -->
<xsl:text>FIXME</xsl:text>
</xsl:otherwise><!-- END unknown type of citation -->
</xsl:choose>

<xsl:text>}}
</xsl:text>
</xsl:template>

<xsl:template match="bibliography">
<xsl:apply-templates />
</xsl:template>


</xsl:stylesheet>
