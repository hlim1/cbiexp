#include <cassert>
#include <cstdio>
#include <iostream>
#include "TsReportReader.h"
#include "SiteCoords.h"
#include "fopen.h"

using namespace std;

void TsReportReader::read(unsigned runId, const char *when)
{
  FILE * const report = fopenRead(TimestampReport::format(runId, when));

  SiteCoords coords;
  timestamp ts;
  unsigned ctr;

  while (true)
    {
      ctr = fscanf(report, "%u\t%u\t%Lu\n", &coords.unitIndex, &coords.siteOffset, &ts);
      if (ctr != 3)
        break;

      siteTs(coords, ts);
    }

  fclose(report);
}
