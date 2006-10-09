#ifndef INCLUDE_SetVector_h
#define INCLUDE_SetVector_h

#include <iostream>
#include <vector>

using namespace std;

class SetVector : private vector<bool>
{
public:
    void initialize(unsigned int); 
    void initialize(vector<bool> &);
    void insert(unsigned);
    bool find(unsigned) const;
    size_t setSize();

    void print(ostream &) const;
    void load(istream &);
    size_t intersectionSize(const SetVector &) const;
    bool nonEmptyIntersection(const SetVector &) const; 
    void calc_union(const SetVector &, SetVector &) const; 

};

ostream & operator<<(ostream &, const SetVector &);
istream & operator>>(istream &, SetVector &); 

#endif // !INCLUDE_SetVector_h

