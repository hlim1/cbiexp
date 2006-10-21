#include <argp.h>
#include <iostream>
#include <iterator>
#include <fstream>
#include <string>
#include <vector>
#include "../fopen.h" 
#include "../RunsDirectory.h"
#include "../NumRuns.h"
#include "../Progress/Bounded.h"
#include "FailureUniverse.h"

using namespace std;

#ifdef HAVE_ARGP_H

static void process_cmdline(int argc, char **argv)
{
    static const argp_child children[] = {
        { &RunsDirectory::argp, 0, 0, 0 }, 
        { &NumRuns::argp, 0, 0, 0 }, 
        { 0, 0, 0, 0 }
    };

    static const argp argp = {
        0, 0, 0, 0, children, 0, 0
    };

    argp_parse(&argp, argc, argv, 0, 0, 0); 
}

#else // !HAVE_ARGP_H

inline void process_cmdline(int, char *[]) { }

#endif // HAVE_ARGP_H

int main(int argc, char** argv)
{
    process_cmdline(argc, argv);
    ios::sync_with_stdio(false);

    const unsigned numRuns = FailureUniverse::getUniverse().cardinality();
    /**************************************************************************
    * Read entropy information for each predicate 
    **************************************************************************/
    ifstream run_entropy("run_entropy.txt");
    vector <double> entropy; 
    copy(istream_iterator<double>(run_entropy), istream_iterator<double>(), back_inserter(entropy)); 
    assert(entropy.size() == numRuns); 
    run_entropy.close(); 

    FILE* out = fopenWrite("run_run_MI_normalized.txt"); 

    ifstream run_MI("run_run_MI.txt");
    Progress::Bounded progress("normalizing mutual information", numRuns);
    for(unsigned int i = 0; i < numRuns; i++) {
        progress.step();
        for(unsigned int j = 0; j < numRuns; j++) { 
            double current; 
            run_MI >> current;
            double iEntropy = entropy[i];
            double jEntropy = entropy[j];
            current = current == 0.0 ? 0.0 : 
                       iEntropy > jEntropy ? 
                         current / iEntropy : 
                         current / jEntropy;
            fprintf(out, "%g\t", current);
        }
        fprintf(out, "\n");
    }
    fclose(out); 

}
