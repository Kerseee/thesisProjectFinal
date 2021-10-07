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

struct OrderDecision {
    const Order* order; // A copy of the order
    std::map<int, int> upgraded_request_rooms;
    double exp_acc_togo; // expected value togo if this order is accepted
    double exp_rej_togo; // expected value togo if this order is rejected
    double total_upgrade_fee; // upgrade fee of this order
    double revenue; // price + upgrade_fee if accepted, 0 otherwise
    bool accepted;  // 1 if thie order is accepted

    // upgrade information, key:{from, to}, value: num
    std::map<data::tuple2d, int> upgrade_info;

    // Constructors
    OrderDecision();
    OrderDecision(const Order& order);
    OrderDecision(const OrderDecision& od);
    OrderDecision& operator=(const OrderDecision& od);
};


class Hotel {
private:
    int service_period;
    int room_type;
    // prices[{service_period, room_type}] = price
    std::map<data::tuple2d, double> prices;
    // rooms[{service_period, room_type}] = num
    std::map<data::tuple2d, int> rooms;

    // upgrade_upper stores types that this type of room can upgrade to
    std::map<int, std::set<int> > upgrade_upper;
    
    // upgrade_upper stores types that can upgrade to this type
    std::map<int, std::set<int> > upgrade_lower;
    
    // upgrade_pairs store the upgrade_pairs
    std::set<data::tuple2d> upgrade_pairs;

public:
    Hotel();
    Hotel(const int service_period, const int room_type, 
          std::map<data::tuple2d, double> prices,
          std::map<data::tuple2d, int> rooms);
    Hotel(const data::CaseData& data);

    // print all information of this hotel
    void print();
    
    // hasRoom Check if there is any available room
    bool hasRooms();
    
    // canBook Check if given number of room type on service_period(day)
    // can be accpeted
    bool canBook(const int day, const int room_type, const int num);

    // booking return false if there is any booking error
    bool booking(const int day, const int room_type, const int num);

    // forceBooking book the room without any check! Call this function only
    // when you call hotel.canBook() for every room you want to book, or
    // there will be some errors.
    void forceBooking(const int day, const int room_type, const int num);
    
    // getMinCapInPeriods return the min capacity of room type during 
    // given periods.
    int getMinCapInPeriods(const int room_type, const std::set<int>& periods);
    
    // getAllMinCapInPeriods return the min capacity of all room types during 
    // given periods.
    std::map<int, int> getAllMinCapInPeriods(const std::set<int>& periods);
    
    // Return number of room type of this hotel
    int getNumRoomType();

    // Return number of service period of this hotel
    int getNumServicePeriod();

    // Return the available rooms of given service_period (day) and room_type
    int getNumAvailableRooms(const int day, const int room_type);

    // Return the rooms of this hotel
    std::map<data::tuple2d, int> getRooms();

    // Return the price of given service_period(day) and room_type, return 0
    // if either day or room_type not exist.
    double getPrice(const int day, const int room_type);

    // Return the upper types that given room_type can upgrade to
    std::set<int> getUpgradeUpper(const int room_type);
};

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