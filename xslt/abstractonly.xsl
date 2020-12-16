<xsl:stylesheet version='2.0' xmlns:xsl='http://www.w3.org/1999/XSL/Transform'>

<!--
  - This Extensible Stylesheet Language Transformation file translates XML files
  - as generated by KBibTeX into nice HTML code.
  -
  - This file was written by Thomas Fischer <fischer@unix-ag.uni-kl.de>
  - It is released under the GNU Public License version 2 or later.
  -->

<xsl:output omit-xml-declaration="yes" indent="no" encoding="UTF-8"/>

<!-- ==============================================================================
     Maintain original HTML tags
-->
<xsl:template match="a|abbr|acronym|address|applet|b|big|blockquote|br|cite|code|del|dfn|div|em|hr|i|kbd|p|param|pre|q|quote|samp|script|span|small|strike|strong|sub|sup|tt|var|button|fieldset|form|input|label|legend|object|option|optgroup|select|caption|col|colgroup|table|tbody|td|tfoot|th|thead|tr|dl|dd|dt|ol|ul|li|img|quote|quotation" xmlns:html="http://www.w3.org/1999/XSL/some">
<xsl:copy copy-namespaces="no">
<xsl:copy-of copy-namespaces="no" select="@*" />
<xsl:apply-templates />
</xsl:copy>
</xsl:template>



<xsl:template match="abstract">
<xsl:apply-templates />
</xsl:template>

<xsl:template match="entry">
<xsl:apply-templates select="abstract" />
</xsl:template>

<xsl:template match="bibliography">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
<head>
<title>Bibliography</title>
<meta http-equiv="content-type" content="text/html; charset=utf-8" />
</head>
<body style="margin:0px; padding: 0px;">
<xsl:apply-templates />
</body>
</html>
</xsl:template>

</xsl:stylesheet>
