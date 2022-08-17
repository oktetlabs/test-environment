<!-- Copyright (C) 2021-2022 OKTET Labs Ltd. All rights reserved. -->
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output method="xml" omit-xml-declaration="no" indent="yes"/>
  <xsl:param name="filters"/>

  <xsl:template match="backup">
      <xsl:copy>
        <xsl:apply-templates select="object"/>
        <xsl:variable name="self" select="." />
        <xsl:apply-templates mode="filter-instances"
                            select="document($filters)/filters/subtree">
          <xsl:with-param name="instances" select="instance" />
        </xsl:apply-templates>
        <!-- This is used to keep the correct margins -->
        <xsl:text>&#xA;&#xA;</xsl:text>
      </xsl:copy>
  </xsl:template>

  <xsl:template mode="filter-instances" match="subtree">
    <xsl:param name="instances" />
    <xsl:variable name="filter" select="normalize-space(.)" />
    <xsl:apply-templates select="$instances[starts-with(@oid, $filter)]" />
  </xsl:template>

  <xsl:template match="*">
    <!-- This is used to keep the correct margins -->
    <xsl:text>&#xA;&#xA;  </xsl:text>
    <xsl:copy-of select="."/>
  </xsl:template>

</xsl:stylesheet>
