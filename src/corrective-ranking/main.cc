#include <cmath>
#include <iostream>

#include "../Confidence.h"
#include "../Progress/Bounded.h"
#include "../Stylesheet.h"
#include "../ViewPrinter.h"

#include "Predicates.h"
#include "allFailures.h"

using namespace std;


////////////////////////////////////////////////////////////////////////
//
//  The main iterative ranking / selection / dilution loop
//


void
buildView(Predicates &candidates, const char projection[])
{
  // create XML output file and write initial header
  ViewPrinter view(Stylesheet::filename, "corrected-view", "all", "hl", projection);

  Progress::Bounded progress("ranking predicates", candidates.count);

  // pluck out predicates one by one, printing as we go
  while (!candidates.empty())
    {
      const Predicates::iterator winner = max_element(candidates.begin(), candidates.end());

      view << "<predictor index=\"" << winner->index + 1
	   << "\" initial=\"" << winner->initial
	   << "\" effective=\"" << winner->effective
	   << "\">"

	   << "<bug-o-meter true-success=\"" << winner->tru.successes.count
	   << "\" true-failure=\"" << winner->tru.failures.count
	   << "\" seen-success=\"" << winner->obs.successes.count
	   << "\" seen-failure=\"" << winner->obs.failures.count
	   << "\" fail=\"" << winner->badness()
	   << "\" context=\"" << winner->context()
	   << "\" increase=\"" << winner->increase()
	   << "\" lower-bound=\"" << winner->lowerBound()
	   << "\" log10-true=\"" << log10(double(winner->tru.count()))
	   << "\"/>"

	   << "</predictor>";

      // cerr << "winner " << winner->index << " dilutes all failures\n";
      allFailures.dilute(winner->tru.failures);
      if (allFailures.count <= 0)
	break;

      for (Predicates::iterator loser = candidates.begin(); loser != candidates.end(); ++loser)
	if (loser != winner)
	  {
	    // cerr << "winner " << winner->index << " dilutes loser " << loser->index << '\n';
	    loser->dilute(*winner);
	  }

      progress.step();
      candidates.erase(winner);
      candidates.rescore(progress);
    }
}


////////////////////////////////////////////////////////////////////////
//
//  Command line processing
//


const char *Stylesheet::filename = "corrected-view.xsl";


void
processCommandLine(int argc, char *argv[])
{
  static const argp_child children[] = {
    { &Confidence::argp, 0, 0, 0 },
    { &Stylesheet::argp, 0, 0, 0 },
    { 0, 0, 0, 0 }
  };

  static const argp argp = {
    0, 0, 0, 0, children, 0, 0
  };

  argp_parse(&argp, argc, argv, 0, 0, 0);
}


////////////////////////////////////////////////////////////////////////
//
//  The main event
//


void
rankMain(int argc, char *argv[], const char projection[])
{
  // command line processing and other initialization
  set_terminate(__gnu_cxx::__verbose_terminate_handler);
  processCommandLine(argc, argv);
  ios::sync_with_stdio(false);
  // feenableexcept(FE_DIVBYZERO | FE_INVALID);

  {
    ifstream failuresFile("f.runs");
    assert(failuresFile);
    failuresFile >> allFailures;
  }

  Predicates candidates;
  buildView(candidates, projection);

  cerr << "allFailures.count == " << allFailures.count << '\n';
}