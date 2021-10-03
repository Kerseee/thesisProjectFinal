#ifndef casePlanner_hpp
#define casePlanner_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "caseData.hpp"
#include "planner.hpp"

namespace planner {

// Experimentor execute an experiment based on given data and random demands.
class Experimentor {
protected:
    const data::CaseData* data_;    // A data copy
    State state_;                   // States in one experiment
    double revenue;

    // isAcceptable check if this order is acceptable with or without upgrade    
    bool isAcceptable(const Order& order, const State& state);

public:
    Experimentor();
    Experimentor(const data::CaseData& data);

    void reset();
    void run();
};

// 待決定是否需要這個
class MyopicCasePlanner {

};

}

#endif /* casePlanner_hpp */