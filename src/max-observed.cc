#include <argp.h>
#include <ext/hash_map>
#include <fstream>
#include <iostream>
#include <numeric>
#include "CompactReport.h"
#include "NumRuns.h"
#include "Progress/Bounded.h"
#include "ReportReader.h"
#include "RunsDirectory.h"
#include "SiteCoords.h"
#include "classify_runs.h"
#include "termination.h"

using namespace std;


typedef __gnu_cxx::hash_map<SiteCoords, count_tp> SitesMap;
SitesMap sites;


////////////////////////////////////////////////////////////////////////
//
//  report reader
//


class Reader : public ReportReader
{
public:
  Reader();
  unsigned increased;

protected:
  void handleSite(const SiteCoords &, vector<count_tp> &);
};


Reader::Reader()
  : increased(0)
{
}


void
Reader::handleSite(const SiteCoords &coords, vector<count_tp> &counts)
{
  const count_tp observations = accumulate(counts.begin(), counts.end(), (count_tp) 0);
  count_tp &maxObserved = sites[coords];
  if (maxObserved < observations)
    {
      maxObserved = observations;
      ++increased;
    }
}


////////////////////////////////////////////////////////////////////////
//
//  Command line processing
//


static void
processCommandLine(int argc, char **argv)
{
  static const argp_child children[] = {
    { &CompactReport::argp, 0, 0, 0 },
    { &NumRuns::argp, 0, 0, 0 },
    { &ReportReader::argp, 0, 0, 0 },
    { &RunsDirectory::argp, 0, 0, 0 },
    { 0, 0, 0, 0 }
  };

  static const argp argp = {
    0, 0, 0, 0, children, 0, 0
  };

  argp_parse(&argp, argc, argv, 0, 0, 0);
};


////////////////////////////////////////////////////////////////////////
//
//  The main event
//


int
main(int argc, char** argv)
{
  set_terminate_verbose();
  processCommandLine(argc, argv);

  {
    classify_runs();

    Progress::Bounded progress("scanning reports", NumRuns::count());
    ofstream increases("max-observed-increases.txt");

    for (unsigned runId = NumRuns::begin; runId < NumRuns::end; runId++)
      {
	progress.step();
	if (is_srun[runId] || is_frun[runId])
	  {
	    Reader reader;
	    reader.read(runId);
	    increases << reader.increased << '\n';
	  }
      }
  }

  for (SitesMap::const_iterator site = sites.begin(); site != sites.end(); ++site)
    cout << site->first << '\t' << site->second << '\n';

  return 0;
}
