//
//  planner.hpp
//  thesisDP
//
//  Created by 黃楷翔 on 2021/3/9.
//  Copyright © 2021年 黃楷翔. All rights reserved.
//

#ifndef planner_hpp
#define planner_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"

namespace planner {
void createFolder(std::string path);
// Store state in the model.
// In this problem this equals to available rooms.
// Please call State::setScale() before use
struct State{
    static int num_service_period;  // |S|
    static int num_room_type;       // |I|
    //rooms: key is <service period s, room type i>, value is number of available level-i room
    std::map<data::tuple2d, int> rooms;
    double prob;    // now only used in Myopic
    State();
    State(const std::map<int, int>& capacity);
    State(const std::map<int, int>& capacity, double prob);
    State(const State& state);
    State(const std::map<data::tuple2d, int>& state_map);
    State(const State& state, const std::map<data::tuple2d, int>& cap_loss);
    State(const std::map<data::tuple3d, int>& demand_order, const int order);
    static void setScale(const int service_period, const int num_room_type);
    bool operator==(const State& state) const;
    bool operator<(const State& state) const;
    State& operator=(const State& state);
    // findMinRooms return minimum rooms of each levels in given periods
    std::map<int, int> findMinRooms(const std::set<int>& periods) const;
    bool hasMinus();
};

struct UpgradedOrder{
    // Store num of requested level-i rooms, key is room-i,
    // value is number of rooms after upgrading
    std::map<int, int> rooms;
    double upgrade_fee;
    UpgradedOrder();
    UpgradedOrder(const std::map<int, int>& rooms, const double fee);
    UpgradedOrder(const UpgradedOrder& plan);
    UpgradedOrder& operator=(const UpgradedOrder& plan);
};

struct MyopicUpgradePlan{
    int id;         // index of this order
    State order;    // origin request
    State upgraded_order;    // order after upgraded
    std::set<int> days;   // requested days
    double exp_acc_togo; // expected value togo if this order is accepted
    double exp_rej_togo; // expected value togo if this order is rejected
    double price;   // origin price of this order
    double upgrade_fee; // upgrade fee of this order
    double revenue; // price + upgrade_fee if accepted, 0 otherwise
    bool decision;  // 1 if thie order is accepted
    // upgrade information, key:{from, to}, value: num
    std::map<data::tuple2d, int> upgrade;

    MyopicUpgradePlan();
    MyopicUpgradePlan(const int id, const State& order, const std::set<int>& days,
                      const double price, const double upgrade_fee,
                      const std::map<data::tuple2d, int>& upgrade);
    MyopicUpgradePlan(const int id, const State& order, const std::set<int>& days,
                      const double exp_acc_togo, const double exp_rej_togo,
                      const double price, const double upgrade_fee,
                      const std::map<data::tuple2d, int>& upgrade);
    MyopicUpgradePlan(const MyopicUpgradePlan& plan);
    MyopicUpgradePlan& operator=(const MyopicUpgradePlan& plan);
    void decide(const bool accept);
    void print();
    
    // getUpgradedRooms return the upgraded rooms arrangement
    void computeUpgradedRooms();
    
};

// Dynamic Programming planner
class DPPlanner{
private:
    std::map<data::tuple3d, bool> decision_;           // Store decisions of accepting or not
    // key of expected_value: (period, order, index of state in this->states_)
    // that is, (t, k, id), where id is the index of this->states
    std::map<data::tuple3d, double> expected_revenue_;
    std::map<data::tuple3d, double> exp_rev_reject_;
    std::map<data::tuple3d, double> exp_rev_accept_;
    std::vector<State> states_;
    const data::Params* dat_;                             // A copy of input data
    
    // individualRevenue return the revenue of a demand-state from individual custormers.
    double individualRevenue(const State& available_room, const State& ind_demand);

    // stateToIndex return the index of state stored in this->states_
    // Return -1 if this state is not found in this->states_.
    int stateToIndex(const State& state);
    
    // getProdIndDemand return the probability of a demand-state
    double getProdIndDemand(const int booking_period, const State& demand);
    
