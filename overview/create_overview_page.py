#!/s/std/bin/python -O

######################################
#
# Author: Jason Fletchall, 12/2007
#
# This script creates an overview page of the
# analyses for all application-releases in the
# given directory.

# Usage: create_overview_page.py analysisDir overviewPageOutDir
#
######################################

from pyPgSQL import PgSQL

import os
import sys
import commands

sys.path.append("/afs/cs.wisc.edu/u/f/l/fletchal/CBI/vizProject")
import visualize_one

debug = 2



######################################
#
# Creates the final HTML document with
# links to each distribution-release
#
######################################
def createAppPage(indexedBuilds, overviewPageOutDir, appName):

    #Opens the app's html index file for writing
    if appName not in os.listdir(overviewPageOutDir):
        commands.getoutput("mkdir -p " + overviewPageOutDir + appName)
    overviewPage = open(overviewPageOutDir + appName + "/index.html", "w")

    #Generates some opening html tags
    overviewPage.write('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"' +
            '"http://www.w3.org/TR/html4/loose.dtd">' +
            '<html>')

    #Generates a header
    overviewPage.write('<head>' +
                        '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">' +
                        '<title>CBI Application Overview - ' + appName + '</title>' +
                        '<a href="' + overviewPageOutDir + 'appOverview.html"> Up </a>' +  
                        '</head>')

    #Outputs [version, release, PLDI2005 link, George link] in order of build date in table format
    overviewPage.write('<body>\n')
    overviewPage.write('<table border="1">\n')
    overviewPage.write('<tr><th> Version </th><th> Release </th><th> Predicate View </th><th> George View </th></tr>\n')
    
    for month in sorted(indexedBuilds, reverse=True):
        for day in sorted(indexedBuilds[month], reverse=True):
            distribution = indexedBuilds[month][day][0]
            version = indexedBuilds[month][day][1]
            release = indexedBuilds[month][day][2]
            overviewPage.write('<tr>')
            overviewPage.write('<a id="' + version + '-' + release + '">' +
                               '<td>' + version + '</td><td>' + release + '</td>' +
                               '<td><a href="../' + distribution + '/' + appName+'-'+version+'-'+release + '/summary.xml"><img src="' + overviewPageOutDir + '/favicon.png"></img></a></td>' +
                               '<td><a href="../' + distribution + '/' + appName+'-'+version+'-'+release + '/src/index.html"><img src="' + overviewPageOutDir + '/favicon.png"></img></a></td>' +
                               '</a>\n')
            overviewPage.write('</tr>')
            
    overviewPage.write('</table>')
    overviewPage.write('</body>')

    #Closes the document
    overviewPage.write('</html>')
    overviewPage.close()

    print "Wrote", overviewPageOutDir + appName + "/index.html"
    

######################################
#
# Creates the final HTML document with
# links to each distribution-release
#
######################################
def createAppOverview(indexedBuilds, overviewPageOutDir):

    #Opens the final html file for writing
    overviewPage = open(overviewPageOutDir+"appOverview.html", "w")

    #Generates some opening html tags
    overviewPage.write('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"' +
            '"http://www.w3.org/TR/html4/loose.dtd">' +
            '<html>')

    #Generates a header
    overviewPage.write('<head>' +
                        '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8">' +
                        '<title>CBI Application Overview</title>' +
                        '</head>')

    #Outputs links to each application, generates index pages for each application
    overviewPage.write('<body>')
    for app in indexedBuilds:
        overviewPage.write('<ul>')
        overviewPage.write('<li><a href="' + app + '/index.html" id="' + app + '"><font size="4"><strong>' + app + '</strong></font></a></li>\n')
        createAppPage(indexedBuilds[app], overviewPageOutDir, app)
        #for release in linksArray[distribution]:
        #    overviewPage.write('<li>' + linksArray[distribution][release][0])
        #    if linksArray[distribution][release][1] > 0:
        #        overviewPage.write('<img src="favicon.png"></img>' + '<strong>' + str(linksArray[distribution][release][1]) + '</strong></li>')
        overviewPage.write('</ul>')
    overviewPage.write('</body>')

    #Closes the document
    overviewPage.write('</html>')
    overviewPage.close()

    print "Wrote", overviewPageOutDir+"appOverview.html"            

