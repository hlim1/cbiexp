#!/s/python-2.5/bin/python -O
# -*- python -*-

from __future__ import with_statement

import optparse
import sqlite3

from glob import glob
from os import makedirs
from os.path import abspath, basename, dirname, isdir, join
from shutil import rmtree

import utils

from initializeSchema import setupPragmas, setupTables
from readReports import processReports
from runsIntoDB import processLabels
from sitesIntoDB import writeSitesIntoDB


makeTemplate = """
database := cbi.sqlite3
begin-runs := 0
end-runs := %(end)d
root := %(cbiexpDir)s

include %(cbiexpDir)s/src/analysis-rules.mk
"""

def doCBIAnalysis(sitesDir, reportsDir, analysisDir, csurfPrj, version=1):
    if version != 1:
        raise ValueError('Version %s of the database schema is unsupported' %
                         str(version))

    sitesFiles = sorted(glob(join(sitesDir, '*.sites')))
    runDirs = glob(join(reportsDir, '[0-9]*/[0-9]*'))
    runDirs = sorted(runDirs, key=lambda d: int(basename(d)))

    # Create a pristine analysisDir
    if isdir(analysisDir): rmtree(analysisDir)
    makedirs(analysisDir)

    # Create database
    database = join(analysisDir, 'cbi.sqlite3')
    conn = sqlite3.connect(database)

    setupPragmas(conn)
    setupTables(conn)
    writeSitesIntoDB(conn, sitesFiles)
    processLabels(conn, runDirs)
    processReports(conn, runDirs)

    conn.commit()

    runCount = conn.execute('SELECT COUNT(*) from Runs').fetchone()[0]
    conn.close()

    # Setup Makefile
    with file(join(analysisDir, 'GNUmakefile'), 'w') as mFile:
        cbiexpDir = abspath(join(dirname(__file__), '../..'))
        if csurfPrj is not None:
            print >>mFile, 'issta-csurf-project := csurfPrj'
        print >>mFile, makeTemplate % {'cbiexpDir': cbiexpDir, 'end': runCount}

    print "To start analysis, run 'make' in", analysisDir

def main():
    parser = optparse.OptionParser(usage='%prog [options] <sites-dir> <reports-dir> <analysis-dir>')

    parser.add_option('-c', '--csurf-prj', action='store', default=None,
                      help = 'enable ISSTA analysis and use CSURF-PRJ')
    parser.add_option('-v', '--version', action='store', default=1, type='int',
                      help = 'version of schema to implement')

    options, args = parser.parse_args()
    if len(args) != 3:
        parser.error('wrong number of positional arguments')

    sitesDir, reportsDir, analysisDir = args
    version = options.version
    csurfPrj = options.csurfPrj

    doCBIAnalysis(sitesDir, reportsDir, analysisDir, csurfPrj, version)

################################################################################

if __name__ == '__main__':
    main()