    // findPosUpgradeOut: given a number of target room, return all possible plan that
    // upgrades lower level rooms, whose sum equals to the number of target room.
    // Only called by DPPlanner::travelUpgradePlans().
    // Return an array of plans. Each plan contains the total number of upgraded
    // room out of level-i room and extra-fee.
    std::vector<UpgradedOrder> findPosUpgradeOut(const std::map<int, int>& avai_room,
                                                  std::map<int, int> upgrade_out,
                                                  std::map<int, int> upgrade_in,
                                                  const std::map<int, int>& req_room,
                                                  const int room_level, const int order);
    
    // travelUpgradePlans travel all possible upgrade plans recursively.
    // All possible plans are stored in upgrade_plans.
    // Only called by DPPlanner::findUpgradePlans().
    void travelUpgradePlans(const std::map<int, int>& avai_room, const std::map<int, int>& req_room,
                            std::map<int, int> upgrade_out, std::map<int, int> uplevel_upgrade_in,
                            const int room_level, double fee, const int order,
                            std::vector<UpgradedOrder>& upgrade_plans);
    
    // findUpgradePlans find all possible upgrade plans givem an order.
    // All upgrade plan are in the demand form. They can be regarded as adjusted demands.
    // Only called by DPPlanner::expRevToGoAccept()
    std::vector<UpgradedOrder> findUpgradePlans(const int booking_period, const int order,
                                                 const State& state);
    
    // expRevToGoReject return the expected revenue to from this booking period to the end
    // of the booking horizon, given booking period, state, and that the order from the travel
    // agency is rejected.
    double expRevToGoReject(const int booking_period, const State& state);
    
    // expRevToGoReject return the expected revenue to from this booking period to the end
    // of the booking horizon, given booking period, state, and that the order from the travel
    // agency is accepted.
    double expRevToGoAccept(const int booking_period, const int order,
                            const State& state);
    
    // computeEV compute and store the expected revenue given booking_period,
    // order id and state_id
    void computeExpRev(const int booking_period, const int order, const int state_id);
    
public:
    enum expValue {max, accpet, reject};
    enum result{revenue, decision};
    DPPlanner();
    DPPlanner(const data::Params& dat);
    
    // Main function of DPPlanner
    double run();
    void printExpRev(expValue ev);
    void printDecision();
    void storeResult(result type, const std::string& file_path);
    void storeTime(const std::string& file_path, const double& time);
    std::vector<State>& getStatesSet();
    
    void debugFunc();
    ~DPPlanner();
};

// nc: naively conservative, bs: baseline search
enum MyopicMode {naive = 0, smart = 1, reserved = 2, nc = 3, bs = 4};
enum MyopicModelType {ip = 0, sp = 1};


class MyopicPlanner{
private:
    int num_experiment_;            // Store the number of experiments
    int num_exper_SP_;              // Store the number of experiments used in SP;
    int early_stop_;
    double threshold_;              // Stop the planner when diff of avg_revs less than this threshold
    State state_;                   // Store the state in 1 experiment
    std::map<data::tuple4d, double> demand_cdf_;  // Store the cdf of random demand
    std::map<data::tuple2d, double> cum_prob_order;  // Store the parameters of distributions of random demand
    std::vector<double> revenues_;  // Store the revenues from each experiment
    std::map<int, double> avg_revs_;    // Store the average (expected) revenues of first key experiments
    const data::Params* dat_;       // A copy of data
    MyopicMode mode_;               // mode of this MP (naive/smart)
    MyopicModelType model_type_;    // model of this MP (MD/MS)
    std::map<int, State> guess_demand_md_;  // guessed demand in period t for md model
    std::map<int, std::map<data::tuple2d, double> > guess_price_md_;
    std::map<int, std::vector<State> > guess_demand_ms_;
    std::map<int, std::vector<std::map<data::tuple2d, double> > > guess_price_ms_;
    std::map<int, State> exp_demand_1p_;  // expected individual demand in each single period    
    int exper_id_;
    std::map<data::tuple2d, int> order_record_;
    std::map<data::tuple2d, int> decision_record_;
    std::map<data::tuple2d, int> acceptable_record_;
    std::map<data::tuple2d, double> rev_rej_record_;
    std::map<data::tuple2d, double> rev_acc_record_;
    std::map<data::tuple2d, double> rev_order_record_;
    std::map<data::tuple2d, double> rev_ind_record_;
    std::map<int, double> time_record_;
    std::map<int, int> sold_out_record_;
    std::map<data::tuple2d, int> random_orders_;
    std::map<data::tuple2d, State> random_demands_;
    bool random_first_;
    double smart_gamma_;    // A coefficient for adjusting future demand and price in smart mode
    std::set<int> best_order_nc_;    // best order in mode nc
    std::map<int, std::vector<MyopicUpgradePlan> > compounded_orders_nc_; 
    double baseline_beta_;  // A coeffiecient for the upper bound of baseline in bs mode

