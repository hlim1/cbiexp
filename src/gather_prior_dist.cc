#include <argp.h>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <ext/hash_map>
#include <fstream>
#include <numeric>
#include <queue>
#include "DatabaseFile.h"
#include "DiscreteDist.h"
#include "NumRuns.h"
#include "PredStats.h"
#include "Progress/Bounded.h"
#include "ReportReader.h"
#include "SiteCoords.h"
#include "classify_runs.h"
#include "fopen.h"
#include "utils.h"

using namespace std;
using __gnu_cxx::hash_map;

static unsigned cur_run;

////////////////////////////////////////////////////////////////
//
// Discrete Distribution
// (Actually, it's a measure, i.e., unnormalized counts
//

typedef pair<DiscreteDist, DiscreteDist> DistPair;
typedef DiscreteDist (DistPair::* Dist);

static ostream &
operator<<(ostream &out, const DiscreteDist &d)
{
  out << d.size() << '\n';
  for (DiscreteDist::const_iterator c = d.begin(); c != d.end(); c++) {
    const count_tp n = c->first;
    const unsigned val = c->second;
    out << n << ' ' << val << ' ';
  }
  return out << '\n';
}

////////////////////////////////////////////////////////////////
//
// Empirical distribution of number of times reached for one retained site
//
struct SiteInfo
{
  DistPair reached;

  void notice(Dist, count_tp);
  //void print(ostream &, ostream &) const;
};

// Since the site is reached n times during this run, increment the
// corresponding counter in the empirical measure
inline void
SiteInfo::notice(Dist d, count_tp n){
  DiscreteDist &disthash = reached.*d;
  DiscreteDist::iterator found = disthash.find(n);
  if (found == disthash.end())
    disthash[n] = 1;
  else
    disthash[n] += 1;
}

typedef hash_map<SiteCoords, SiteInfo> SiteSeen;
static SiteSeen siteSeen;

typedef hash_map<SiteCoords, unsigned> SiteCounter;
static SiteCounter siteCount;

////////////////////////////////////////////////////////////////
//
// Report Reader
//

class Reader : public ReportReader
{
public:
  Reader(const char* filename) : ReportReader(filename) {}

  void setd(unsigned);
  void countZeros();

protected:
  void handleSite(const SiteCoords &, vector<count_tp> &);

private:
  Dist d;
};

inline void
Reader::setd(unsigned run)
{
  siteCount.clear();
  if (is_frun[run]) d = &DistPair::first;
  else
    if (is_srun[run]) d = &DistPair::second;
    else
    {
         ostringstream message;
         message << "Ill-formed run " << run;
         throw runtime_error(message.str());
    }
}

void
Reader::handleSite(const SiteCoords &coords, vector<count_tp> &counts)
{
  SiteCounter::iterator f = siteCount.find(coords);
  if (f == siteCount.end())
    siteCount[coords] = 1;
  else
    siteCount[coords] += 1;

  const SiteSeen::iterator found = siteSeen.find(coords);
  if (found != siteSeen.end())
  {
    const count_tp sum = accumulate(counts.begin(), counts.end(), (count_tp) 0);
    SiteInfo &info = found->second;
    info.notice(d, sum);
  }
}

// Compact reports do not contain sites which are not reached.
// Hence we must manually add those zeros into the empirical distribution
void Reader::countZeros()
{
  for (SiteSeen::iterator s = siteSeen.begin(); s != siteSeen.end(); s++) {
    const SiteCoords &coords = s->first;
    SiteInfo &info = s->second;
    const SiteCounter::iterator found = siteCount.find(coords);
    if (found == siteCount.end())
      info.notice(d, 0);
  }
}

////////////////////////////////////////////////////////////////////////
//
//  Command line processing
//


static void process_cmdline(int argc, char **argv)
{
    static const argp_child children[] = {
        { &DatabaseFile::argp, 0, 0, 0 },
        { &NumRuns::argp, 0, 0, 0 },
        { 0, 0, 0, 0 }
    };

    static const argp argp = {
	0, 0, 0, 0, children, 0, 0
    };

    argp_parse(&argp, argc, argv, 0, 0, 0);
};

////////////////////////////////////////////////////////////////
//
// The main event
//

int main (int argc, char** argv)
{
  process_cmdline (argc, argv);

  ofstream ffp("fpriors.dat");
  ofstream sfp("spriors.dat");

  typedef queue<SiteCoords> Sites;
  Sites retainedSites;

  FILE * const pfp = fopenRead(PredStats::filename);
  pred_info pi;
  while (read_pred_full(pfp, pi)) {
    const SiteSeen::iterator found = siteSeen.find(pi);
    if (found == siteSeen.end()) {
      SiteInfo &siteInfo = siteSeen[pi];
      siteInfo.reached.first.clear();
      siteInfo.reached.second.clear();
      retainedSites.push(pi);
    }
  }

  fclose(pfp);

  Reader reader(DatabaseFile::DatabaseName);
  Progress::Bounded progress("Gathering empirical distribution", NumRuns::count());
  for (cur_run = NumRuns::begin(); cur_run < NumRuns::end(); cur_run++) {
    progress.step();
    reader.setd(cur_run);
    reader.read(cur_run);
    reader.countZeros();
  }

  while (!retainedSites.empty()) {
    const SiteCoords &coords = retainedSites.front();
    const SiteSeen::iterator found = siteSeen.find(coords);
    if (found == siteSeen.end())
      cerr << "Error: Cannot find site " << coords.siteIndex << '\n';
    else {
      const SiteInfo &info = found->second;
      ffp << coords.siteIndex << '\n';
      ffp << info.reached.first;
      sfp << coords.siteIndex << '\n';
      sfp << info.reached.second;
    }
    retainedSites.pop();
  }

  ffp.close();
  sfp.close();
  return 0;
}
