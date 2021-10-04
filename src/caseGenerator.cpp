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
    this->upgrade_price = order.upgrade_price;
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

// generateOrder generate an order node 
Order OrderGenerator::generateOrder(const int period){
    Order order;

    // Generate before
        // int before = this->random(this->data_->prob_before);
    // Generate night
        // int night = this->random(this->data_->prob_night.at(before));
    // Compute check_in
        // int check_in = this->getCheckIn(before);
    // Compute checkout
        // int check_out = this->getCheckout(check_in, night);

    // Check if checkin and checkout is valid
        /* if(!this->data_->isValidDay(check_in) ||
              !this->data_->isValidDay(check_out)){
            order.is_order = false;
            return order;
        }*/
        // order.is_order = true;
    // Generate request days
        // order.request_days = this->getRequestDays(check_in, check_out);
    // Generate request num
        /* for r in room_type {
            order.request_rooms[r] = this->random(this->dat_->prob_request.at(r).at(before));   
        }
        */
    // Generate discount
        // double discount = this->random(this->data_->prob_discount.at(before));
    // Generate price
        // order.price = this->getPrice(this->request_days, discount);
    // Generate upgrade_fee
        // order.upgrade_fee = this->getUpgradeFee(discount);
    
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
       << "upgrade_price: " << order.upgrade_price << "\n";
    return os;
}