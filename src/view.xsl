<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

  <xsl:output
    method="html"
    indent="yes"
    doctype-system="http://www.w3.org/TR/html4/strict.dtd"
    doctype-public="-//W3C//DTD HTML 4.01//EN"/>


  <xsl:variable name="projectURL" select="'http://www.cs.berkeley.edu/~liblit/sampler/'"/>


  <!-- look up schemes by their short id codes -->
  <xsl:key name="scheme" match="scheme" use="@code"/>

  <!-- where are we right now? -->
  <xsl:variable name="current-scheme-code" select="/view/schemes/@current"/>
  <xsl:variable name="current-scheme-name" select="key('scheme', $current-scheme-code)/@name"/>
  <xsl:variable name="current-page">
    <xsl:value-of select="$current-scheme-code"/>_<xsl:value-of select="/view/sorts/@current"/>
  </xsl:variable>

  <!-- main master template for generated page -->
  <xsl:template match="/view">
    <xsl:variable name="title">
      Cooperative Bug Isolation Report: <xsl:value-of select="$current-scheme-name"/> predicates
    </xsl:variable>
    <html>
      <head>
	<title><xsl:value-of select="$title"/></title>
	<link rel="stylesheet" href="view.css" type="text/css"/>
      </head>

      <body>
	<!-- generic page header -->
	<h1>
	  <a href="{$projectURL}" class="logo">
	    <img src="{$projectURL}logo.png" alt="" style="border-style: none"/>
	  </a>
	</h1>

	<table class="views">
	  <xsl:apply-templates select="schemes"/>
	  <xsl:apply-templates select="sorts"/>
	  <tr>
	    <th>Go to:</th>
	    <td>
	      [<a href="{@summary}">report summary</a>]
	      [<a href="{$projectURL}">CBI web page</a>]
	    </td>
	  </tr>
	</table>

	<xsl:apply-templates select="predictors"/>

      </body>
    </html>
  </xsl:template>


  <!-- row of links to other schemes' pages -->
  <xsl:template match="schemes">
    <tr>
      <th>Scheme:</th>
      <td><xsl:apply-templates/></td>
    </tr>
  </xsl:template>


  <!-- link to a single other scheme's page -->
  <xsl:template match="scheme">
    <xsl:call-template name="other-view">
      <xsl:with-param name="name" select="@name"/>
      <xsl:with-param name="scheme" select="@code"/>
      <xsl:with-param name="order" select="../../sorts/@current"/>
    </xsl:call-template>
  </xsl:template>


  <!-- row of links to other sorting orders' pages -->
  <xsl:template match="sorts">
    <tr>
      <th>Sorted by:</th>
      <td>
	<xsl:call-template name="other-view">
	  <xsl:with-param name="name" select="'lower bound of confidence interval'"/>
	  <xsl:with-param name="scheme" select="../schemes/@current"/>
	  <xsl:with-param name="order" select="'lb'"/>
	</xsl:call-template>
	<xsl:call-template name="other-view">
	  <xsl:with-param name="name" select="'increase score'"/>
	  <xsl:with-param name="scheme" select="../schemes/@current"/>
	  <xsl:with-param name="order" select="'is'"/>
	</xsl:call-template>
	<xsl:call-template name="other-view">
	  <xsl:with-param name="name" select="'fail score'"/>
	  <xsl:with-param name="scheme" select="../schemes/@current"/>
	  <xsl:with-param name="order" select="'fs'"/>
	</xsl:call-template>
	<xsl:call-template name="other-view">
	  <xsl:with-param name="name" select="'true in # F runs'"/>
	  <xsl:with-param name="scheme" select="../schemes/@current"/>
	  <xsl:with-param name="order" select="'nf'"/>
	</xsl:call-template>
      </td>
    </tr>
  </xsl:template>


  <!-- a single link to an alternative view (different scheme or sort order) -->
  <xsl:template name="other-view">
    <xsl:param name="name"/>
    <xsl:param name="scheme"/>
    <xsl:param name="order"/>

    <!-- where are we going? -->
    <xsl:variable name="destination">
      <xsl:value-of select="$scheme"/>_<xsl:value-of select="$order"/>
    </xsl:variable>

    <!-- output a placeholder or a true link -->
    [<xsl:choose>
      <xsl:when test="$destination = $current-page">
	<!-- placeholder for the link to ourself -->
	<xsl:value-of select="$name"/>
      </xsl:when>
      <xsl:otherwise>
	<!-- true link to someone else -->
	<a href="{$destination}.{/view/@prefix}.xml"><xsl:value-of select="$name"/></a>
      </xsl:otherwise>
    </xsl:choose>]
  </xsl:template>


  <!-- table of all retained predictors -->
  <xsl:template match="predictors">
    <table class="predictors">
      <tr>
	<th/>
	<th>predicate</th>
	<th>function</th>
	<th>file:line</th>
      </tr>
      <xsl:apply-templates/>
    </table>
  </xsl:template>


  <!-- a single retained predictor -->
  <xsl:template match="predictor">
    <tr>
      <td>
	<table class="scores" width="{scores/@log10-seen * 60}px" title="Ctxt: {round(scores/@context * 100)}%, LB: {round(scores/@lower-bound * 100)}%, Incr: {round(scores/@increase * 100)}%, Fail: {round(scores/@badness * 100)}%&#10;tru in {true/@success} S and {true/@failure} F&#10;obs in {seen/@success} S and {seen/@failure} F">
	  <tr>
	    <td class="f1" style="width: {scores/@context * 100}%"/>
	    <td class="f2" style="width: {scores/@lower-bound * 100}%"/>
	    <td class="f3" style="width: {(scores/@badness - (scores/@context + scores/@lower-bound)) * 100}%"/>
	    <td class="f4" style="width: {(1 - scores/@badness) * 100}%"/>
	  </tr>
	</table>
      </td>
      <xsl:apply-templates select="source"/>
    </tr>
  </xsl:template>


  <!-- source information for a single predictor -->
  <xsl:template match="source">
    <td><xsl:value-of select="translate(@predicate, ' ', '&#160;')"/></td>
    <td><xsl:value-of select="@function"/></td>
    <td><xsl:value-of select="@file"/>:<xsl:value-of select="@line"/></td>
  </xsl:template>


</xsl:stylesheet>