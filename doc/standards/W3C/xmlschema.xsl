<?xml version='1.0'?>
<!-- Wrapper for xmlspec.xsl for XML Schema part 1 -->
<!-- Henry S. Thompson $Id: xmlschema.xsl,v 1.1 2003/10/28 09:35:41 arybchik Exp $ -->
<!-- XSL Style sheet, DTD omitted -->
<!DOCTYPE xsl:stylesheet [
<!ENTITY sect   "&#xa7;">
<!ENTITY nbsp   "&#160;">
<!ENTITY cellfront '#bedce6'>
<!-- width for examples - - set small for variable size -->
<!ENTITY pwidth " width: 1px;"> <!-- 5.8 in works well for printing -->
<!-- width for mappings - - set small doesn't work -->
<!ENTITY twidth " width: 100&#37;;"> <!-- 5.8 in works well for printing -->
]>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:hfp="http://www.w3.org/2001/XMLSchema-hasFacetAndProperty" xmlns:xlink="http://www.w3.org/TR/WD-xlink" version="1.0" exclude-result-prefixes="hfp xlink xsd">

 <!-- highlight unprocessed tags
 <xsl:template match="*"><SPAN STYLE="color:GREEN"><xsl:attribute name="TITLE"><xsl:value-of select="name()"/></xsl:attribute><xsl:apply-templates/></SPAN></xsl:template> -->

 <xsl:import href="xmlspec.xsl"/>

 <xsl:strip-space elements="xsd:*"/>

  <!-- Get and stash schema and schema dump, if present.
       Two stages because we need to establish context for
       unparsed-entity-uri as the input document -->
 
 <xsl:variable name="sdf">
   <xsl:value-of select="/spec/@schemaDump"/>
 </xsl:variable>
 
 <xsl:variable name="sd">
   <xsl:value-of select="/spec/@schemaProper"/>
 </xsl:variable>
 
 <xsl:variable name="sde">
   <xsl:value-of select="/spec/@schemaExample"/>
 </xsl:variable>
 
 <xsl:variable name="ddx">
   <xsl:value-of select="/spec/@datatypeDoc"/>
 </xsl:variable>

 <xsl:variable name="schemaDump" select="document($sdf,/)/*"/>
 <xsl:variable name="schemaProper" select="document($sd,/)/*"/>
 <xsl:variable name="schemaExample" select="document($sde,/)/*"/>
 
 <xsl:variable name="datatypeURL">
  <xsl:value-of select="/spec/back/div1/blist/bibl[@id='ref-xsp2']/loc/@href"/>
 </xsl:variable>
 
 <xsl:variable name="otherURL">
  <xsl:choose>
  <xsl:when test="/spec/back/div1/blist/bibl[@id='ref-xsp2']">
  <xsl:value-of select="/spec/back/div1/blist/bibl[@id='ref-xsp2']/loc/@href"/>
  </xsl:when>
  <xsl:otherwise>
  <xsl:value-of select="/spec/back/div1/div2/blist/bibl[@id='structural-schemas']/loc/@href"/>
  </xsl:otherwise>
 </xsl:choose>
 </xsl:variable>
 
 <xsl:variable name="reprelts" select="/spec/body/div1[@id='components' or @id='datatype-components']/div2/div3/reprdef/reprelt|/spec/body/div1[@id='composition']/div2[@id='layer2']/div3/reprdef/reprelt|/spec/body/div1[@id='datatype-components']/div2/div3/div4/reprdef/reprelt"/>

 <xsl:variable name="cnotes" select="//constraintnote"/>
 <xsl:variable name="dcnotes" select="document($ddx,/)//constraintnote"/>
 
 <xsl:template match="stale">
  <p class="stale">
<xsl:text>[Unrestructured material elided]</xsl:text>
  </p><!-- [Begin unrestructured material:
  <div class="stale">
   <xsl:apply-templates/>
  </div>
  <p class="stale">
<xsl:text>End unrestructured material]</xsl:text>
  </p>-->
 </xsl:template>

<!-- Override - - always empty and single in xmlschema-1,
     section numbers after in ()s -->
<!-- Check print for underline, else bold -->
 <xsl:template match="specref" name="specref">
  <xsl:param name="ref"><xsl:value-of select="@ref"/></xsl:param>
		<a href="#{$ref}">
                <xsl:value-of select="id($ref)/head"/>
		<xsl:text> (&sect;</xsl:text>
                <xsl:for-each select="id($ref)">
		<xsl:choose>
		<xsl:when test="ancestor::back">
			<xsl:number level="multiple" count="inform-div1|div1|div2|div3|div4" format="A.1"/>
		</xsl:when>
		<xsl:when test="ancestor::body">
			<xsl:number level="multiple" count="div1|div2|div3|div4" format="1.1"/>
		</xsl:when>
                </xsl:choose>
                </xsl:for-each>
		<xsl:text>)</xsl:text>
		</a>
 </xsl:template>

