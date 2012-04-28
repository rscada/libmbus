<?xml version="1.0" ?>
<xsl:stylesheet version="1.0"
   xmlns:xsl="http://www.w3.org/1999/XSL/Transform" >
   
<!--  Sample XSL to convert XML in HTML  -->

<xsl:template match ="/" >
    <html>
      <head>
        <title> MBus Data </title>
      </head>
      <body>
        <xsl:apply-templates />
      </body>
    </html>
</xsl:template>

<xsl:template match="MBusData" >
    
    <xsl:apply-templates select="SlaveInformation"/>
    
    <br />
    
    <table border="1" cellspacing="0" cellpadding="5">
        <tr bgcolor = "#e0e0e0" >
            <td>Unit</td>
            <td>Value</td>
            <td>Function</td>
        </tr>
<xsl:for-each select="DataRecord" >
        <tr>
            <td><xsl:value-of select="Unit" /></td>
            <td><xsl:value-of select="Value" /></td>
            <td><xsl:value-of select="Function" /></td>
        </tr>
</xsl:for-each>
     </table>
</xsl:template >

<xsl:template match="SlaveInformation" >
    <table border="1" cellspacing="0" cellpadding="5">
        <tr>
            <td bgcolor = "#e0e0e0" >Id</td>
            <td><xsl:value-of select="Id" /></td>
        </tr>
        <tr>
            <td bgcolor = "#e0e0e0" >Manufacturer</td>
            <td><xsl:value-of select="Manufacturer" /></td>
        </tr>
        <tr>
            <td bgcolor = "#e0e0e0" >Version</td>
            <td><xsl:value-of select="Version" /></td>
        </tr>
        <tr>
            <td bgcolor = "#e0e0e0" >Medium</td>
            <td><xsl:value-of select="Medium" /></td>
        </tr>
        <tr>
            <td bgcolor = "#e0e0e0" >AccessNumber</td>
            <td><xsl:value-of select="AccessNumber" /></td>
        </tr>
        <tr>
            <td bgcolor = "#e0e0e0" >Status</td>
            <td><xsl:value-of select="Status" /></td>
        </tr>
        <tr>
            <td bgcolor = "#e0e0e0" >Signature</td>
            <td><xsl:value-of select="Signature" /></td>
        </tr>
    </table>
</xsl:template >


</xsl:stylesheet >