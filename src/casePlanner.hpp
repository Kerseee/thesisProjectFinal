#ifndef planner_hpp
#define planner_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "caseData.hpp"
#include "planner.hpp"

namespace planner {

// Order stores information of an order from a travel agency.
struct Order {
    int id;
    double price;
    std::set<int> request_days;
    std::map<int, int> request_rooms;
    std::map<data::tuple2d, double> upgrade_price;
};

class Generator {
public:
    // Generate a random number from the given distribution.
    // input: dist[value] = probability
    int random(std::map<int, double> dist);
};

// OrderGenerator generate random orders from travel agencies.
class OrderGenerator: public Generator {
private:
    /* Variables */
    const data::CaseData* data_;

    /* Private methods */
    int randomBefore();
    int randomNight(const int before);
    double randomDiscount(const int before);
    int randomNumRooms(const int before);
    double getPrice(const double discount);

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
    std::map<int, std::map<int, State> > generate(const int num_experiments);
};

// IndDemandGenerator generate random demand from individual customers.
class IndDemandGenerator {
private:
    const data::CaseData* data_;
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

// Experimentor execute an experiment based on given data and random demands.
class Experimentor {

};

// 待決定是否需要這個
class MyopicCasePlanner {

};

}

#endif /* casePlanner_h */