#ifndef caseGenerator_hpp
#define caseGenerator_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "caseData.hpp"
#include "planner.hpp"

namespace planner{

// Order stores information of an order from a travel agency.
struct Order {
    bool is_order;
    double price;
    std::set<int> request_days;
    std::map<int, int> request_rooms;
    std::map<data::tuple2d, double> upgrade_fees;

    Order();
    Order(const Order& order);
    Order& operator=(const Order& order);
};

class Generator {
public:
    // Generate a random number from the given distribution.
    // May return 0 if sum of probabilities from given distribution is not equal 1 
    // input: dist[value] = probability, seed = an unsigned number
    int random(const std::map<int, double>& dist, const unsigned seed);
};


// OrderGenerator generate random orders from travel agencies.
class OrderGenerator: public Generator {
private:
    /* Variables */
    const data::CaseData* data;

    /* Private methods */
    // getCheckIn return the day of check-in given period and before
    int getCheckIn(const int period, const int before);

    // getCheckOut return the day of check-out given check-in and night
    int getCheckOut(const int check_in, const int night);

    // getRequestDays return set of requested days given check_in and check_out
    std::set<int> getRequestDays(const int check_in, const int check_out);

    // getPrice return the total price of an order after discount
    // Inputs: rooms[room_type] = num
    double getPrice(const std::map<int, int>& rooms,
        const std::set<int>& days, const double discount);
    
    // getUpgradeFee return the upgrade fee of given discount
    // Output: upgrade_fee[{from, to}] = price
    // Please note that upgrade fee may be negative if price of upper level room
    // is lower than lower level room.
    std::map<data::tuple2d, double> getUpgradeFee(
        const std::set<int>& days, double discount);

public:
    OrderGenerator();
    OrderGenerator(const data::CaseData& data);

    // generateOrder generate an order node 
    Order generateOrder(const int period);
    // generate() generates a group of orders from travel agencies for the 
    // whole booking stage for one experiment.
    std::map<int, Order> generate();
    // generate(num_experiments) generates groups of orders from travel agencies
    // for the whole booking stage for multiple experiments.
    std::map<int, std::map<int, Order> > generate(const int num_experiments);
    // Print the data in this OrderGenerator
    void printData();

    // For debug 
    void testFunc();
};

// IndDemandGenerator generate random demand from individual customers.
class IndDemandGenerator: public Generator {
private:
    const data::CaseData* data;

public:
    IndDemandGenerator();
    IndDemandGenerator(const data::CaseData& data);
    
    // generateDemand generate an demand node 
    State generateDemand(const int period);
    // generate() generates a group of individual demands for the whole 
    // booking stage for one experiment.
    std::map<int, State> generate();
    // generate(num_experiments) generates groups of individual demands for the
    // whole booking stage for multiple experiments.
    std::map<int, std::map<int, State> > generate(const int num_experiments);
};

}

std::ostream& operator<<(std::ostream& os, const planner::Order& order);

#endif /* caseGenerator_hpp */