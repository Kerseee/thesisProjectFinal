#include <iostream>
#include <stdio.h>
#include <fstream>
#include <map>
#include <tuple>
#include <vector>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"
#include "casePlanner.hpp"

namespace planner {

// ======================================================================
// --------------------------- OrderDecision ----------------------------
// ======================================================================

OrderDecision::OrderDecision(){
    this->order = nullptr;
    this->accepted = false;
    this->exp_acc_togo = 0;
    this->exp_rej_togo = 0;
    this->revenue = 0;
    this->total_upgrade_fee = 0;
}
OrderDecision::OrderDecision(const Order& order){
    this->order = &order;
    this->accepted = false;
    this->exp_acc_togo = 0;
    this->exp_rej_togo = 0;
    this->revenue = 0;
    this->total_upgrade_fee = 0;
}
OrderDecision::OrderDecision(const OrderDecision& od){
    this->operator=(od);
}
OrderDecision& OrderDecision::operator=(const OrderDecision& od){
    this->order = od.order;
    this->accepted = od.accepted;
    this->exp_acc_togo = od.exp_acc_togo;
    this->exp_rej_togo = od.exp_rej_togo;
    this->revenue = od.revenue;
    this->total_upgrade_fee = od.total_upgrade_fee;
    this->upgrade_info = od.upgrade_info;
    this->upgraded_request_rooms = od.upgraded_request_rooms;
    return *this;
}

// ======================================================================
// ------------------------------- Hotel --------------------------------
// ======================================================================

Hotel::Hotel(){
    this->service_period = 0;
    this->room_type = 0;
}

Hotel::Hotel(
    const int service_period, const int room_type, 
    std::map<data::tuple2d, double> prices, std::map<data::tuple2d, int> rooms
){
    this->service_period = service_period;
    this->room_type = room_type;
    this->prices = prices;
    this->rooms = rooms;
}

Hotel::Hotel(const data::CaseData& data){
    this->service_period = data.scale.service_period;
    this->room_type = data.scale.room_type;
    // Store price and rooms
    for(int r = 1; r <= this->room_type; r++){
        // Get room
        auto it_room = data.capacity.find(r);
        int room = 0;
        if(it_room != data.capacity.end()){
            room = it_room->second;
        }
        for(int s = 1; s <= this->service_period; s++){
            // Get price
            auto it_price = data.price_ind.find({s, r});
            double price = 0;
            if(it_price != data.price_ind.end()){
                price = it_price->second;
            }
            
            this->prices[{s, r}] = price;
            this->rooms[{s, r}] = room;
        }
    }
}

// print all information of this hotel
void Hotel::print(){
    std::cout << "Print Hotel Info:"
        << "------------------------------------------------------------\n"
        << "service peirods: " << this->service_period << "\n"
        << "room types: " << this->room_type << "\n"
        << "prices: " << this->prices<< "\n"
        << "rooms: " << this->rooms<< "\n"
        << "------------------------------------------------------------\n";
}

// hasRoom Check if there is any available room
bool Hotel::hasRooms(){
    for(auto& room: this->rooms){
        if(room.second > 0) return true;
    }
    return false;
}

// booking return false if there is any booking error
bool Hotel::booking(const int room_type, const int day, const int num){
    auto it = this->rooms.find({day, room_type});
    // Return False if key {day, room_type} not exist
    if(it == this->rooms.end()) return false;
    // Return False requested number exceed available number of rooms
    if(num > it->second) return false;
    
    // Book the room and return true
    this->rooms[it->first] -= num;
    return true;
}

// getMinCapInPeriods return the min capacity of room type during 
// given periods.
int Hotel::getMinCapInPeriods(
    const int room_type, const std::set<int>& periods
){
    int min_cap = INT_MAX;
    bool valid = false;
    for(auto& s: periods){
        auto it = this->rooms.find({s, room_type});
        // Ignore following if the key is not found
        if(it == this->rooms.end()) continue;
        
        // This search is valid if any key exist
        valid = true;
        // Find the min capacity
        int cap = it->second;
        if(cap < min_cap){
            min_cap = cap;
        }
    }
    if(!valid) return 0;
    return min_cap;
}
// getAllMinCapInPeriods return the min capacity of all room types during 
// given periods.
std::map<int, int> Hotel::getAllMinCapInPeriods(const std::set<int>& periods){
    std::map<int, int> min_caps;
    for(int r = 1; r <= this->room_type; r++){
        min_caps[r] = this->getMinCapInPeriods(r, periods);
    }
    return min_caps;
}


// // run1Period run the period routine in given period, with given the random
// // order and demand, and then return the total revenue in this period.
// void Experimentor::run1Period(
//     const int period, const Order& order, const State& ind_demand
// ){
//     double total_rev = 0;
    
//     // Check if the order is acceptable
//     if(this->isAcceptable(order)){
//         // Find the best decision of this order
//         OrderDecision decision = this->findBestRevOrder(period, order);
//         // Book the rooms of this order and get revenue
//         double rev_order = this->booking(decision);
//         total_rev += rev_order;
//     }

//     // Find the acceptable demands
//     std::map<data::tuple2d, int> acceptable_demands = 
//         this->getAcceptedIndDemand(ind_demand);
//     // Book the rooms of these demand and get revenue
//     double rev_ind_demand = this->booking(acceptable_demands);
//     total_rev += rev_ind_demand;

//     return total_rev;
// }

// // isAcceptable check if this order is acceptable with or without upgrade    
// bool Experimentor::isAcceptable(const Order& order){
//     // If given order is an empty order then return false
//     if(!order.is_order) return false;

//     // // Find bottleneck of capacity during the request days

//     // std::map<int, int> aval_rooms = 
//     //     this->hotel.getAllMinCapInPeriods(order.request_days);
//     // // Check if this order is acceptable considering upgrade

//     // int sum_req = 0;
//     // for(int i = State::num_room_type; i >= 1; i--){
//     //     int max_aval_rooms = aval_rooms[i];
//     //     for(auto& upgradeI: this->dat_->upgrade_to.at(i)){
//     //         max_aval_rooms += aval_rooms.at(upgradeI);
//     //     }
//     //     sum_req += this->dat_->req_rooms.at({order, i});
//     //     if(sum_req > max_aval_rooms){
//     //         return false;
//     //     }
//     // }
//     return true;
// }

// // getAcceptedIndDemand return the actual acceptable individual demand
// std::map<data::tuple2d, int> Experimentor::getAcceptedIndDemand(
//     const State& ind_demand
// ){
//     std::map<data::tuple2d, int> acc_demand;
//     // Store all the bookable number of rooms.
    
//     return acc_demand; 
// }

// // booking book the hotel rooms and return the revenue
// double Experimentor::booking(const OrderDecision& decision){
//     double revenue = 0;
//     // Run through all the upgraded requsted rooms
//     // And call this->hotel.booking()
    
//     return revenue;
// }
// double Experimentor::booking(
//     const std::map<data::tuple2d, int>& demand
// ){
//     double revenue = 0;
//     // Run through all the demand in each period and rooms
//     // And call this->hotel.booking()

//     return revenue;
// }



}