######################################
#
# Generates links for each summary page
# in given summary array, and returns them
# in the same array.
#
######################################
def generateLinks(summaryArray, analysisDir):

    for distribution in summaryArray:
        for release in summaryArray[distribution]:
            summaryArray[distribution][release][0] = '<a href="' + analysisDir + '/' + distribution + '/' + release + '/summary.xml">' + release + '</a>'

    return summaryArray


######################################
#
# Queries the database for "interesting builds"
#  - those builds with some failures to analyze
#
# Ouput list row is of the form:
#  appName, distribution, version, release, month(since year 0 A.D.), dayOfTheMonth
#
######################################
def query(queryname):
    db = PgSQL.connect(host='postgresql.cs.wisc.edu', port=49173, database='cbi')
    db.autocommit = False

    query = file(queryname).read()
    cursor = db.cursor()
    cursor.execute(query)

    while True:
        rows = cursor.fetchmany()
        if rows:
            for apName, version, release, distribution, year, month, day in rows:
                yield apName, distribution, version, release, 12 * year + month, day
        else:
            break

    db.rollback()
    db.close()


######################################
#
# Indexes the given list by appName, then by date
#
# Ouput dictionary is of the form:
#  appName =>
#             month =>
#                      day =>
#                             distribution, version, release
#
######################################
def indexByAppname_Date(interestingBuilds):

    indexedBuilds = {}
    for build in interestingBuilds:
        #top index is by app
        appName = build[0]
        if appName not in indexedBuilds:
            indexedBuilds[appName] = {}

        #second index is by build month
        month = build[4]
        if month not in indexedBuilds[appName]:
            indexedBuilds[appName][month] = {}

        #third index is by day of the month
        day = build[5]
        if day not in indexedBuilds[appName][month]:
            indexedBuilds[appName][month][day] = [build[1], build[2], build[3]]

    return indexedBuilds


######################################
#
# Indexes the given list by distribution, then by date, then by appName
#
# Ouput dictionary is of the form:
#  distribution =>
#                  month =>
#                           day =>
#                                  appName =>
#                                             version, release
#
######################################
def indexByDistribution_Date(interestingBuilds):

    indexedBuilds = {}
    for build in interestingBuilds:
        #top index is by distribution
        distribution = build[1]
        if distribution not in indexedBuilds:
            indexedBuilds[distribution] = {}

        #second index is by build month
        month = build[4]
        if month not in indexedBuilds[distribution]:
            indexedBuilds[distribution][month] = {}

        #third index is by day of the month
        day = build[5]
        if day not in indexedBuilds[distribution][month]:
            indexedBuilds[distribution][month][day] = {}

        #fourth index is by appName
        appName = build[0]
        if appName not in indexedBuilds[distribution][month][day]:
            indexedBuilds[distribution][month][day][appName] = [build[2], build[3]]

    return indexedBuilds
        


