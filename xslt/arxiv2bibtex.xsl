<xsl:stylesheet version = '1.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform' xmlns:arxiv="http://arxiv.org/schemas/atom">
<xsl:output method="text" omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>
<xsl:strip-space elements="*"/>



<!-- START HERE -->
<xsl:template match="/">
<!-- process each entry -->
<xsl:apply-templates select="feed/entry" />
</xsl:template>



<!-- An entry is interpreted as a BibTeX @article -->
<xsl:template match="entry">
<xsl:text>@misc{</xsl:text><xsl:value-of select='substring(id,22,100)' />

<!-- process authors by merging all names with "and" -->
<xsl:text>,
    author = {</xsl:text>
<xsl:for-each select="author/name" >
<xsl:if test="position() > 1"><xsl:text> and </xsl:text></xsl:if>
<xsl:value-of select="."/>
</xsl:for-each>
<xsl:text>}</xsl:text>

<xsl:apply-templates select="title" />
<xsl:apply-templates select="updated" />
<xsl:apply-templates select="summary" />
<xsl:apply-templates select="link" />
<xsl:apply-templates select="arxiv:doi" />
<xsl:apply-templates select="arxiv:journal_ref" />
<xsl:text>,
    comment = { published = </xsl:text>
<xsl:value-of select="published" />
<xsl:text>, updated = </xsl:text>
<xsl:value-of select="published" />
<xsl:text>, </xsl:text>
<xsl:value-of select="arxiv:comment" />
<xsl:text> }
}

</xsl:text>
</xsl:template>




<xsl:template match="title">
<xsl:text>,
    title = {{</xsl:text><xsl:value-of select="." /><xsl:text>}}</xsl:text>
</xsl:template>

<xsl:template match="updated">
<xsl:text>,
    year = {</xsl:text><xsl:value-of select="substring(.,1,4)" /><xsl:text>},
    month = </xsl:text>
<xsl:choose>
<xsl:when test="substring(.,6,2) = '01'"><xsl:text>jan</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '02'"><xsl:text>feb</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '03'"><xsl:text>mar</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '04'"><xsl:text>apr</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '05'"><xsl:text>may</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '06'"><xsl:text>jun</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '07'"><xsl:text>jul</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '08'"><xsl:text>aug</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '09'"><xsl:text>sep</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '10'"><xsl:text>oct</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '11'"><xsl:text>nov</xsl:text></xsl:when>
<xsl:when test="substring(.,6,2) = '12'"><xsl:text>dec</xsl:text></xsl:when>
<xsl:otherwise>{}</xsl:otherwise></xsl:choose>
</xsl:template>

<xsl:template match="summary">
<xsl:text>,
    abstract = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>

<xsl:template match="link"><xsl:choose>
<xsl:when test="@title = 'doi'">
<!-- ignore for now -->
</xsl:when>
<xsl:otherwise>
<!-- FIXME use counter to cover multiple URLs -->
<xsl:text>,
    url = {</xsl:text><xsl:value-of select="@href" /><xsl:text>}</xsl:text>
</xsl:otherwise>
</xsl:choose>
</xsl:template>

<xsl:template match="arxiv:doi">
<xsl:text>,
    doi = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>

<xsl:template match="arxiv:journal_ref">
<!-- FIXME split journal_ref into journal name, volume, issue, year, ... -->
<xsl:text>,
    journal = {</xsl:text><xsl:value-of select="." /><xsl:text>}</xsl:text>
</xsl:template>



</xsl:stylesheet>
