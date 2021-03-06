#ifndef caseStructures_hpp
#define caseStructures_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"

namespace planner{

// Order stores information of an order from a travel agency.
struct Order {
    bool is_order;  // 0 for empty order, 1 for general order
    double price;
    std::set<int> request_days; // set of days in service period
    std::map<int, int> request_rooms;   // request_rooms[room_type] = num
    // upgrade_fees[{lower_room_type, upper_room_type}] = price
    std::map<data::tuple2d, double> upgrade_fees;

    Order();
    Order(const Order& order);
    Order& operator=(const Order& order);
};

struct OrderDecision {
    const Order* order; // A copy of the order
    std::map<int, int> upgraded_request_rooms;
    double exp_acc_togo; // expected value togo if this order is accepted
    double exp_rej_togo; // expected value togo if this order is rejected
    double total_upgrade_fee; // upgrade fee of this order
    double revenue; // price + upgrade_fee
    bool acceptable;    // 1 if this order is acceptable
    bool accepted;  // 1 if this order is accepted

    // upgrade information, key:{from, to}, value: num
    std::map<data::tuple2d, int> upgrade_info;

    // Constructors
    OrderDecision();
    OrderDecision(const Order& order);
    OrderDecision(const OrderDecision& od);
    OrderDecision& operator=(const OrderDecision& od);

    // computeUpgradeFee compute the total upgrade fee with this->order and
    // this.upgrade_info
    void computeUpgradeFee();
    // computeUpgradedRooms compute the number of rooms for each room type
    // after upgraded, with this->order and this.upgrade_info
    void computeUpgradedRooms();
    // refreshUpgradeInfo() refresh the upgrade fee and upgrade rooms and 
    // revenue with this->upgrade_info
    void refreshUpgradeInfo();
};

struct ExperimentorResult {
    int num_periods;
    int stop_period;    // store period that out of capacity
    // store decision of each period
    std::map<int, OrderDecision> decisions; 
    // store accepted demand of each period
    std::map<int, std::map<data::tuple2d, int> > accepted_demands;
    // store revenue from order in each period
    std::map<int, double> order_revenues;
    // store revenue from individual demands in each period
    std::map<int, double> ind_demand_revenues;
    double revenue; // store the total revenue of one experiment
    // store the total time of running experiment, not including preparing time
    double runtime; 

    ExperimentorResult();
    ExperimentorResult(const int num_periods);
    ExperimentorResult(const ExperimentorResult& result);
    ExperimentorResult& operator=(const ExperimentorResult& result);
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
    Hotel(const Hotel& hotel);
    Hotel& operator=(const Hotel& hotel);

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

    // Return the lower types that can be upgraded to this room_type
    std::set<int> getUpgradeLower(const int room_type);

    // Return the upgrade pairs
    std::set<data::tuple2d> getUpgradePairs();

    // Return the copy of remain available room after accept given an OrderDecision
    std::map<data::tuple2d, int> showTryBooking(const OrderDecision& od);
};

}

std::ostream& operator<<(std::ostream& os, const planner::Order& order);
std::ostream& operator<<(std::ostream& os, const planner::OrderDecision& od);

#endif /* caseStructures_hpp */
