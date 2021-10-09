#ifndef caseGenerator_hpp
#define caseGenerator_hpp

#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"
#include "caseStructures.hpp"
// #include "third_party/json-develop/single_include/nlohmann/json.hpp"

namespace planner{

class Generator {
private:
    int records;
public:
    // Generate a random number from the given distribution.
    // May return 0 if sum of probabilities from given distribution is not equal 1 
    // input: dist[value] = probability, seed = an unsigned number
    int random(const std::map<int, double>& dist, const unsigned seed);
    // genSeedFromClock return the seed from system clock
    unsigned genSeedFromClock();
};


// OrderGenerator generate random orders from travel agencies.
class OrderGenerator: public virtual Generator {
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
class IndDemandGenerator: public virtual Generator {
private:
    const data::CaseData* data;

public:
    IndDemandGenerator();
    IndDemandGenerator(const data::CaseData& data);
    
    // generateDemand generate demand, only store the pair of service_period
    // and room that demand > 0 
    std::map<data::tuple2d, int> generateDemand(const int period);
    
    // generate() generates a group of individual demands for the whole 
    // booking stage for one experiment.
    std::map<int, std::map<data::tuple2d, int> > generate();
    
    // generate(num_experiments) generates groups of individual demands for the
    // whole booking stage for multiple experiments.
    std::map<int, std::map<int, std::map<data::tuple2d, int> > > generate(
        const int num_experiments);
};

// ExpectedDemandGenerator generate expected future demands from individual
// customers
class ExpectedDemandGenerator: public virtual Generator {
private:
    const data::CaseData* data;

public:
    ExpectedDemandGenerator();
    ExpectedDemandGenerator(const data::CaseData& data);

    // generateExpDemand generate expected future demand given period
    std::map<data::tuple2d, int> generateExpDemand(const int period);

    // generate() generates expected future demands for all booking period
    // in data
    std::map<int, std::map<data::tuple2d, int> > generate();
};

// ExpectedDemandGenerator generate estimated future demands from individual
// customers
class EstimatedDemandGenerator: public virtual Generator {
private:
    const data::CaseData* data;
public:
    EstimatedDemandGenerator();
    EstimatedDemandGenerator(const data::CaseData& data);

    // generateEstDemand generate estimated future demand
    std::map<data::tuple2d, int> generateEstDemand(
        const int period, const unsigned seed);
    
    // genDemandScenarios generate estimated future demands given
    // sample_size and period
    std::vector<std::map<data::tuple2d, int> > genDemandScenarios(
        const int sample_size, const int period);
    
    // generate() generate estimated future demands for all booking
    // periods in data, given sample_size
    std::map<int, std::vector<std::map<data::tuple2d, int> > > generate(
        const int sample_size);
};

}



#endif /* caseGenerator_hpp */