######################################
#
# Gets all the summary pages in the given
# directory, and returns those in an array
# indexed by build distribution, release.
#
######################################
def getSuccessfulSummaryPages(analysisDir, undo, backup):

    #Walks the given directory for summary.xml files
    summaryArray = {}
    for analysisPath, dirs, resultFiles in os.walk(analysisDir):
        #Looking in any reports, src, or sites directories is a waste of time
        if 'data' in dirs:
            dirs.remove('data')
        if 'debug' in dirs:
            dirs.remove('debug')
        if 'sites' in dirs:
            dirs.remove('sites')
        if 'src' in dirs:
            dirs.remove('src')
        #if len(analysisDir.split('/')) == 5:

        for resultFile in resultFiles:
            if (resultFile == "summary.xml"):
                #Look in directory containing summary (analysis home) for preds.txt, count # predicates (if any)
                preds = open(analysisPath + "/preds.txt", "r")
                predList = preds.readlines()
                numPreds = len(predList)
                
                #All paths take the form .../distribution/release/, so extract distribution & release names
                splitPath = analysisPath.rsplit("/", 2)
                distribution = splitPath[1]
                release = splitPath[2]
                if distribution not in summaryArray:
                    summaryArray[distribution] = {}
                summaryArray[distribution][release] = ["summary.xml", numPreds]

                #Backup src htmls if requested
                if backup == True:
                    createBackup(analysisPath)
                #Process html views to pretty them up
                #highlightSource(analysisPath)
                #Undo George highlighting
                if undo == True:
                    undoHighlight(analysisPath)

                #Update links in predicate table to make what they link to more readable
                #processPredicateLinks(analysisPath)
                
                print "Found summary page for", distribution + "/" + release

    return summaryArray


######################################
#
# Highlights all the src html files using
# George.
#
######################################
def highlightSource(resultsDir):
    sys.argv = "junk","-esp",resultsDir
    visualize_one.main()


######################################
#
# "Undoes" the html highlighting done by
# George.  This is really just copying
# all the html files back from a backup
# directory.
#
######################################
def undoHighlight(resultsDir):
    if not resultsDir.endswith("/"):
        resultsDir += "/"
    copyFromDir = resultsDir + "srcBACKUP/"
    copyToDir = resultsDir + "src/"
    commands.getoutput("../george/utils/copy_dir_files " + copyFromDir + " " + copyToDir)


######################################
#
# Creates a backup of all the src html
# files and stores them in srcBACKUP/
#
######################################
def createBackup(resultsDir):
    if not resultsDir.endswith("/"):
        resultsDir += "/"
    if 'srcBACKUP' not in os.listdir(resultsDir):
        commands.getoutput('mkdir ' + resultsDir + 'srcBACKUP')
    copyToDir = resultsDir + "srcBACKUP/"
    copyFromDir = resultsDir + "src/"
    commands.getoutput("../george/utils/copy_dir_files " + copyFromDir + " " + copyToDir) 
    

def main():

    ##TODO
    # 1. Copy all things of interest to given output directory (summary.xml, supporting xmls, debug dir, etc.)
    # 2. Overview should go at top of that --- provide option to not copy everything (just place overview on top)

    if len(sys.argv) != 3 and len(sys.argv) != 4:
        print "Usage: create_overview_page.py analysisDir overviewPageOutDir [-u -b]"
        print "Where -u indicates that George highlighting should be undone."
        print "Where -b indicates that a backup of src html should be made."
        return

    analysisDir = sys.argv[1]
    if not analysisDir.endswith('/'):
        analysisDir += '/'
    overviewPageOutDir = sys.argv[2]
    if not overviewPageOutDir.endswith('/'):
        overviewPageOutDir += '/'
    undo = False
    backup = False
    if len(sys.argv) == 4:
        option = sys.argv[3]
        if option == "-u":
            undo = True
        elif option == "-b":
            backup = True

    interestingBuilds = list(query("interesting_builds.sql"))
    
    appName_indexedBuilds = indexByAppname_Date(interestingBuilds)
    createAppOverview(appName_indexedBuilds, overviewPageOutDir)
    
    #distribution_indexedBuilds = indexByDistribution_Date(interestingBuilds)
    #createDistOverview(distribution_indexedBuilds)

    #summaryArray = getSuccessfulSummaryPages(analysisDir, undo, backup)
    #linksArray = generateLinks(summaryArray, analysisDir)
    #createOverviewPage(linksArray, overviewPageOutDir)

    if debug > 1:
        commands.getoutput("firefox " + overviewPageOutDir + "appOverview.html")


if __name__ == '__main__':
    main()
