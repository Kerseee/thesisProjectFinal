#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include <chrono>
#include <random>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"
#include "caseGenerator.hpp"
#include "caseStructures.hpp"
#include "../third_party/progressbar-master/include/progressbar.hpp"

namespace planner{

// ======================================================================
// ----------------------------- Generator ------------------------------
// ======================================================================

// Generate a random number from the given distribution.
// May return 0 if sum of probabilities from given distribution is not equal 1 
// input: dist[value] = probability, seed = an unsigned number
int Generator::random(const std::map<int, double>& dist, const unsigned seed){
    
    // Random an probability from uniform distribution in [0, 1]
    std::default_random_engine generator (seed);
    std::uniform_real_distribution<double> distribution (0.0, 1.0);
    double prob = distribution(generator);

    // Return the part that prob locates in.
    double cumulator = 0;
    for(auto& d: dist){
        cumulator += d.second;
        if(prob <= cumulator) return d.first;
    }

    // If not find any match then return 0
    return 0;
}

unsigned Generator::genSeedFromClock(){
    return std::chrono::system_clock::now().time_since_epoch().count();
}

// ======================================================================
// --------------------------- OrderGenerator ---------------------------
// ======================================================================

OrderGenerator::OrderGenerator(){
    ;
}
OrderGenerator::OrderGenerator(const data::CaseData& data){
    this->data = &data;
}

void OrderGenerator::testFunc(){
    std::cout << "\nInput period and before:\n";
    int period = 0, before = 0;
    std::cin >> period >> before;
    std::cout << "check-in: " << this->getCheckIn(period, before);
};

// Print the data in this OrderGenerator
void OrderGenerator::printData(){
    this->data->printAll();
}

// getCheckIn return the day of check-in given period and before
int OrderGenerator::getCheckIn(const int period, const int before){
    int booking_day = this->data->scale.getBookingDay(period);
    int check_in = this->data->scale.getServicePeriod(booking_day, before);
    return check_in;
}

// getCheckOut return the day of check-out given check-in and night
int OrderGenerator::getCheckOut(const int check_in, const int night){
    return check_in + night - 1;
}

// getRequestDays return set of requested days given check_in and check_out
std::set<int> OrderGenerator::getRequestDays(
    const int check_in, const int check_out
){
    std::set<int> request_days;
    for(int i = check_in; i <= check_out; i++){
        request_days.insert(i);
    }
    return request_days;
}

// getPrice return the total price of an order after discount
// Inputs: rooms[room_type] = num
double OrderGenerator::getPrice(
    const std::map<int, int>& rooms, const std::set<int>& days, 
    const double discount
){
    double price = 0;
    std::map<data::tuple2d, double>::const_iterator it;
    for(auto& r: rooms){
        for(auto& d: days){
            // Check if key exist
            it = this->data->price_ind.find({d, r.first});
            if(it == this->data->price_ind.end()){
                std::cout << "Error in OrderGenerator::getPrice: key{"
                    << r.first << "," << d << "} not exist!\n";
                exit(1);
            }
            price += it->second * r.second; 
        }
    }
    price *= discount;
    return price;
}

// getUpgradeFee return the upgrade fee of given discount
// Please note that upgrade fee may be negative if price of upper level room
// is lower than lower level room.
// Output: upgrade_fee[{from, to}] = price
std::map<data::tuple2d, double> OrderGenerator::getUpgradeFee(
    const std::set<int>& days, double discount
){
    std::map<data::tuple2d, double> upgrade_fee;
    std::map<data::tuple2d, double>::const_iterator it;
    std::string err_prefix = "Error in OrderGenerator::getUpgradeFee: ";
    std::string err_suffix = " not exist!\n";
    auto out_of_bound = this->data->price_ind.end();

    for(auto& pair: this->data->upgrade_pairs){
        int from = std::get<0>(pair), to = std::get<1>(pair);

        // Compute average price differences of rooms in given days
        double price_dif = 0;
        for(auto& day: days){
            // Get price of lower type room
            data::tuple2d key_from = std::make_tuple(day, from);
            it = this->data->price_ind.find(key_from);
            if(it == out_of_bound){
                std::cout << err_prefix << key_from << err_suffix;
                exit(1);
            }
            double price_from = it->second;

            // Get price of upper type room
            data::tuple2d key_to = std::make_tuple(day, to);
            it = this->data->price_ind.find(key_to);
            if(it == out_of_bound){
                std::cout << err_prefix << key_to << err_suffix;
                exit(1);
            }
            double price_to = it->second;

            // Compute price difference
            price_dif += price_to - price_from;
        }
        price_dif = (price_dif / days.size()) * discount;
        upgrade_fee[pair] = price_dif;
    }
    return upgrade_fee;
}

// generateOrder generate an order node 
Order OrderGenerator::generateOrder(const int period){
    Order order;
    if(!this->data->scale.isBookingPeriod(period)) return order;
    
    // For debug
    std::string err_prefix = "Error in OrderGenerator::generateOrder: ";
    std::string err_suffix = " not exist!\n";

    // Generate seed
    unsigned seed = this->genSeedFromClock();
    srand(seed);
    
    // Generate before
    int before = this->random(this->data->prob_before, rand());

    // Generate night
    auto it_night = this->data->prob_night.find(before);
    if(it_night == this->data->prob_night.end()){
        std::cout << err_prefix << "Before " << before << " in prob_night"
             << err_suffix;
        exit(1);
    }
    int night = this->random(it_night->second, rand());

    // Compute check_in day
    int check_in = this->getCheckIn(period, before);

    // Compute checkout
    int check_out = this->getCheckOut(check_in, night);

    // If checkin or checkout is not valid then return an empty order
    if(!this->data->scale.isServicePeriod(check_in) ||
       !this->data->scale.isServicePeriod(check_out)
    ){
        order.is_order = false;
        return order;
    }
    order.is_order = true;

    // Generate request days
    order.request_days = this->getRequestDays(check_in, check_out);

    // Generate request num
    for(int r = 1; r <= this->data->scale.room_type; r++) {
        // Check if key r and before exist
        auto it_room = this->data->prob_request.find(r);
        if(it_room == this->data->prob_request.end()){
            std::cout << err_prefix << "Room " << r << " in request" 
                << err_suffix;
            exit(1);
        }
        auto it_before = it_room->second.find(before);
        if(it_before == it_room->second.end()){
            std::cout << err_prefix << "Before " << before << " in request" 
                << err_suffix;
            exit(1);
        }
        order.request_rooms[r] = this->random(it_before->second, rand());
    }
        
    // Generate discount
    auto it_discount = this->data->prob_discount.find(before);
    if(it_discount == this->data->prob_discount.end()){
        std::cout << err_prefix << "Before " << before << " in discount" 
            << err_suffix;
            exit(1);
    }
    int discount_percentage = this->random(it_discount->second, rand());
    double discount = static_cast<double>(discount_percentage) / 100;

    // Generate price
    order.price = this->getPrice(
        order.request_rooms, order.request_days, discount
    );

    // Generate upgrade_fee
    order.upgrade_fees = this->getUpgradeFee(order.request_days, discount);
    
    return order;
}

// generate() generates a group of orders from travel agencies for the 
// whole booking stage for one experiment.
std::map<int, Order> OrderGenerator::generate(){
    std::map<int, Order> orders;
    for(int t = this->data->scale.booking_period; t > 0; t--){
        orders[t] = this->generateOrder(t);
    }
    return orders;
}

// generate(num_experiments) generates groups of orders from travel agencies
// for the whole booking stage for multiple experiments.
std::map<int, std::map<int, Order> > OrderGenerator::generate(const int num_experiments){
    std::map<int, std::map<int, Order> > order_groups;
    for(int e = 1; e <= num_experiments; e++){
        order_groups[e] = this->generate();
    }
    return order_groups;
}

// ======================================================================
// ------------------------- IndDemandGenerator -------------------------
// ======================================================================

IndDemandGenerator::IndDemandGenerator(){
    ;
}

IndDemandGenerator::IndDemandGenerator(const data::CaseData& data){
    this->data = &data;
}

// generateDemand generate demand, only store the pair of service_period
// and room that demand > 0 
std::map<data::tuple2d, int> 
IndDemandGenerator::generateDemand(const int period){
    std::map<data::tuple2d, int> demand;
    if(!this->data->scale.isBookingPeriod(period)) return demand;

    // For debug
    std::string err_prefix = "Error in IndDemandGenerator::GenerateDemand: ";
    std::string err_suffix = " not exist!\n";

    // Generate seed
    unsigned seed = this->genSeedFromClock();
    srand(seed);

    // Generate demands
    for(int r = 1; r <= this->data->scale.room_type; r++){
        for(int s = 1; s <= this->data->scale.service_period; s++){
            // Compute the true before to generate
            int booking_day = this->data->scale.getBookingDay(period);
            int before = this->data->scale.getBefore(booking_day, s);
            
            // Find the demand distribution with given room, service period
            auto it = this->data->prob_ind_demand.find(r);
            if(it == this->data->prob_ind_demand.end()){
                std::cout << err_prefix << "Room " << r << err_suffix;
                exit(1);
            }
            auto it_demand_at_r = it->second.find(before);

            // Generate random demand for room r on service period s
            // If distribution not found then let demand be 0.
            int num_room = 0;
            if(it_demand_at_r != it->second.end()){
                num_room = this->random(it_demand_at_r->second, rand());
            }
            if(num_room > 0){
                demand[{s, r}] = num_room;
            }
        }
    }
    return demand;
}

// generate() generates a group of individual demands for the whole 
// booking stage for one experiment.
std::map<int, std::map<data::tuple2d, int> > IndDemandGenerator::generate(){
    std::map<int, std::map<data::tuple2d, int> > demands;
    for(int t = this->data->scale.booking_period; t > 0; t--){
        demands[t] = this->generateDemand(t);
    }
    return demands;
}

// generate(num_experiments) generates groups of individual demands for the
// whole booking stage for multiple experiments.
std::map<int, std::map<int, std::map<data::tuple2d, int> > > 
IndDemandGenerator::generate(const int num_experiments){
    std::map<int, std::map<int, std::map<data::tuple2d, int> > > demand_groups;
    for(int e = 1; e <= num_experiments; e++){
        demand_groups[e] = this->generate();
    }
    return demand_groups;
}

// ======================================================================
// ---------------------- ExpectedDemandGenerator -----------------------
// ======================================================================

ExpectedDemandGenerator::ExpectedDemandGenerator(){
    ;
}

ExpectedDemandGenerator::ExpectedDemandGenerator(const data::CaseData& data){
    this->data = &data;
}

// generateExpDemand generate expected future demand given period
std::map<data::tuple2d, int> ExpectedDemandGenerator::generateExpDemand(
    const int period
){
    std::map<data::tuple2d, int> exp_demands;

    for(int r = 1; r <= this->data->scale.room_type; r++){
        auto it_room = this->data->prob_ind_demand.find(r);
        // Skip r if room type r is not found in prob_ind_demand
        if(it_room == this->data->prob_ind_demand.end()){
            continue;
        }
        for(int s = 1; s <= this->data->scale.service_period; s++){
            // Get the sum of expected value of demands from this period to the
            // end of booking stage
            double sum_expected_demands = 0;
            for(int t = period; t >= 1; t--){
                int booking_day = this->data->scale.getBookingDay(t);
                int before = this->data->scale.getBefore(booking_day, s);
                auto it_before = it_room->second.find(before);
                // Skip day s if day s is not found in prob_ind_demand[r]
                if(it_before == it_room->second.end()){
                    continue;
                }
                // Get the expected value of demand that request the service 
                // period s in the period t.
                double expected_demand = 0;
                for(auto& e: it_before->second){
                    expected_demand += e.first * e.second;
                }
                // Update the sum_expected demands
                sum_expected_demands += expected_demand;
            }
            int round_sum_exp_demand = round(sum_expected_demands);
            if(round_sum_exp_demand > 0){
                exp_demands[{s, r}] = round_sum_exp_demand;
            }
        }
    }
    return exp_demands;
}

// generate() generates expected future demands for all booking period
// in data
std::map<int, std::map<data::tuple2d, int> > 
ExpectedDemandGenerator::generate(){
    std::map<int, std::map<data::tuple2d, int> > exp_all_demands;
    progressbar bar(this->data->scale.booking_period);
    for(int t = this->data->scale.booking_period; t >= 1; t--){
        exp_all_demands[t] = this->generateExpDemand(t);
        bar.update();
    }
    return exp_all_demands;
}

// ======================================================================
// ---------------------- EstimatedDemandGenerator ----------------------
// ======================================================================

EstimatedDemandGenerator::EstimatedDemandGenerator(){
    ;
}
EstimatedDemandGenerator::EstimatedDemandGenerator(const data::CaseData& data){
    this->data = &data;
}

// generateEstDemand generate estimated future demand
std::map<data::tuple2d, int> 
EstimatedDemandGenerator::generateEstDemand(
    const int period, const unsigned seed
){
    srand(seed);
    std::map<data::tuple2d, int> demand;

    for(int r = 1; r <= this->data->scale.room_type; r++){
        auto it_room = this->data->prob_ind_demand.find(r);
        // Skip r if room type r is not found in prob_ind_demand
        if(it_room == this->data->prob_ind_demand.end()){
            continue;
        }
        for(int s = 1; s <= this->data->scale.service_period; s++){
            // Get the sum of sampling number of demands from this period to the
            // end of booking stage
            int sum_demands = 0;
            for(int t = period; t >= 1; t--){
                // Get before
                int booking_day = this->data->scale.getBookingDay(t);
                int before = this->data->scale.getBefore(booking_day, s);
                auto it_before = it_room->second.find(before);
                // Skip day s if day s is not found in prob_ind_demand[r]
                if(it_before == it_room->second.end()){
                    continue;
                }
                // Sampling the number of demand from the distribution
                int num = this->random(it_before->second, rand());
                // Update the sum_demands
                sum_demands += num;
            }

            if(sum_demands > 0){
                demand[{s, r}] = sum_demands;
            }
        }
    }
    return demand;
}

// genDemandScenarios generate estimated future demands given
// sample_size and period
std::vector<std::map<data::tuple2d, int> > 
EstimatedDemandGenerator::genDemandScenarios(
    const int sample_size, const int period
){
    unsigned seed = this->genSeedFromClock();
    srand(seed);
    std::vector<std::map<data::tuple2d, int> > demands;
    for(int i = 0; i < sample_size; i++){
        demands.push_back(this->generateEstDemand(period, rand()));
    }
    return demands;
}

// generate() generate estimated future demands for all booking
// periods in data, given sample_size
std::map<int, std::vector<std::map<data::tuple2d, int> > >
EstimatedDemandGenerator::generate(const int sample_size){
    std::map<int, std::vector<std::map<data::tuple2d, int> > > all_demands;
    progressbar bar(this->data->scale.booking_period); 
    for(int t = this->data->scale.booking_period; t >= 1; t--){
        all_demands[t] = this->genDemandScenarios(sample_size, t);
        bar.update();
    }
    return all_demands;
}



}   // End of namespace planner