    void setMode(MyopicMode m);

    void setModelType(MyopicModelType mt);

    void storeDemandCdf();

    // isAcceptable check if this order is acceptable with or without upgrade    
    bool isAcceptable(const int order, const State& state); 
    bool isAcceptable(const MyopicUpgradePlan& plan, const State& state);

    // guessDemand return the random demand we guess from individuals in the future
    State guessInvDemand(const int period);

    // guessFutureDemand return the random demand we guess from individuals 
    // and travel agencies in the future, and also return the guessed price
    // of these rooms.
    std::tuple<State, std::map<data::tuple2d, double> > guessFutureDemand(const int period, const State& demand);
    
    // guessFutureDemandSP return the vector of random demands we guess in each scenarios from individuals 
    // and travel agencies in the future, and also return the guessed price
    // of these rooms
    void guessFutureDemandSP(const int period, const std::vector<State>& s_demands,
                             std::vector<State>& adj_demands, 
                             std::vector<std::map<data::tuple2d, double> >& adj_prices);
    
    // getAdjOrderPrices return ths adjustments order prices
    std::map<data::tuple2d, double> getAdjOrderPrices(const int period);
    // getAdjOrderdemands return ths adjustments order demand
    State getAdjOrderdemands(const int period);

    // getAdjAllDemandPrices return ths adjustments prices, which are weighted average of individual prices
    // and order prices, using demand quantities as weights.
    std::map<data::tuple2d, double> getAdjAllDemandPrices(const State& demand, const State& orders,
                                                          const std::map<data::tuple2d, double>& order_prices);

    // experiment perfor 1 experiment from booking period |T| to 1,
    // and return the revenue of this experiment.
    double experiment();

    double experiment(const std::map<data::tuple2d, int>& orders,
                      const std::map<data::tuple2d, State>& demands);

    // resetState reset the num of available rooms to the origin capacity
    void resetState();

    // setNumExpt set the number of experiments
    void setNumExpt(const int num);

    // setNumExpt set the number of sampling
    void setNumSample(const int num);

    // setThreshold set the threshold
    void setThreshold(const double thd);

    // randomOrder return the index of order arriving at this period
    int randomOrder(const int period);
    // randomOrder return the map of order arriving at each period
    std::map<int, int> randomOrder();

    // findOptPlanIP modify the given plan, considering the demand we guess. 
    // This problem is formulated in an integer programming problem in this function.
    void findOptPlanIP(const int period, MyopicUpgradePlan& plan, 
                       const State& state, const State& demand,
                       const std::map<data::tuple2d, double>& prices);

    // findOptPlanSP modify the given plan, considering the demand we guess. 
    // This problem is formulated in an stochastic programming problem in this function.
    void findOptPlanSP(const int period, MyopicUpgradePlan& plan, 
                       const State& state, const std::vector<State>& s_demands,
                       const std::vector<std::map<data::tuple2d, double> >& prices);

    // guessDemandSP return a vector of demand we guess in this period
    std::vector<State> guessDemandSP(const int period);

    // findBestRevOrder return the best revenue obtained from random order 
    double findBestRevOrder(const int period, const int order);

