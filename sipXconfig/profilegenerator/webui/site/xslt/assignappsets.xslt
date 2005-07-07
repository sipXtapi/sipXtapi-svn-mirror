<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:output method="html"/>

    <!-- begin processing -->
    <xsl:template match="/">
        <select name="applicationsetid">
            <option>Select Application Set to Assign</option>
            <xsl:apply-templates select="items/applicationset">
                <xsl:sort select="name"/>
            </xsl:apply-templates>
        </select>
    </xsl:template>

    <xsl:template match="applicationset">
        <option value="{id}">
            <xsl:value-of select="name"/>
        </option>
    </xsl:template>
</xsl:stylesheet>