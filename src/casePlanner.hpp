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

// OrderGenerator generate random orders from travel agencies.
class OrderGenerator {

};

// IndDemandGenerator generate random demand from individual customers.
class IndDemandGenerator {

};

// Experimentor execute an experiment based on given data and random demands.
class Experimentor {

};

// 待決定是否需要這個
class MyopicCasePlanner {

};

}

#endif /* casePlanner_h */