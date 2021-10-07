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
#include "caseStructures.hpp"

namespace planner {

// Experimentor execute an experiment based on given data and random demands.
class Experimentor {
protected:
    /* Variables */
    Hotel* hotel;
    double revenue;
    
    /* Protected methods */

    // run1Period run the period routine in given period, with given the random
    // order and demand, and then return the total revenue in this period.
    double run1Period(const int period, const Order& order, 
                      const std::map<data::tuple2d, int>& ind_demand);

    // isAcceptable check if this order is acceptable with or without upgrade    
    bool isAcceptable(const Order& order);

    // findBestOrderDecision return the best order decision 
    virtual OrderDecision findBestOrderDecision(
        const int period, const Order& order) = 0;

    // getAcceptedIndDemand return the actual acceptable individual demand
    std::map<data::tuple2d, int> getAcceptedIndDemand(
        const std::map<data::tuple2d, int>& ind_demand);
    
    // getAcceptedIndDemand return the actual acceptable individual demand
    // given available rooms
    std::map<data::tuple2d, int> getAcceptedIndDemand(
        const std::map<data::tuple2d, int>& ind_demand,
        const std::map<data::tuple2d, int>& available_rooms);
    
    // getRevIndDemand return the total price of given demand
    double getRevIndDemand(std::map<data::tuple2d, int> acc_demand);

    // booking call this->hotel->booking for each request day and rooms
    // Return false if there is any booking error
    bool booking(const OrderDecision& decision);
    bool booking(const std::map<data::tuple2d, int>& ind_demand);
};

// Deterministic planner
class DeterExperimentor: public virtual Experimentor{
protected:
    /* Variables */
    const data::CaseData* data;    // A data copy
    const std::map<int, Order>* orders; // Order received in each period
    
    // Demands received in each period: 
    // ind_demands[period][{service_period, room_type}] = num
    const std::map<int, std::map<data::tuple2d, int> >* ind_demands;

    // Estimated future remand in each period: 
    // estimated_ind_demands[period][{service_period, room_type}] = num
    const std::map<int, std::map<data::tuple2d, int> >* estimated_ind_demands;
 
    /* Protected methods */

    // findBestOrderDecision return the best order decision
    OrderDecision findBestOrderDecision(
        const int period, const Order& order) override;

public:
    /* Constructors and destructor */
    DeterExperimentor();
    DeterExperimentor(const data::CaseData* data);
    ~DeterExperimentor();

    /* Public method */
    void run();
};

// Stochastic planner
class StochExperimentor: public virtual Experimentor{
protected:
    /* Variables */
    const data::CaseData* data;    // A data copy

    /* Protected methods */

    // findBestOrderDecision return the best order decision
    OrderDecision findBestOrderDecision(
        const int period, const Order& order) override;

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