    // randomDemand return the random demand from individuals
    State randomDemand(const int period);

    // getIndPrices return the prices of individual customers given period
    std::map<data::tuple2d, double> getIndPrices(const int period);

    // getRevDemand return the revenue of random demand from individuals,
    // based on this->state
    double getRevDemand(const State& demand);

    // getRevDemand return the revenue of random demand from individuals,
    // based on given state.
    double getRevDemand(const State& demand, const State& state);
    double getRevDemand(const State& demand, const State& state, 
                        const std::map<data::tuple2d, double> prices);

    // isNoAvlRoom return if there are available rooms
    bool isNoAvlRoom();

    // storeGuessFuture store the guessed demand and price of each period
    // in order to make better decision whether we should accept the order
    // in each period.
    void storeGuessFuture();

    // isAllPosIndDemLess return true if the number of all possible individual
    // demand scenarios is less then this->num_exper_MS
    bool isAllPosIndDemLess();

    // allPosDemand return all possible individual demand 
    std::vector<State> allPosIndDemand(const int period);

    // getUpgradeFee return the upgrade fee of given upgrade plan
    double getUpgradeFee(const int order,
                         const std::map<data::tuple2d, int>& upgrade);
    
    // updateState update the given state with the given plan
    void updateState(State& state, const MyopicUpgradePlan& plan);

    // getReservedCap return the reserved capacities for future order.
    // This function is mainly for reserved mode.
    State getReservedCap(const int period, const State& state);

    // getExpDemand1P return the expected demand in the given single period.
    State getExpDemand1P(const int period);

    // findBestOrderNC store the best orders in mode naively conservative
    void findBestOrderNC();

    // storeCompoundedOrdersNC store the compounded orders in mode naively conservative
    void storeCompoundedOrdersNC();

    // orderToPlan return a plan instance of order
    MyopicUpgradePlan orderToPlan(const int order, const int num=1);

    // findOptPlanNC find the opt plan in NC
    MyopicUpgradePlan findOptPlanNC(const int period, const MyopicUpgradePlan& plan, 
                                    const State& state);
    
    // findBaselineUB find the baseline in bs mode
    double findBaselineBS(const int period, const MyopicUpgradePlan& plan, 
                          const State& state);
    
    // decideBS decide whether to accept this plan (upgraded order) or not.
    // It is only used in mode bs.
    void decideBS(MyopicUpgradePlan& plan, double baseline);
    

public:
    MyopicPlanner();
    MyopicPlanner(const data::Params& dat, MyopicMode m = MyopicMode::naive, 
                  MyopicModelType mt = MyopicModelType::ip);

    double run();
    double run(const int num);
    double run(const int num, const int num_sample);
    double run(const double threshold);
    double run(const int num, const double threshold);
    double run(const int num, const int early_stop, const double threshold);
    double run(const int num, const int num_sample,
               const int early_stop, const double threshold);

    void storeRevs(std::string path);
    std::map<int, double> getAvgRevs();

    void testGuessDemand();
    void testSmartGamma(const std::string& scenario);
    void testBSBeta(const std::string& scenario);
    void printAvgRevs();
    void test(const std::string& scenario);
    void testNC(const std::string& scenario);
    // funcTest test the target function
    void funcTest();
    void debug();
    void clearRecords();
    ~MyopicPlanner();
};

// END TODO 4/12

// firstKAverage return the average of first k elements in an array
double firstKAverage(const std::vector<double> arr, const int num);

// getExpValNB return the expected value of negative binomial
// distribution, given parameters k and p. 
double getExpValNB(const int k, const double p);

double getExpValGeo(const double p);
} // namespace planner


std::ostream& operator<<(std::ostream& os, const planner::State& state);
std::ostream& operator<<(std::ostream& os, const std::vector<planner::State>& state_list);
planner::State operator+(const planner::State& state1, const planner::State& state2);
planner::State operator-(const planner::State& state1, const planner::State& state2);

// forceMinus return state1 - state2, allow negative number
planner::State forceMinus(const planner::State& state1, const planner::State& state2);

#endif /* planner_hpp */
