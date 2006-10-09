#include <argp.h>
#include <numeric>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <iterator>
#include "RunsDirectory.h"
#include "NumRuns.h"
#include "Progress/Bounded.h"
#include "PredStats.h"

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

/*****************************************************************************
* Set union, where set inclusion is represented by true at the 
* appropriate index in a vector<bool>.
****************************************************************************/
vector <bool> *
set_union(const vector <bool> & first, const vector <bool> & second)
{
    assert(first.size() == second.size());
    vector<bool> * result = new vector<bool>();
    transform(first.begin(), first.end(), second.begin(), back_inserter(*result), logical_or<bool>());
    return result;
}

class Union : public binary_function <vector <bool> *, vector <bool> *, vector <bool> *> {
public:
    Union() { }
    vector <bool> * operator()(const vector <bool> * first, const vector <bool> * second) const {
        return set_union(*first, *second);
    }
};

/***************************************** *************************************
* We don't need to find the set which is the intersection; we just need to know
* whether it is non-empty.
******************************************************************************/
bool
nonEmptyIntersection(const vector <bool> & first, const vector <bool> & second)
{
    return inner_product(first.begin(), first.end(), second.begin(), false);
}

class NonEmptyIntersection : public unary_function <vector <bool> *, bool> {
public:
    NonEmptyIntersection(const vector <bool> * first) {
        this->first = first;
    }

    bool operator()(const vector <bool> * second) const {
        return nonEmptyIntersection(*first, *second); 
    }
private:
    const vector <bool> * first;
};

struct delete_object
{
  template <typename T> void operator()(T *ptr){ delete ptr;}
};

/******************************************************************************
* Take the head of the list. If any element in the tail of the list has a 
* non-empty intersection with the head remove it from the tail. Return the
* union of the head of the list and all removed elements. 
******************************************************************************/
vector <bool> *
coalesce(list <vector <bool> *> & theList) 
{
    vector <bool> * head = theList.front();
    theList.pop_front();
    list <vector <bool> *>::iterator keep = partition(theList.begin(), theList.end(), NonEmptyIntersection(head));
    vector <bool> * theUnion = accumulate(theList.begin(), keep, head, Union());
    assert(theUnion->size() == head->size());
    delete head;
    for_each(theList.begin(), keep, delete_object());
    theList.erase(theList.begin(), keep);
    return theUnion;
}

class LowerBound : public unary_function <double, bool> {
public:
    LowerBound(double bound) {
        this->bound = bound;
    }

    bool operator()(double val) const {
        return val >= bound;
    }

private:
    double bound;
};

int main(int argc, char** argv)
{
    process_cmdline(argc, argv);
    ios::sync_with_stdio(false);

    /**************************************************************************
    * Read in the normalized mutual information between a pred and all other
    * preds. The normalized mutual information between a pred and itself is 1.
    * If the predicate has a score that is at least 0.9 with at least one
    * pred other than itself then we have found a set of closely associated 
    * predicates that is worth keeping. 
    **************************************************************************/
    vector < vector <bool> * > sets; 
    const unsigned numPreds = PredStats::count();
    ifstream pred_MI("pred_pred_MI_normalized.txt");
    Progress::Bounded reading("calculating predicate sets", numPreds);
    for(unsigned int i = 0; i < numPreds; i++) {
        reading.step();
        string line;
        getline(pred_MI, line);
        istringstream parse(line);
        vector <bool> current;
        transform(istream_iterator<double>(parse), istream_iterator<double>(), back_inserter(current), LowerBound(0.9));
        assert(current.size() == numPreds);
        int cardinality = count(current.begin(), current.end(), true);
        assert(cardinality > 0);
        if(cardinality > 1) {
            sets.push_back(new vector<bool>(current));
        }
    }
    assert(pred_MI.peek() == EOF);
    pred_MI.close();

    /**************************************************************************
    * Having found these sets we need to coalesce them. If two sets have a 
    * non-empty intersection we replace them with a single set which is the
    * union of the intersecting sets.
    **************************************************************************/
    ofstream out("pred_sets.txt");
    Progress::Bounded coalescing("coalescing sets", sets.size()); 
    list <vector <bool> * > set_list(sets.begin(), sets.end()); 
    while(!set_list.empty()) {
        coalescing.step();
        vector <bool> * current = coalesce(set_list); 
        assert(current->size() == numPreds); 
        for(unsigned int j = 0; j < current->size(); j++) {
            if((*current)[j]) out << j << "\t"; 
        }
        out << "\n";
    }
    out.close();

}
