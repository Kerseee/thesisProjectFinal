#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include <chrono>
#include <random>
#include "data.hpp"
#include "caseData.hpp"
#include "planner.hpp"
#include "caseGenerator.hpp"

namespace planner{

Order::Order() {
    this->is_order = false;
    this->price = 0;
}

Order::Order(const Order& order){
    this->operator=(order);
}

Order& Order::operator=(const Order& order){
    this->is_order = order.is_order;
    this->price = order.price;
    this->request_days = order.request_days;
    this->request_rooms = order.request_rooms;
    this->upgrade_fees = order.upgrade_fees;
    return *this;
}

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

OrderGenerator::OrderGenerator(){
    ;
}
OrderGenerator::OrderGenerator(const data::CaseData& data){
    this->data_ = &data;
}

void OrderGenerator::testFunc(){
    std::cout << "\nInput period and before:\n";
    int period = 0, before = 0;
    std::cin >> period >> before;
    std::cout << "check-in: " << this->getCheckIn(period, before);
};

// Print the data in this OrderGenerator
void OrderGenerator::printData(){
    this->data_->printAll();
}

// getCheckIn return the day of check-in given period and before
int OrderGenerator::getCheckIn(const int period, const int before){
    // Check if period is in booking_period_day
    std::map<int, int>::const_iterator it;
    it = this->data_->scale.booking_period_day.find(period);
    if(it == this->data_->scale.booking_period_day.end()){
        std::cout << "Error in OrderGenerator::getCheckIn: Period" 
            << period << " not exist!\n";
        exit(1);
    }

    int booking_day = it->second;
    return before - booking_day + 1;
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
            it = this->data_->price_ind.find({d, r.first});
            if(it == this->data_->price_ind.end()){
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
    auto out_of_bound = this->data_->price_ind.end();

    for(auto& pair: this->data_->upgrade_pairs){
        int from = std::get<0>(pair), to = std::get<1>(pair);

        // Compute average price differences of rooms in given days
        double price_dif = 0;
        for(auto& day: days){
            // Get price of lower type room
            data::tuple2d key_from = std::make_tuple(day, from);
            it = this->data_->price_ind.find(key_from);
            if(it == out_of_bound){
                std::cout << err_prefix << key_from << err_suffix;
                exit(1);
            }
            double price_from = it->second;

            // Get price of upper type room
            data::tuple2d key_to = std::make_tuple(day, to);
            it = this->data_->price_ind.find(key_to);
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
    
    // For debug
    std::string err_prefix = "Error in OrderGenerator::generateOrder: ";
    std::string err_suffix = " not exist!\n";

    // Generate seed
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    srand(seed);
    
    // Generate before
    int before = this->random(this->data_->prob_before, rand());

    // Generate night
    auto it_night = this->data_->prob_night.find(before);
    if(it_night == this->data_->prob_night.end()){
        std::cout << err_prefix << "Before " << before << " in prob_night"
             << err_suffix;
        exit(1);
    }
    int night = this->random(it_night->second, rand());

    // Compute check_in day
    int check_in = this->getCheckIn(period, before);

    // Compute checkout
    int check_out = this->getCheckOut(check_in, night);

    // Check if checkin and checkout is valid
    if(!this->data_->scale.isServiceDay(check_in) ||
       !this->data_->scale.isServiceDay(check_out)
    ){
        order.is_order = false;
        return order;
    }
    order.is_order = true;

    // Generate request days
    order.request_days = this->getRequestDays(check_in, check_out);

    // Generate request num
    for(int r = 1; r <= this->data_->scale.room_type; r++) {
        // Check if key r and before exist
        auto it_room = this->data_->prob_request.find(r);
        if(it_room == this->data_->prob_request.end()){
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
    auto it_discount = this->data_->prob_discount.find(before);
    if(it_discount == this->data_->prob_discount.end()){
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
    for(int t = this->data_->scale.booking_period; t > 0; t--){
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


// generateDemand generate an demand node 
State IndDemandGenerator::generateDemand(const int period){
    State state;

    for(int r = 1; r <= this->data_->scale.room_type; r++){
        for(int s = 1; s <= this->data_->scale.service_period; s++){

            // Compute the true before to generate
                // int before = this->getBefore(period, s);
            // Generate demand
                // state.rooms[s, r] = this->random(
                //     this->data_->prob_ind_demand.at(r).at(before));
            
        }
    }
    return state;
}
// generate() generates a group of individual demands for the whole 
// booking stage for one experiment.
std::map<int, State> IndDemandGenerator::generate(){
    std::map<int, State> demands;
    for(int t = this->data_->scale.booking_period; t > 0; t--){
        demands[t] = this->generateDemand(t);
    }
    return demands;
}
// generate(num_experiments) generates groups of individual demands for the
// whole booking stage for multiple experiments.
std::map<int, std::map<int, State> > 
IndDemandGenerator::generate(const int num_experiments){
    std::map<int, std::map<int, State> > demand_groups;
    for(int e = 1; e <= num_experiments; e++){
        demand_groups[e] = this->generate();
    }
    return demand_groups;
}

}   
// End of namespace planner

std::ostream& operator<<(std::ostream& os, const planner::Order& order){
    os << "is_order: " << order.is_order << "\n"
       << "price: " << order.price << "\n"
       << "request_days: " << order.request_days << "\n"
       << "request_rooms: " << order.request_rooms << "\n"
       << "upgrade_price: " << order.upgrade_fees << "\n";
    return os;
}