#ifndef casePlanner_hpp
#define casePlanner_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"
#include "caseGenerator.hpp"

namespace planner {

// Experimentor execute an experiment based on given data and random demands.
class Experimentor {
protected:
    /* Variables */
    const data::CaseData* data_;    // A data copy
    State state_;                   // States in one experiment
    double revenue;

    /* Protected methods */
    // isAcceptable check if this order is acceptable with or without upgrade    
    bool isAcceptable(const Order& order, const State& state);

    // findBestRevOrder return the best revenue obtained from random order 
    double findBestRevOrder(const int period, const Order& order);

    virtual void findOptPlan() = 0;

    // updateState update the given state with the given plan
    void updateState(State& state, const MyopicUpgradePlan& plan);

    // getRevDemand return the revenue of random demand from individuals
    double getRevDemand(const State& demand);

public:
    /* Constructors */
    Experimentor();
    Experimentor(const data::CaseData& data);

    /* Public methods */
    void reset();
    void run();
};

// Deterministic planner
class DeterExperimentor: public virtual Experimentor{
protected:
    // findOptPlan modify the given plan, considering the demand we guess. 
    // This problem is formulated in an integer programming problem in this function.
    void findOptPlan(const int period, MyopicUpgradePlan& plan, 
                     const State& state, const State& demand,
                     const std::map<data::tuple2d, double>& prices);
public:
    void run();
};

// Stochastic planner
class StochExperimentor: public virtual Experimentor{
protected:
    // findOptPlan modify the given plan, considering the demand we guess. 
    // This problem is formulated in an stochastic programming problem in this function.
    void findOptPlan(const int period, MyopicUpgradePlan& plan, 
                     const State& state, const std::vector<State>& s_demands,
                     const std::vector<std::map<data::tuple2d, double> >& prices);

public:
    void run();

};

// Adjusted planner
class AdjExperimentor: public virtual Experimentor {
protected:
    // findBaseline find the baseline in bs mode
    double findBaseline(const int period, const MyopicUpgradePlan& plan, 
                        const State& state);
    
    // decide decide whether to accept this plan (upgraded order) or not.
    void decide(MyopicUpgradePlan& plan, double baseline);
};

class ADExperimentor: public AdjExperimentor, public DeterExperimentor {
public:
    void run();
};

class ASExperimentor: public AdjExperimentor, public StochExperimentor {
public:
    void run();
};


class ExperimentsController {

};

}

#endif /* casePlanner_hpp */