<!-- Override to get our Definition style in and
     override (W3C CSS) stylesheet's PRE indent -->
	<xsl:template match="spec">
		<html>
		<head>
		<title>
		<xsl:value-of select="header/title"/>
		</title>
		<style type="text/css">
                   code { font-family: monospace; font-size: 100%}
                   span.propdef { font-weight: bold; font-family: monospace }
                   span.termdef {color: #850021}
                   a.termref:visited, a.termref:link {font-family: sans-serif;
                              font-style: normal;
                              color: black;
                              text-decoration: none } 
                   a.eltref:visited, a.eltref:link { font-family: sans-serif;
                              color: black; 
                              text-decoration: none }
                   a.propref:visited, a.xpropref:visited, a.propref:link, a.xpropref:link { color: black; text-decoration: none;
                                           font-family: sans-serif }
                   dl.props, dl.psvi {margin-bottom: .5em; margin-top: 0em}
                   div.localToc {margin-top: 0em; margin-left: 8ex}
                   div.toc1 {margin-left: 5ex}
                   div.toc2 {margin-left: 2ex}
                   div.tocLine{margin: 0em; text-indent: -6ex}
                   h3.withToc {margin-bottom: 0em}
                   div.constraintnote { margin-top: 1em }
                   div.constraint {
                      margin-left: 1em; }
                   <!-- constraint clause lists -->
                   div.constraintlist {
                      margin-left: 1em; margin-bottom: 0em
                   }
                   div.clnumber {
                      text-indent: -1em;
                      margin-top: 0em; margin-bottom: 0em }
                   <!-- schema component definitions -->
                   div.schemaComp { border: 4px double gray; 
                                     margin: 0em 1em; padding: 0em }
                   div.compHeader { margin: 4px;
                                    font-weight: bold }
                   span.schemaComp { color: #A52A2A }                  
                   div.compBody { 
                                  border-top-width: 4px;
                                  border-top-style: double;
                                  border-top-color: #d3d3d3;
                                  padding: 4px ; margin: 0em}
                   <!-- PSVI property definition -->
                   div.psviDef { border: 4px double gray; 
                                     margin: 1em 1em; padding: 0em }
                   div.psviHeader { margin: 4px;
                                    font-weight: bold }
                   span.psviDef { color: #A52A2A }                  
                   div.psviBody { border-top-width: 4px;
                                  border-top-style: double;
                                  border-top-color: #d3d3d3;
                                  padding: 4px ; margin: 0em}
                   <!-- element displays -->
                   div.reprdef { border: 4px double gray; 
                                     margin: 0em 1em; padding: 0em }
                   div.reprHeader { margin: 4px;
                                    font-weight: bold }
                   span.reprdef { color: #A52A2A }                  
                   div.reprBody, div.reprcomp, div.reprdep { 
                                  border-top-width: 4px;
                                  border-top-style: double;
                                  border-top-color: #d3d3d3;
                                  padding: 4px ; margin: 0em}
                   table.reprcomp { margin-bottom: -.5em}
                   p.element-syntax-1 { font-family: monospace;
                                        margin-top: 0em; margin-bottom: .5em }
                   p.element-syntax { font-family: monospace; 
                                  border-top-width: 1px;
                                  border-top-style: solid;
                                  border-top-color: #d3d3d3;
                                  padding: 4px ; margin: 0em}
                   <!-- examples with prose -->
                   div.exampleInner pre { margin-left: 1em; 
                                          margin-top: 0em; margin-bottom: 0em}
                   div.exampleOuter {border: 4px double gray; 
                                     margin: 0em; padding: 0em}                 
                   div.exampleInner { background-color: #d5dee3;
                                      border-top-width: 4px;
                                      border-top-style: double;
                                      border-top-color: #d3d3d3;
                                      border-bottom-width: 4px;
                                      border-bottom-style: double;
                                      border-bottom-color: #d3d3d3;
                                      padding: 4px; margin: 0em }
                   div.exampleWrapper { margin: 4px }
                   div.exampleHeader { font-weight: bold;
                                       margin: 4px}
                   <!-- particle restriction table -->
                   table.restricts { margin-top: 1em; margin-bottom: 1em; margin-left: -2em}
                   table.restricts th { margin-left: 0em }
                   table.ubc td, table.ubc th { font-size: smaller }
                   table.dtdemo th { text-align: center;
                                     background-color: #d5dee3}
                   table.dtdemo pre { margin-left: 0em;  margin-bottom: 0em}
                   table.dtdemo td {background-color: #bedce6}
                  <xsl:apply-templates select="." mode="css"/>
                </style>
		<link rel="stylesheet" type="text/css" href="http://www.w3.org/StyleSheets/TR/W3C-{@docStatus}"/>
		</head>
		<body>
                  <xsl:apply-templates/>
		</body>
		</html>
	</xsl:template>


	<!-- Override to get extra text -->
        <xsl:template match="termdef">
             <span class="termdef">
               <a name="{@id}"><xsl:text>[Definition:]&nbsp;&nbsp;</xsl:text></a>
               <xsl:apply-templates/>
             </span>
        </xsl:template>

        <!-- override to get header inside box -->
        <xsl:template match="scrap">
             <div class="scrap">
             <table cellpadding="5" border="1" bgcolor="#f5dcb3" width="100%">
                <tbody>
                  <tr align="left">
                    <td>
                      <strong>
                      <font color="red"><xsl:value-of select="head"/></font>
                      </strong>
                    </td>
                   </tr>
                   <tr><td><table border="0" bgcolor="#f5dcb3"><tbody>
	          <xsl:apply-templates select="prodgroup|prod"/>
                   </tbody></table></td></tr>
                </tbody>
              </table>
              </div>
        </xsl:template>

<!-- special treatment for special common case:  eg inside note -->
        <xsl:template match="note[@role='example' or child::eg]">
          <div class="exampleOuter">
           <div class="exampleHeader">Example</div>
           <xsl:if test="*[1][self::p]">
            <div class="exampleWrapper">
             <xsl:apply-templates select="*[1]"/>
            </div>
           </xsl:if>
           <div class="exampleInner">
            <xsl:apply-templates select="eg"/>
           </div>
           <xsl:if test="*[position()>1 and self::p]">
           <div class="exampleWrapper">
            <xsl:apply-templates select="*[position()>1 and self::p]"/>
           </div>
           </xsl:if>
          </div>
         </xsl:template>

        <xsl:template match="note/eg">        
         <xsl:call-template name="egbody"/>        
        </xsl:template>

        <xsl:template match="local|iiName|strong">
                <b><xsl:apply-templates/></b>
        </xsl:template>
<!-- **************************************************************** -->
<!-- Tables -->
<!-- **************************************************************** -->
<!-- 
     Tables

     just copy over the entire subtree, as is
     except that the children of TD, TH and CAPTION are possibly handled by
     other templates in this stylesheet
  -->
<xsl:template match="table|caption|thead|tfoot|tbody|colgroup|col|tr|th|td">
   <xsl:copy>
      <xsl:apply-templates select="* | @* | text()"/>
   </xsl:copy>
</xsl:template>

<xsl:template match="@*">
   <xsl:copy>
      <xsl:apply-templates/>
   </xsl:copy>
</xsl:template>

        <!-- We do this differently from James -->
        <xsl:template match="rhs/com">
             <i class="com"><xsl:apply-templates/></i>
        </xsl:template>

         <xsl:template match="pt">
           <i><xsl:apply-templates/></i>
         </xsl:template>


<!-- 'Choose' cases for XSDL spec  -->
        <xsl:template match="constraintnote">
             <div class="constraintnote">
               <a name="{@id}"></a>
               <b>
                 <xsl:choose>
                      <xsl:when test="@type=&quot;cos&quot;">
                          <xsl:text>Schema Component Constraint:  </xsl:text>
                      </xsl:when>
                  <xsl:when test="@type=&quot;src&quot;">
                          <xsl:text>Schema Representation Constraint:  </xsl:text>
                      </xsl:when>
                      <xsl:when test="@type=&quot;cvc&quot;">
                          <xsl:text>Validation Rule:  </xsl:text>
                      </xsl:when>
                      <xsl:when test="@type=&quot;sic&quot;">
                          <xsl:text>Schema Information Set Contribution:  </xsl:text>
                      </xsl:when>
                      <xsl:otherwise>Constraint:  </xsl:otherwise>
                 </xsl:choose>
               <xsl:value-of select="head"/>
               </b><br/>
               <div class="constraint"><xsl:apply-templates/></div>
               
             </div>
        </xsl:template>

        <!-- Override to avoid stripping prefix (Why does James do this?) -->
	<xsl:template match="issue">
		<xsl:call-template name="insertID"/>
		<blockquote>
			<b>Issue (<xsl:value-of select="@id"/>): </b>
			<xsl:apply-templates/>
		</blockquote>
	</xsl:template>

        <xsl:template match="constraintnote/head"/>
 
 <!-- Our own component definition -->
 
 <xsl:template match="compdef">
  <div class="schemaComp">
   <div class="compHeader">
    <span class="schemaComp">Schema&nbsp;Component</span><xsl:text>:&nbsp;</xsl:text><a href="#{@ref}"><xsl:value-of select="@name"/></a>
   </div>
   <div class="compBody">
    <xsl:apply-templates/>
   </div>
  </div>
 </xsl:template>
 
 <xsl:template match="proplist">
  <xsl:choose>
     <xsl:when test="@role='psvi'">
      <div class="psviDef">
   <div class="psviHeader">
    <span class="psviDef">PSVI Contributions
for</span><xsl:text>&nbsp;</xsl:text><xsl:value-of select="@item"/>&nbsp;information items
   </div>
   <div class="psviBody">
    <dl class="psvi">
       <xsl:apply-templates/>
      </dl>
   </div>
  </div>
     </xsl:when>
     <xsl:otherwise>
      <dl class="props">
       <xsl:apply-templates/>
      </dl>
     </xsl:otherwise>
    </xsl:choose>
  
 </xsl:template>
 
 <!-- we choose our own brackets -->
 
 <xsl:template match="proplist[@role='psvi']/propdef">
  <xsl:call-template name="propdef"/>
 </xsl:template>
 
 <xsl:template match="propdef">
  <xsl:call-template name="propdef">
   <xsl:with-param name="ob">{</xsl:with-param>
   <xsl:with-param name="cb">}</xsl:with-param>
  </xsl:call-template>
 </xsl:template>
 
 <xsl:template match="propref[@role='psvi']">
  <xsl:call-template name="propref"/>
 </xsl:template>
 
 <xsl:template match="xpropref[@role='psviAnon']">
  <xsl:call-template name="xpropref"/>
 </xsl:template>
 
 <xsl:template match="xpropref[@role='anon']">
  <xsl:call-template name="xpropref">
   <xsl:with-param name="ob">{</xsl:with-param>
   <xsl:with-param name="cb">}</xsl:with-param>
  </xsl:call-template>
 </xsl:template>
 
 <xsl:template match="propref">
  <xsl:call-template name="propref">
   <xsl:with-param name="ob">{</xsl:with-param>
   <xsl:with-param name="cb">}</xsl:with-param>
  </xsl:call-template>
 </xsl:template>

 <!-- Our own complex abstract data type to concrete syntax mapping display -->
 
 <xsl:template match="reprdef">
  <div class="reprdef">
   <div class="reprHeader">
    <span class="reprdef">XML&nbsp;Representation&nbsp;Summary</span><xsl:text>:&nbsp;</xsl:text><code><xsl:value-of select="reprelt[1]/@eltname"/></code>
               <xsl:text>&nbsp;Element Information Item</xsl:text>
   </div>
   <div class="reprBody">
    <xsl:apply-templates/>
   </div>
  </div>
 </xsl:template>
 
 <xsl:template match="reprelt">                  
  <xsl:variable name="name" select="@eltname"/>
  <xsl:variable name="class">
   <xsl:call-template name="chooseClass"/>
  </xsl:variable>
  <p class="{$class}">
   <xsl:choose>
     <xsl:when test="@type">
      <xsl:variable name="type" select="@type"/>
      <xsl:call-template name="explicitType">
       <xsl:with-param name="elt" select="@eltname"/>
       <xsl:with-param name="typeDef" select="$schemaDump/xsd:complexType[@name=$type]"/>
       <xsl:with-param name="anchor">
        <xsl:if test="@local"><xsl:value-of select="@local"/>::</xsl:if><xsl:value-of select="@eltname"/>
       </xsl:with-param>
      </xsl:call-template>
     </xsl:when>
     <xsl:when test="$name='example'">
      <!-- cheat like h*** -->
      <xsl:apply-templates select="$schemaExample/xsd:element[@name='example']"/>
     </xsl:when>
     <xsl:otherwise>
     <xsl:apply-templates select="$schemaDump/xsd:element[@name=$name]"/></xsl:otherwise>
    </xsl:choose></p> 
 </xsl:template>

 
 <xsl:template match="reprcomp">
  <xsl:apply-templates select="reprdep"/>
  <div class="reprcomp">
  <xsl:if test="propmap"><table class="reprcomp">
   <thead>
    <tr>
     <th><a><xsl:attribute name="href">#<xsl:value-of select="@ref"/></xsl:attribute><xsl:value-of select="@abstract"/></a><strong>&nbsp;Schema Component</strong></th>
    </tr>
   </thead>
   <tbody>
    <tr>
     <td>
      <table border="0" cellpadding="3">
       <thead>
        <tr>
         <th align="left">Property</th>
         <th align="left">Representation</th>
        </tr>
       </thead>
       <tbody valign="top">
        <xsl:for-each select="propmap">
         <tr valign="top">
          <td>
<a href="#{@name}" class="propref">{<xsl:value-of select="id(@name)/@name"/>}</a>
</td>
          <td>
           <xsl:apply-templates/>
          </td>
         </tr>
        </xsl:for-each>
       </tbody>
      </table>
     </td>
    </tr>
   </tbody>
  </table></xsl:if></div>
  
 </xsl:template>
 
 <xsl:template match="reprdep|reprdef/p[preceding-sibling::*]">
   <div class="reprdep">
    <xsl:apply-templates/>
   </div>  
 </xsl:template>

 <xsl:template match="xsd:element">
 <!--
 <xsl:message>matched: xsd:element, @name=<xsl:value-of select='@name'/>, calling explicitType</xsl:message>
 -->
 <xsl:call-template name="explicitType">
   <xsl:with-param name="elt" select="@name"/>
   <xsl:with-param name="typeDef" select="xsd:complexType"/>
   <xsl:with-param name="std" select="@type"/>
   <xsl:with-param name="anchor" select="@name"/>
  </xsl:call-template>
 </xsl:template>
 
 <xsl:template name="chooseClass">
  <xsl:choose>
    <xsl:when test="count(preceding-sibling::*)=0">element-syntax-1</xsl:when>
    <xsl:otherwise>element-syntax</xsl:otherwise>
   </xsl:choose>
 </xsl:template>
 
 <xsl:template name="explicitType">
  <xsl:param name="elt"/>
  <xsl:param name="typeDef"/>
  <xsl:param name="std"/>
  <xsl:param name="anchor"/>
  <xsl:variable name="model" select="$typeDef/xsd:complexContent/xsd:restriction/*[self::xsd:sequence or self::xsd:choice or self::xsd:group or self::xsd:all]"/>
  <!--
  <xsl:message>elt=<xsl:value-of select='$elt'/></xsl:message>
  <xsl:message>std=<xsl:value-of select='$std'/></xsl:message>
  <xsl:message>typeDef=<xsl:value-of select='$typeDef/@id'/></xsl:message>
  <xsl:message>anchor=<xsl:value-of select='$anchor'/></xsl:message>
  -->
   <a name="element-{$anchor}">
    <xsl:text>&lt;</xsl:text>
     <xsl:call-template name="eltname">
      <xsl:with-param name="name">
       <xsl:value-of select="$elt"/>
      </xsl:with-param>
     </xsl:call-template>    
   </a>   
     <xsl:apply-templates select="$typeDef/*/xsd:restriction/xsd:attribute"/>
   <xsl:apply-templates select="$typeDef/*/xsd:restriction/xsd:anyAttribute"/>
     <xsl:choose>
      <xsl:when test="$std">
       <!-- must be a simple type - - never happens? -->
       <xsl:variable name="localDef" select="$schemaProper/xsd:simpleType[@name=$std]"/>
       <xsl:text>&gt;</xsl:text>
<br/>
<em><xsl:text>&nbsp;&nbsp;Content: </xsl:text></em>
       <xsl:value-of select="$std"/>
       <xsl:text>&nbsp;:&nbsp;</xsl:text>
       <xsl:apply-templates select="$localDef"/>
       <br/>
       <xsl:text>&lt;/</xsl:text>
 <xsl:call-template name="eltname">
  <xsl:with-param name="name" select="$elt"/>
 </xsl:call-template>
<xsl:text>&gt;</xsl:text>
      </xsl:when>
      <xsl:when test="not($model)">
       <br/><xsl:text>/&gt;</xsl:text>
      </xsl:when>
      <xsl:otherwise>
       <xsl:apply-templates mode="top" select="$model"/>
       <xsl:text>&lt;/</xsl:text>
 <xsl:call-template name="eltname">
  <xsl:with-param name="name" select="$elt"/>
 </xsl:call-template>
<xsl:text>&gt;</xsl:text>
      </xsl:otherwise>
     </xsl:choose>
   </xsl:template>

 <xsl:template name="eltname">
  <xsl:param name="name"/>
  <xsl:choose>
   <xsl:when test="starts-with($name,'!')">
    <xsl:value-of select="substring($name,2)"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="$name"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:template>

<xsl:template match="xsd:group|xsd:choice|xsd:all|xsd:sequence" mode="top">
<xsl:text>&gt;</xsl:text>
<br/>
<em><xsl:text>&nbsp;&nbsp;Content: </xsl:text></em>
<xsl:apply-templates select="." mode="model"/>
<br/>
</xsl:template>

<xsl:template match="xsd:group|xsd:choice|xsd:all|xsd:sequence" mode="model">
 <xsl:variable name="needParen" select="not(parent::xsd:group) or @minOccurs!='1' or @maxOccurs!='1'"/>
 <xsl:call-template name="group"/>
<xsl:if test="$needParen"><xsl:text>(</xsl:text></xsl:if>
<xsl:apply-templates mode="model"/>
<xsl:if test="$needParen"><xsl:text>)</xsl:text></xsl:if>
<xsl:call-template name="repeat"/>
</xsl:template>


 <xsl:template match="xsd:element" mode="model">
  <xsl:call-template name="group"/>
  <xsl:choose>
   <xsl:when test="@name">
    <xsl:call-template name="eltref">
     <xsl:with-param name="name" select="@name"/>
     <xsl:with-param name="inside" select="ancestor::xsd:element/@name"/>
     <xsl:with-param name="inModel" select="1"/>
    </xsl:call-template>
   </xsl:when>
   <xsl:otherwise>
    <xsl:call-template name="eltref">
     <xsl:with-param name="name" select="substring-after(@ref,':')"/>
     <xsl:with-param name="inModel" select="1"/>
    </xsl:call-template>
   </xsl:otherwise>
  </xsl:choose>
<xsl:call-template name="repeat"/>
</xsl:template>

 <xsl:template match="xsd:any" mode="model">
  <xsl:call-template name="group"/>
  <xsl:choose>
   <xsl:when test="@namespace='##any'">
    <em>{any}</em>
   </xsl:when>
   <xsl:otherwise>
    <em>{any with namespace: <xsl:value-of select="@namespace"/>}</em>
   </xsl:otherwise>
  </xsl:choose><xsl:call-template name="repeat"/></xsl:template>
 
 <xsl:template name="eltref">
  <xsl:param name="name"/>
  <xsl:param name="inside"/>
  <xsl:param name="inModel"/>

  <xsl:variable name="ref">
   <xsl:choose>
    <xsl:when test="$inside and $reprelts/@local[.=$inside]"><xsl:value-of select="$inside"/>::<xsl:value-of select="$name"/></xsl:when>
    <xsl:otherwise><xsl:value-of select="$name"/></xsl:otherwise>
   </xsl:choose>
  </xsl:variable>
  <xsl:variable name="doc">
   <xsl:if test="not($inside or $name='schema' or $name='example' or $reprelts[@eltname=$name])">

    <xsl:value-of select="$otherURL"/>
   </xsl:if>
  </xsl:variable>
  <a href="{$doc}#element-{$ref}" class="eltref"><xsl:if test="not($inModel)">&lt;</xsl:if><xsl:value-of select="$name"/><xsl:if test="not($inModel)">></xsl:if></a>
 </xsl:template>
 
 <xsl:template match="eltref">
  <xsl:call-template name="eltref">
   <xsl:with-param name="name" select="@ref"/>
   <xsl:with-param name="inside" select="@inside"/>
  </xsl:call-template>
 </xsl:template>

<xsl:template name="group">
<xsl:if test="position()>1">
<xsl:choose>
<xsl:when test="parent::xsd:sequence"><xsl:text>, </xsl:text></xsl:when>
<xsl:when test="parent::xsd:choice"><xsl:text> | </xsl:text></xsl:when>
<xsl:when test="parent::xsd:all"><xsl:text> &amp; </xsl:text></xsl:when>
</xsl:choose>
</xsl:if>
</xsl:template>

<xsl:template name="repeat">
  <xsl:choose>
   <xsl:when test="@minOccurs='1' and @maxOccurs='unbounded'">
    <xsl:text>+</xsl:text>
   </xsl:when>
   <xsl:when test="@minOccurs='0' and @maxOccurs='unbounded'">
    <xsl:text>*</xsl:text>
   </xsl:when>
   <xsl:when test="@minOccurs='0'">
    <xsl:text>?</xsl:text>
   </xsl:when>
  </xsl:choose>
</xsl:template>

<xsl:template match="xsd:attribute[@use='prohibited']"/>
<xsl:template match="xsd:attribute[not(@use='prohibited')]">
 <xsl:variable name="aname">
  <xsl:choose>
   <xsl:when test="@name">
    <xsl:value-of select="@name"/>
   </xsl:when>
   <xsl:otherwise><xsl:value-of select="@ref"/></xsl:otherwise>
  </xsl:choose>
 </xsl:variable>
<br/>
<xsl:text>&nbsp;&nbsp;</xsl:text>
<xsl:choose>
<xsl:when test="@use='required'">
<b><xsl:value-of select="$aname"/></b>
</xsl:when>
<xsl:otherwise>
<xsl:value-of select="$aname"/>
</xsl:otherwise>
</xsl:choose>
<xsl:text> = </xsl:text>
	<xsl:choose>
		<xsl:when test="@type">
			<xsl:variable name="type" select="@type"/>
			<xsl:variable name="localDef" select="$schemaProper/xsd:simpleType[@name=$type]"/>
			<xsl:choose>
			 <xsl:when test="$localDef/xsd:restriction/xsd:enumeration or $localDef/xsd:union">
				<xsl:apply-templates select="$localDef"/></xsl:when>
		         <xsl:when test="$aname='public'">
			  <a href="{$datatypeURL}#anyURI">anyURI</a>
			</xsl:when>
			<xsl:otherwise>
				<a href="{$datatypeURL}#{@type}"><xsl:value-of select="$type"/></a></xsl:otherwise>
			</xsl:choose>
		</xsl:when>
		<xsl:when test="$aname='xml:lang'">
			<a href="{$datatypeURL}#language">language</a>
		</xsl:when>
		<xsl:when test="$aname='xpath'">
			<em>a subset of XPath expression, see below</em>
		</xsl:when>
		<xsl:when test="not(xsd:simpleType)">
			<a href="{$datatypeURL}#dt-anySimpleType">anySimpleType</a>
		</xsl:when>
 	</xsl:choose>
<xsl:apply-templates/>
 <xsl:if test="@use='default' or @use='fixed'">
  <xsl:text>&nbsp;:&nbsp;</xsl:text>
  <xsl:choose>
   <xsl:when test="@value=&quot;&quot;">
    <xsl:text>''</xsl:text>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="@value"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:if>
</xsl:template>
 
 
<xsl:template match="xsd:anyAttribute">
<br/>

<xsl:text>&nbsp;&nbsp;</xsl:text>
 <em>{any attributes<xsl:choose>
<xsl:when test="@namespace='##any'"></xsl:when>
                    <xsl:when test="@namespace='##other'"><!-- cheat for schemas -->
<xsl:text> with non-schema namespace</xsl:text>
                    </xsl:when>
                    <xsl:otherwise><xsl:text> with namespace: </xsl:text><xsl:value-of select="@namespace"/></xsl:otherwise>
</xsl:choose><xsl:text> . . .}</xsl:text></em>
</xsl:template>
 
 <xsl:template match="xsd:simpleType[xsd:restriction/xsd:enumeration]">
 <!--<xsl:message>xsd:st[rest/enum]</xsl:message>-->
  <xsl:if test="xsd:restriction/xsd:enumeration[2]">(</xsl:if>
  <xsl:apply-templates select="xsd:restriction/xsd:enumeration"/>
  <xsl:if test="xsd:restriction/xsd:enumeration[2]">)</xsl:if>
 </xsl:template>
 
 <xsl:template match="xsd:simpleType[xsd:list]">
 <!--<xsl:message>xsd:st[list]</xsl:message>-->
  <xsl:apply-templates select="xsd:list"/>
 </xsl:template>
 
 <xsl:template match="xsd:list[@itemType]">
 <xsl:variable name="type">
  <xsl:choose>
   <xsl:when test="starts-with(@itemType,'xs:')">
    <xsl:value-of select="substring-after(@itemType,':')"/>
   </xsl:when>
   <xsl:otherwise>
    <xsl:value-of select="@itemType"/>
   </xsl:otherwise>
  </xsl:choose>
 </xsl:variable>
 <xsl:variable name="localDef" select="$schemaProper/xsd:simpleType[@name=$type]"/>
  <xsl:text>List of </xsl:text><xsl:choose>
    <xsl:when test="$localDef/xsd:restriction/xsd:enumeration or $localDef/xsd:union"><xsl:apply-templates select="$localDef"/></xsl:when>
    <xsl:otherwise><a href="{$datatypeURL}#{$type}"><xsl:value-of select="$type"/></a></xsl:otherwise>
   </xsl:choose>
 </xsl:template>
 
 <xsl:template match="xsd:list[xsd:simpleType]">
  <xsl:text>List of </xsl:text>
  <xsl:apply-templates select="xsd:simpleType"/>
 </xsl:template>

 <xsl:template match="xsd:simpleType[xsd:union]">
 <!--<xsl:message>xsd:st[union]</xsl:message>-->
   (<xsl:apply-templates select="xsd:union"/>)
 </xsl:template>
 
 <xsl:template match="xsd:union">
	<xsl:if test="@memberTypes">
		<xsl:variable name="type" select="substring-after(@memberTypes,':')"/>
		<xsl:variable name="localDef" select="$schemaProper/xsd:simpleType[@name=$type]"/>
		<xsl:choose>
					<xsl:when test="$localDef/xsd:restriction/xsd:enumeration or $localDef/xsd:union">
						<xsl:apply-templates select="$localDef"/>
					</xsl:when>
					<xsl:otherwise>
						<a href="{$datatypeURL}#{$type}"><xsl:value-of select="$type"/></a>
					</xsl:otherwise>
				</xsl:choose>
	</xsl:if>
	<xsl:if test="xsd:simpleType and @memberTypes">
		<xsl:text> | </xsl:text>
	</xsl:if>
	<xsl:apply-templates select="xsd:simpleType[1]"/>
	<xsl:for-each select="xsd:simpleType[position()>1]">
		<xsl:text> | </xsl:text><xsl:apply-templates select="."/>
	</xsl:for-each>
 </xsl:template>

 <xsl:template match="xsd:simpleType[xsd:annotation/xsd:documentation and not(xsd:restriction/xsd:enumeration)]" priority="0.1">
 <!--<xsl:message>xsd:st[ann/doc and no rest/enum]</xsl:message>-->
  <!-- This is a hack to cope with utility types with no real definition -->
  <em><xsl:value-of select="xsd:annotation/xsd:documentation[1]"/></em>
 </xsl:template>

<xsl:template match="xsd:enumeration">
<xsl:if test="position()>1"> | </xsl:if>
<var><xsl:value-of select="@value"/></var>
</xsl:template>

<xsl:template match="constant">
<xsl:if test="position()>1"> | </xsl:if>
<xsl:text>"</xsl:text>
<xsl:value-of select="@value"/>
<xsl:text>"</xsl:text>
</xsl:template>

 <xsl:template match="schemaComp">
  <table class="schemaComp" border="1">
   <thead>
    <tr>
     <th><strong><xsl:choose><xsl:when test="@id"><a name="{@id}"><xsl:value-of select="head"/></a></xsl:when><xsl:otherwise><xsl:value-of select="head"/></xsl:otherwise></xsl:choose></strong></th>
    </tr>
   </thead>
   <tbody>
    <tr>
     <td>
      <xsl:apply-templates select="pvlist"/>
     </td>
    </tr>
   </tbody>
  </table>
 </xsl:template>
 
 <xsl:template match="pvlist">
    <table border="0" cellpadding="3">
       <thead>
        <tr>
         <th align="left">Property</th>
         <th align="left">Value</th>
        </tr>
       </thead>
       <tbody valign="top">
        <xsl:apply-templates select="pvpair"/>
       </tbody>
      </table>
 </xsl:template>
 
 <xsl:template match="pvpair">
  <tr>
   <td><xsl:call-template name="propref">
   <xsl:with-param name="ob">{</xsl:with-param>
   <xsl:with-param name="cb">}</xsl:with-param>
       </xsl:call-template></td>
   <td>
    <xsl:apply-templates/>
   </td>
  </tr>
 </xsl:template>

<xsl:template match="restrictCases">
  <table class="restricts" border="2" cellpadding="3">
   <thead>
    <tr>
     <th colspan="2"/>
     <th colspan="6"><strong>Base Particle</strong></th>
    </tr>
   </thead>
   <tbody>
    <tr>
     <th colspan="2"/>
     <th>elt</th>
     <th>any</th>
     <th>all</th>
     <th>choice</th>
     <th>sequence</th>
    </tr>
    <tr>
     <th rowspan="5">Derived Particle</th>
     <xsl:apply-templates select="restrict[1]"/>
    </tr>
    <xsl:for-each select="restrict[position()>1]">
     <tr><xsl:apply-templates select="."/></tr>
    </xsl:for-each>
   </tbody>
  </table>
 </xsl:template>
 
 <xsl:template match="restrict">
   <th><xsl:value-of select="@case"/></th>
   <td>
    <xsl:call-template name="rcell">
     <xsl:with-param name="val" select="elt"/>
    </xsl:call-template>
   </td>
   <td>
    <xsl:call-template name="rcell">
     <xsl:with-param name="val" select="any"/>
    </xsl:call-template>
   </td>
   <td>
    <xsl:call-template name="rcell">
     <xsl:with-param name="val" select="all"/>
    </xsl:call-template>
   </td>
   <td>
    <xsl:call-template name="rcell">
     <xsl:with-param name="val" select="choice"/>
    </xsl:call-template>
   </td>
   <td>
    <xsl:call-template name="rcell">
     <xsl:with-param name="val" select="seq"/>
    </xsl:call-template>
   </td>
 </xsl:template>
 
 <xsl:template name="rcell">
  <xsl:param name="val"/>
  <xsl:variable name="vref">
   <xsl:choose>
   <xsl:when test="$val/@ref">
    <xsl:value-of select="$val/@ref"/>
   </xsl:when>
   <xsl:otherwise><xsl:value-of select="$val"/></xsl:otherwise>
   </xsl:choose>  
  </xsl:variable>
  <xsl:choose>
   <xsl:when test="$vref='Forbidden'">
    <xsl:text>Forbidden</xsl:text>
   </xsl:when>
   <xsl:otherwise><a href="#rcase-{$vref}"><xsl:value-of select="$val"/></a></xsl:otherwise>
  </xsl:choose>  
 </xsl:template>
<!--* CMSMcQ:  see if I can work on olist here ...
    *-->
<xsl:template match="constraintnote//olist|propmap//olist|olist[.//item[@id]]">
 <xsl:choose>
  <xsl:when test="@role='and'"><b>all</b> of the following must be true:</xsl:when>
  <xsl:when test="@role='andval'"><b>all</b> of the following:</xsl:when>
  <xsl:when test="@role='andtest'"><b>all</b> of the following are true</xsl:when>
  <xsl:when test="@role='And'"><b>All</b> of the following must be true:</xsl:when>
  <xsl:when test="@role='or'"><b>one</b> of the following must be true:</xsl:when>
  <xsl:when test="@role='orval'"><b>one</b> of the following:</xsl:when>
  <xsl:when test="@role='ortest'"><b>one</b> of the following is true</xsl:when>
  <xsl:when test="@role='Or'"><b>One</b> of the following must be true:</xsl:when>
  <xsl:when test="@role='case'">the appropriate <b>case</b> among the following
must be true:</xsl:when>
  <xsl:when test="@role='Case'">The appropriate <b>case</b> among the following
must be true:</xsl:when>
  <xsl:when test="@role='caseval'">the appropriate <b>case</b> among the following:</xsl:when>
  <xsl:when test="@role='Caseval'">The appropriate <b>case</b> among the following:</xsl:when>
 </xsl:choose>
 <div class="constraintlist">
  <xsl:apply-templates/>
 </div>
</xsl:template>

<xsl:template match="constraintnote//olist/item|propmap//olist/item|olist[item[@id]]/item">
 <div class="clnumber">
  <xsl:number level="multiple" count="item" from="constraintnote|propmap|div3|div2" format="1"/>
  <xsl:choose>
        <xsl:when test="@id">
         <a name="{@id}"><xsl:text> </xsl:text></a>
        </xsl:when>
        <xsl:otherwise><xsl:text> </xsl:text></xsl:otherwise>
       </xsl:choose>
  <xsl:apply-templates/>
 </div>
</xsl:template>

 
 <xsl:template match="olist[@role='case' or @role='Case' or @role='caseval' or @role='Caseval']/item">
  <div class="clnumber">
  <xsl:number level="multiple" count="item" from="constraintnote|propmap" format="1"/>
       <xsl:choose>
        <xsl:when test="@id">
         <a name="{@id}"><xsl:text> </xsl:text></a>
        </xsl:when>
        <xsl:otherwise><xsl:text> </xsl:text></xsl:otherwise>
       </xsl:choose>
       
  <xsl:choose>
   <xsl:when test="p[@role='if']"><b>If </b><xsl:for-each select="p[@role='if']">
<xsl:apply-templates/>
</xsl:for-each>, <b>then </b><xsl:for-each select="p[@role='then']">
<xsl:apply-templates/>
</xsl:for-each></xsl:when>
   <xsl:otherwise><b>otherwise </b><xsl:for-each select="p[@role='otherwise']">
<xsl:apply-templates/>
</xsl:for-each>
   </xsl:otherwise>
  </xsl:choose>
 </div> 
 </xsl:template>
 
 <xsl:template match="clauseref">
  <xsl:text>clause </xsl:text>
  <a href="#{@ref}"><xsl:for-each select="id(@ref)"><xsl:number level="multiple" count="item" from="constraintnote|propmap|div3|div2" format="1"/></xsl:for-each></a>
 </xsl:template>
 
 <!-- hack to tabulate outcomes -->
 <xsl:template match="div2[@id='outcome-src']">
  <xsl:apply-templates/>
  <dl>   
  <xsl:for-each select="$cnotes[@type='src']|$dcnotes[@type='src']">
   <xsl:sort select="@id"/>
   <xsl:variable name="id" select="@id"/>
   <dt><xsl:value-of select="@id"/></dt>
   <dd><a>
     <xsl:attribute name="href">
      <xsl:if test="$dcnotes[@id=$id]"><xsl:value-of select="$datatypeURL"/></xsl:if>#<xsl:value-of select="@id"/></xsl:attribute>
     <xsl:value-of select="head"/></a></dd>
  </xsl:for-each>
  </dl>
 </xsl:template>
 
 <xsl:template match="div2[@id='outcome-cos']">
  <xsl:apply-templates/>
  <dl>   
  <xsl:for-each select="$cnotes[@type='cos']|$dcnotes[@type='cos']">
   <xsl:sort select="@id"/>
   <xsl:variable name="id" select="@id"/>
   <dt><xsl:value-of select="@id"/></dt>
   <dd><a>
     <xsl:attribute name="href">
      <xsl:if test="$dcnotes[@id=$id]"><xsl:value-of select="$datatypeURL"/></xsl:if>#<xsl:value-of select="@id"/></xsl:attribute>
     <xsl:value-of select="head"/></a></dd>
  </xsl:for-each>
  </dl>
 </xsl:template>

 <xsl:template match="div2[@id='validation_failures']">
  <xsl:apply-templates/>
  <dl>   
  <xsl:for-each select="$cnotes[@type='cvc']|$dcnotes[@type='cvc']">
   <xsl:sort select="@id"/>
   <xsl:variable name="id" select="@id"/>
   <dt><xsl:value-of select="@id"/></dt>
   <dd><a>
     <xsl:attribute name="href">
      <xsl:if test="$dcnotes[@id=$id]"><xsl:value-of select="$datatypeURL"/></xsl:if>#<xsl:value-of select="@id"/></xsl:attribute>
     <xsl:value-of select="head"/></a></dd>
  </xsl:for-each>   
  </dl>
 </xsl:template>
 
 <xsl:template match="div2[@id='PSVI_contributions']">
  <xsl:apply-templates/>
  <xsl:variable name="ppl" select="$cnotes[@type='sic']//proplist[@role='psvi']"/>
  <dl>
   <xsl:for-each select="$ppl">
    <xsl:sort select="@item"/>
    <xsl:variable name="ci" select="@item"/>
    <xsl:if test="not(preceding::*[@item=$ci])">
     <dt><xsl:value-of select="$ci"/> information item properties</dt>
     <dd>
      <xsl:for-each select="$ppl[@item=$ci]/propdef">
       <xsl:sort select="@name"/>
       <xsl:variable name="ce" select="ancestor::constraintnote"/>
       <a href="#{@id}">[<xsl:value-of select="@name"/>]</a>
       (<a href="#{$ce/@id}"><xsl:value-of select="$ce/head"/></a>)
       <br/>
      </xsl:for-each>
     </dd>
    </xsl:if>
   </xsl:for-each>
  </dl>
 </xsl:template>
 
 <xsl:template match="ednote[@role='pf']">
		<blockquote>
		<p><b>Ed. Note: </b><SPAN STYLE="color: red">Priority Feedback Request</SPAN><br/><xsl:apply-templates/></p>
		</blockquote>
	</xsl:template>
 
  <!-- hack to tabulate termdefs -->
 <xsl:template match="ednote[@role='glossary']">
  <dl>   
  <xsl:for-each select="//termdef[not(@role='local')]">
   <xsl:sort select="@term"/>
   <dt>
<a href="#{@id}"><xsl:value-of select="@term"/></a>
</dt>
   <dd>
<xsl:apply-templates/>
</dd>
  </xsl:for-each>
  </dl>
 </xsl:template>
 
 
<!-- pvb -->
   <xsl:variable name="datatypes" select="$schemaProper/xsd:simpleType[not(contains(.//xsd:documentation,'not for public'))]"/>

	<xsl:template match="dtref">
		<a href="#{@ref}">
			<xsl:value-of select="@ref"/>
		</a>
	</xsl:template>

	<xsl:template match="baseref">
		<xsl:variable name="type" select="ancestor::div3/head"/>
		<xsl:variable name="base" select="substring-after($datatypes[@name=$type]/xsd:restriction/@base,&quot;:&quot;)"/>
		<a href="#{$base}"><xsl:value-of select="$base"/></a>
	</xsl:template>
	
	<xsl:template match="itemTyperef">
		<xsl:variable name="type" select="ancestor::div3/head"/>
		<xsl:variable name="itemType" select="substring-after($schemaProper//xsd:simpleType[@name=$type]/descendant::xsd:list/@itemType,&quot;:&quot;)"/>
		<a href="#{$itemType}"><xsl:value-of select="$itemType"/></a>
	</xsl:template>
 <xsl:template match="nt">
  <xsl:variable name="def" select="id(@def)"/>
  <a href="#{@def}">
   <!--diagnose difference between datatypes and structures-->
   <xsl:choose>
    <xsl:when test="$def/self::lhs"><xsl:value-of select="$def"/></xsl:when>
    <xsl:otherwise><xsl:apply-templates/></xsl:otherwise>
   </xsl:choose>
   </a>
	</xsl:template>


<!--
     the following is for use in building the "tableau" of XML Reps from
	 the "dump" file
	<xsl:template match="xsd:simpleType[xsd:restriction]" priority='1.0'>
		<a href='#{@name}'><xsl:value-of select='@name'/></a>
	</xsl:template>
  -->

	<xsl:template match="applicable-facets">
		<table border="1" bgcolor="&cellfront;">
			<tr>
			<th><a href="#defn-basetype">{base type definition}</a></th>
			<th>applicable <a href="#defn-facets">{facets}</a></th>
		</tr>
		<tr>
			<th colspan="2" align="center">
				If <a href="#defn-variety">{variety}</a> is <a href="#dt-list">list</a>,
				then
			</th>
		</tr>
		<tr>
			<td>[all datatypes]</td>
			<td><a href="#dt-length">length</a>,
				<a href="#dt-minLength">minLength</a>,
				<a href="#dt-maxLength">maxLength</a>,
				<a href="#dt-pattern">pattern</a>,
				<a href="#dt-enumeration">enumeration</a>,
				<a href="#dt-whiteSpace">whiteSpace</a>
			</td>
		</tr>
		<tr>
			<th colspan="2" align="center">
				If <a href="#defn-variety">{variety}</a> is
				<a href="#dt-union">union</a>, then
			</th>
		</tr>
		<tr>
			<td>[all datatypes]</td>
			<td><a href="#dt-pattern">pattern</a>,
				<a href="#dt-enumeration">enumeration</a>
			</td>
		</tr>

		<tr>
			<th colspan="2" align="center">
				else if <a href="#defn-variety">{variety}</a> is
				<a href="#dt-atomic">atomic</a>, then
			</th>
		</tr>
		<xsl:apply-templates select="$datatypes[xsd:restriction[substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;]][position()=1]" mode="first-appl"/>
		<xsl:apply-templates select="$datatypes[xsd:restriction[substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;]][position()>1]" mode="rest-appl"/>
	<!--<xsl:message>primitives done</xsl:message>-->
	</table>
	<!--<xsl:message>applicable-facets exited</xsl:message>-->
	</xsl:template>

	<xsl:template match="xsd:simpleType" mode="first-appl">
		<tr>
			<td><a href="#{@name}"><xsl:value-of select="@name"/></a></td>
			<td>
				<xsl:call-template name="applicable-facets">
					<xsl:with-param name="name" select="@name"/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="derivedBy" select="@derivedBy"/>
				</xsl:call-template>
			</td>
		</tr>
	</xsl:template>

	<xsl:template match="xsd:simpleType" mode="rest-appl">
		<tr>
			<td><a href="#{@name}"><xsl:value-of select="@name"/></a></td>
			<td>
				<xsl:choose>
					<xsl:when test="xsd:restriction">
						<xsl:call-template name="applicable-facets">
							<xsl:with-param name="name" select="@name"/>
							<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
							<xsl:with-param name="derivedBy">restriction</xsl:with-param>
						</xsl:call-template>
					</xsl:when>
					<xsl:when test="xsd:list">
						<xsl:call-template name="applicable-facets">
							<xsl:with-param name="name" select="@name"/>
							<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
							<xsl:with-param name="derivedBy">list</xsl:with-param>
						</xsl:call-template>
					</xsl:when>						
				</xsl:choose>
			</td>
		</tr>
	<!--<xsl:message>simpleType m='ra' exited w/ <xsl:value-of select='@name'/></xsl:message>-->
	</xsl:template>

	<xsl:template name="applicable-facets">
		<xsl:param name="name"/>
		<xsl:param name="base"/>
		<xsl:param name="derivedBy"/>
	<!--<xsl:message>af named temp called w/ name=<xsl:value-of select='$name'/>,
	base=<xsl:value-of select='$base'/> and db=<xsl:value-of select='$derivedBy'/></xsl:message>-->
		<xsl:choose>
			<xsl:when test="$base=&quot;anySimpleType&quot; or $derivedBy=&quot;list&quot;">
				<xsl:for-each select="$datatypes[@name=$name]/xsd:annotation/xsd:appinfo/hfp:hasFacet">
					<a href="#dc-{@name}">
						<xsl:value-of select="@name"/>
					</a>
					<xsl:if test="position()!=last()">
						<xsl:text>, </xsl:text>
					</xsl:if>
				</xsl:for-each>
			</xsl:when>
			<xsl:when test="$datatypes[@name=$base]//xsd:restriction">
				<xsl:call-template name="applicable-facets">
					<xsl:with-param name="name" select="$datatypes[@name=$base]/@name"/>
					<xsl:with-param name="base" select="$datatypes[@name=$base]//xsd:restriction/@base"/>
					<xsl:with-param name="derivedBy">restriction</xsl:with-param>
				</xsl:call-template>
			</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="applicable-facets">
					<xsl:with-param name="name" select="$datatypes[@name=$base]/@name"/>
					<xsl:with-param name="base" select="$datatypes[@name=$base]//xsd:restriction/@base"/>
					<xsl:with-param name="derivedBy">list</xsl:with-param>
				</xsl:call-template>
			</xsl:otherwise>
		</xsl:choose>
	<!--<xsl:message>af named temp exiting w/ <xsl:value-of select='$name'/></xsl:message>-->
	</xsl:template>

	<xsl:template match="fundamental-facets">
	<!--<xsl:message>fundamental-facets called</xsl:message>-->
		<table border="1" bgcolor="&cellfront;">
			<tbody>
				<tr>
					<th>&nbsp;</th>
					<th>Datatype</th>
					<th><a href="#dt-ordered">ordered</a></th>
					<th><a href="#dt-bounded">bounded</a></th>
					<th><a href="#dt-cardinality">cardinality</a></th>
					<th><a href="#dt-numeric">numeric</a></th>
				</tr>
				<xsl:apply-templates select="$datatypes[xsd:restriction[substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;] or xsd:list[substring-after(@type,&quot;:&quot;)=&quot;anySimpleType&quot;]][position()=1]" mode="first-fund"/>
				<xsl:apply-templates select="$datatypes[xsd:restriction[substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;] or xsd:list[substring-after(@type,&quot;:&quot;)=&quot;anySimpleType&quot;]][position()>1]" mode="rest-fund"/>
	<!--<xsl:message>primitives done</xsl:message>-->
				<tr>
					<td colspan="7"/>
				</tr>
				<xsl:apply-templates select="$datatypes[xsd:restriction[not(substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;)] or xsd:list[not(substring-after(@itemType,&quot;:&quot;)=&quot;anySimpleType&quot;)]][position()=1]" mode="first-fund"/>
	<!--<xsl:message>1st derived done</xsl:message>-->
				<xsl:apply-templates select="$datatypes[xsd:restriction[not(substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;)] or xsd:list[not(substring-after(@itemType,&quot;:&quot;)=&quot;anySimpleType&quot;)]][position()>1]" mode="rest-fund"/>
	<!--<xsl:message>derived done</xsl:message>-->
			</tbody>
		</table>
	<!--<xsl:message>fundamental-facets exited</xsl:message>-->
	</xsl:template>

	<xsl:variable name="numPrimitive"><xsl:value-of select="count($datatypes/xsd:restriction[substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;])"/></xsl:variable>
	<xsl:variable name="numDerived"><xsl:value-of select="count($datatypes[xsd:restriction[not(substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;)] or xsd:list[not(substring-after(@base,&quot;:&quot;)=&quot;anySimpleType&quot;)]])"/></xsl:variable>

	<xsl:template match="xsd:simpleType" mode="first-fund">
	<!--<xsl:message>first fund called <xsl:value-of select='@name'/>, <xsl:value-of select='xsd:restriction/@base'/></xsl:message>-->
			<tr>
				<xsl:choose>
					<xsl:when test="substring-after(xsd:restriction/@base,&quot;:&quot;)=&quot;anySimpleType&quot;">
						<td>
							<xsl:attribute name="rowspan">
								<xsl:value-of select="$numPrimitive"/>
							</xsl:attribute>
							<a href="#dt-primitive">primitive</a>
						</td>
					</xsl:when>
					<xsl:otherwise>
						<td>
							<xsl:attribute name="rowspan">
								<xsl:value-of select="$numDerived"/>
							</xsl:attribute>
							<a href="#dt-derived">derived</a>
						</td>
					</xsl:otherwise>
				</xsl:choose>
				<td><a href="#{@name}"><xsl:value-of select="@name"/></a></td>
	<!--<xsl:message>fund-fact: recursing on <xsl:value-of select='substring-after(xsd:restriction/@base,":")'/> for order</xsl:message>-->
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">ordered</xsl:with-param>
				</xsl:call-template>
	<!--<xsl:message>fund-fact: recursing on <xsl:value-of select='substring-after(xsd:restriction/@base,":")'/> for bound</xsl:message>-->
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">bounded</xsl:with-param>
				</xsl:call-template>
	<!--<xsl:message>fund-fact: recursing on <xsl:value-of select='substring-after(xsd:restriction/@base,":")'/> for card</xsl:message>-->
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">cardinality</xsl:with-param>
				</xsl:call-template>
	<!--<xsl:message>fund-fact: recursing on <xsl:value-of select='substring-after(xsd:restriction/@base,":")'/> for numeric</xsl:message>-->
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">numeric</xsl:with-param>
				</xsl:call-template>
			</tr>
	</xsl:template>

	<xsl:template name="fundamental-facets">
		<xsl:param name="dt"/>
		<xsl:param name="base"/>
		<xsl:param name="facet"/>
	<!--
	<xsl:message>named temp ff called w/
	name=<xsl:value-of select='$dt/@name'/>,
	base=<xsl:value-of select='$base'/> and
	facet=<xsl:value-of select='$facet'/></xsl:message>
	-->
		<xsl:choose>
			<xsl:when test="$dt/@name=&quot;anySimpleType&quot; or $dt/@name=&quot;deriviationControl&quot;">
				<!-- intentionally empty -->
			</xsl:when>
			<xsl:when test="substring-after($dt/@base,&quot;:&quot;)=&quot;anySimpleType&quot; or $dt//hfp:hasProperty[@name=$facet]">
				<xsl:apply-templates select="$dt//hfp:hasProperty[@name=$facet]"/>
			</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="$datatypes[@name=$base]"/>
					<xsl:with-param name="base" select="substring-after($datatypes[@name=$base]/xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet" select="$facet"/>
				</xsl:call-template>
			</xsl:otherwise>
		</xsl:choose>
	<!--<xsl:message>named temp ff exited w/ name=<xsl:value-of select='$dt/@name'/></xsl:message>-->
	</xsl:template>

	<xsl:template match="xsd:simpleType" mode="rest-fund">
			<tr>
				<td><a href="#{@name}"><xsl:value-of select="@name"/></a></td>
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">ordered</xsl:with-param>
				</xsl:call-template>
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">bounded</xsl:with-param>
				</xsl:call-template>
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">cardinality</xsl:with-param>
				</xsl:call-template>
				<xsl:call-template name="fundamental-facets">
					<xsl:with-param name="dt" select="."/>
					<xsl:with-param name="base" select="substring-after(xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="facet">numeric</xsl:with-param>
				</xsl:call-template>
			</tr>
	</xsl:template>

	<xsl:template match="hfp:hasProperty">
		<td><xsl:value-of select="@value"/></td>
	</xsl:template>
	
	<xsl:template match="revisions">
		<ol>
			<xsl:for-each select="//revisiondesc/slist/sitem">
				<li><xsl:apply-templates/></li>
			</xsl:for-each>
		</ol>
	</xsl:template>

	<xsl:template match="compref">
		<a href="#{@ref}">
			<xsl:value-of select="id(@ref)/parent::*[self::div4 or self::div3 or self::div2]/head"/>
		</a>
	</xsl:template>

	<xsl:template match="subtypes">
		<xsl:variable name="name"><xsl:value-of select="ancestor::div3/head"/></xsl:variable>
		<p>
			The following <a href="#dt-built-in" class="termref"><span class="arrow">&#xB7;</span>built-in<span class="arrow">&#xB7;</span></a>
			datatypes are <a href="#dt-derived" class="termref"><span class="arrow">&#xB7;</span>derived<span class="arrow">&#xB7;</span></a> from
			<strong><xsl:value-of select="$name"/></strong>:
		</p>
		<ul>
			<xsl:variable name="subtypes" select="$datatypes[xsd:restriction[substring-after(@base,':')=$name] or descendant::xsd:list[substring-after(@itemType,':')=$name]]"/>
			<xsl:apply-templates select="$subtypes" mode="subtypes"/>
		</ul>
	</xsl:template>

	<xsl:template match="xsd:simpleType" mode="subtypes">
			<li>
				<a href="#{@name}">
					<xsl:value-of select="@name"/>
				</a>
			</li>
	</xsl:template>

	<xsl:template match="facets">
		<xsl:call-template name="facets">
			<xsl:with-param name="name" select="ancestor::div3/head"/>
			<xsl:with-param name="derivedName" select="ancestor::div3/head"/>
		</xsl:call-template>
	</xsl:template>

	<xsl:template name="facets">
		<xsl:param name="name"/>
		<xsl:param name="derivedName"/>
		<xsl:variable name="facets" select="$datatypes[@name=$name]/xsd:annotation/xsd:appinfo/hfp:hasFacet"/>
		<xsl:choose>
			<xsl:when test="$facets">
				<p>
					<strong><xsl:value-of select="$derivedName"/></strong> has the
					following <a href="#dt-constraining-facet" class="termref"><span class="arrow">&#xB7;</span>constraining facets<span class="arrow">&#xB7;</span></a>:
				</p>
				<ul>
					<xsl:apply-templates select="$facets"/>
				</ul>
			</xsl:when>
			<xsl:otherwise>
				<xsl:call-template name="facets">
					<xsl:with-param name="name" select="substring-after($datatypes[@name=$name]/xsd:restriction/@base,&quot;:&quot;)"/>
					<xsl:with-param name="derivedName" select="$derivedName"/>
				</xsl:call-template>
			</xsl:otherwise>
		</xsl:choose>
	</xsl:template>

	<xsl:template match="xsd:appinfo/hfp:hasFacet[@name]">
		<li><a href="#rf-{@name}"><xsl:value-of select="@name"/></a></li>
	</xsl:template>

</xsl:stylesheet>
