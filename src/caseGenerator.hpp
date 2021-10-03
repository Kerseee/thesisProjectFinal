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
    std::map<data::tuple2d, double> upgrade_price;

    Order();
    Order(const Order& order);
    Order& operator=(const Order& order);
};

// TODO 10/3
class Generator {
public:
    // Generate a random number from the given distribution.
    // input: dist[value] = probability
    int random(const std::map<int, double>& dist);
};

// OrderGenerator generate random orders from travel agencies.
class OrderGenerator: public Generator {
private:
    /* Variables */
    const data::CaseData* data_;

    /* Private methods */
    // TODO 10/3
    int getCheckIn(const int before);
    int getCheckOut(const int check_in, const int night);
    std::set<int> getRequestDays(const int check_in, const int check_out);
    double getPrice(std::set<int> days, double discount);
    std::map<data::tuple2d, double> getUpgradeFee(double discount);

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
};

// IndDemandGenerator generate random demand from individual customers.
class IndDemandGenerator: public Generator {
private:
    const data::CaseData* data_;

    // TODO
    // getBefore return "before" given booking and service period.
    int getBefore(const int booking_period, const int service_period);

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