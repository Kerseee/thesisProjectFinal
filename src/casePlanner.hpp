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

// Experimentor define the basic general myopic process.
class Experimentor {
protected:
    /* Variables */
    Hotel* hotel;
    double revenue;
    
    /* Protected methods */

    // handleOrder handle the order received in given period and return revenue
    double handleOrder(const int period, const Order& order);
    
    // handleOrder handle the individual remand received in given period
    // and return revenue
    double handleIndDemand(
        const int period, const std::map<data::tuple2d, int>& ind_demand);
    
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

// CaseExperimentor extend the Experimentor, and define the use of CaseData
class CaseExperimentor: public virtual Experimentor{
protected:
    /* Variables */
    const data::CaseData* data;    // A data copy
    const std::map<int, Order>* orders; // Order received in each period
    
    // Demands received in each period: 
    // ind_demands[period][{service_period, room_type}] = num
    const std::map<int, std::map<data::tuple2d, int> >* ind_demands;

    // demandJoin return a demand map copied from org_demand and add elements
    // {{day, roomtype}}:0} for those day that not in org_demand but in days.
    std::map<data::tuple2d, int> demandJoin(
        const std::map<data::tuple2d, int>& org_demand, 
        const std::set<int>& days)

public:
    /* Constructors and destructor */
    CaseExperimentor();

    // addOrders add orders for each booking period   
    void addOrders(const std::map<int, Order>& orders);
    // addIndDemands add individual demands for each booking period
    void addIndDemands(
        const std::map<int, std::map<data::tuple2d, int> >& demands);
};

// Deterministic planner
class DeterExperimentor: public virtual CaseExperimentor{
protected:
    /* Variables */
    // Estimated future remand in each period: 
    // estimated_ind_demands[period][{service_period, room_type}] = num
    const std::map<int, std::map<data::tuple2d, int> >* estimated_ind_demands;
 
    /* Protected methods */

    // decide() is called by findBestOrderDecision, it go through the
    // upgrade algorithm and store the upgraded information into od. 
    void decide(const int period, OrderDecision& od);

    // findBestOrderDecision return the best order decision
    OrderDecision findBestOrderDecision(
        const int period, const Order& order) override;

public:
    /* Constructors and destructor */
    DeterExperimentor();
    DeterExperimentor(const data::CaseData& data);
    ~DeterExperimentor();

    // addEstIndDemands add the estimated future individual demands.
    void addEstIndDemands(
        const std::map<int, std::map<data::tuple2d, int> >& est_demands);

    /* Public method */
    void run();
};

// Stochastic planner
class StochExperimentor: public virtual CaseExperimentor{
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
class AdjExperimentor: public virtual CaseExperimentor {
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

// raiseKeyError output the error message and exit
template<typename T>
void raiseKeyError(const std::string& location, const T& key, 
    const std::string container);

#endif /* casePlanner_hpp */