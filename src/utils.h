#ifndef UTILS_H
#define UTILS_H

#include <cstdio>
#include <string>
#include "PredCoords.h"


struct pred_stat {
    double lb;
    double in;
    double fs;
    double co;
    double imp;
    double fdenom;
    double sdenom;
};

struct pred_info : public PredCoords {
    int siteIndex;
    pred_stat ps;
    int s, f, os, of;
};


pred_stat compute_pred_stat(int s, int f, int os, int of, int confidence);

bool retain_pred(int s, int f, double lb);
bool retain_pred(int s, int f, int os, int of, int conf);

bool read_pred_full(FILE* fp, pred_info &);

const std::string &scheme_name(char code);

/* Return the index for the complement of the predicate
 * for pred indices 0, 2, 4, complement is 1, 3, 5 and vice versa
 */
PredCoords notP (PredCoords &p);

#endif
