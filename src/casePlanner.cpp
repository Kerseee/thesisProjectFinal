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
    // Store upgrade info
    this->upgrade_upper = data.upgrade_upper;
    this->upgrade_lower = data.upgrade_lower;
    this->upgrade_pairs = data.upgrade_pairs;
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

// canBook Check if given number of room type on service_period(day)
// can be accpeted
bool Hotel::canBook(const int day, const int room_type, const int num){
    auto it = this->rooms.find({day, room_type});
    // Return False if key {day, room_type} not exist
    if(it == this->rooms.end()) return false;
    // Return False requested number exceed available number of rooms
    if(num > it->second) return false;
    return true;
}

// booking return false if there is any booking error
bool Hotel::booking(const int day, const int room_type, const int num){
    // Check if this booking is valid
    if(!this->canBook(day, room_type, num)) return false;
    
    // Book the room and return true.
    this->forceBooking(day, room_type, num);
    return true;
}

// forceBooking book the room without any check! Call this function only
// when you call hotel.canBook() for every room you want to book, or
// there will be some errors.
void Hotel::forceBooking(const int day, const int room_type, const int num){
    auto it = this->rooms.find({day, room_type});
    this->rooms[it->first] -= num;
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

// Return number of room type of this hotel
int Hotel::getNumRoomType(){
    return this->room_type;
}
// Return number of service period of this hotel
int Hotel::getNumServicePeriod(){
    return this->service_period;
}

// Return the available rooms of given service_period (day) and room_type
int Hotel::getNumAvailableRooms(const int day, const int room_type){
    auto it = this->rooms.find({day, room_type});
    // Return 0 if day or room type not found in this hotel
    if(it == this->rooms.end()) return 0;
    return it->second;
}

// Return the rooms of this hotel
std::map<data::tuple2d, int> Hotel::getRooms(){
    return this->rooms;
}

// Return the price of given service_period(day) and room_type, return 0
// if either day or room_type not exist.
double Hotel::getPrice(const int day, const int room_type){
    auto it = this->prices.find({day, room_type});
    if(it == this->prices.end()) return 0;
    return it->second;
}

// Return the upper types that given room_type can upgrade to
std::set<int> Hotel::getUpgradeUpper(const int room_type){
    auto it = this->upgrade_upper.find(room_type);
    if(it == this->upgrade_upper.end()) return std::set<int>();
    return it->second;
}

// ======================================================================
// ---------------------------- Experimentor ----------------------------
// ======================================================================


// run1Period run the period routine in given period, with given the random
// order and demand, and then return the total revenue in this period.
double Experimentor::run1Period(
    const int period, const Order& order, 
    const std::map<data::tuple2d, int>& ind_demand
){
    double total_rev = 0;
    
    // Check if the order is acceptable
    if(this->isAcceptable(order)){
        // Find the best decision of this order
        OrderDecision decision = this->findBestOrderDecision(period, order);
        // Book the rooms of this order
        bool success = this->booking(decision);
        // Handel booking error
        if(!success){
            std::cout << "Error in run1Period: Booking error at period "
                << period << " with decision info:\n";
                // << decision;
            exit(1);
        }
        // Get the revenue of this decision
        total_rev += decision.revenue;
    }

    // Find the acceptable demands
    std::map<data::tuple2d, int> acc_demands = 
        this->getAcceptedIndDemand(ind_demand);
    // Book the rooms of these demand and get revenue
    bool success = this->booking(acc_demands);
    // Handel booking error
    if(!success){
        std::cout << "Error in run1Period: Booking error at period "
            << period << " with ind demand: " << acc_demands << "\n";
        exit(1);
    }
    // Get the revenue of this ind_demand
    double rev_ind_demand = this->getRevIndDemand(acc_demands);
    total_rev += rev_ind_demand;

    return total_rev;
}

// isAcceptable check if this order is acceptable with or without upgrade    
bool Experimentor::isAcceptable(const Order& order){
    // If given order is an empty order then return false
    if(!order.is_order) return false;

    // Find bottleneck of capacity during the request days
    std::map<int, int> min_caps = 
        this->hotel->getAllMinCapInPeriods(order.request_days);
    // Check if this order is acceptable considering upgrade

    int sum_req = 0;
    for(int r = this->hotel->getNumRoomType(); r >= 1; r--){
        // Get the maximum arrangable rooms of types that equals or upper then
        // type r.
        // If there are any type that not exist then put 0.
        int max_aval_rooms = min_caps[r];
        std::set<int> upgrade_upper = this->hotel->getUpgradeUpper(r);
        for(auto& upper: upgrade_upper){
            max_aval_rooms += min_caps[upper];
        }
        // Get the number of request room equals or upper then type r
        // in this order.
        // If the room type is not found in request_rooms then put 0.
        auto it_req_room = order.request_rooms.find(r);
        if(it_req_room != order.request_rooms.end()){
            sum_req += it_req_room->second;
        }
        if(sum_req > max_aval_rooms){
            return false;
        }
    }
    return true;
}

// getAcceptedIndDemand return the actual acceptable individual demand
std::map<data::tuple2d, int> Experimentor::getAcceptedIndDemand(
    const std::map<data::tuple2d, int>& ind_demand
){
    return this->getAcceptedIndDemand(ind_demand, this->hotel->getRooms()); 
}

// getAcceptedIndDemand return the actual acceptable individual demand
// given available rooms
std::map<data::tuple2d, int> Experimentor::getAcceptedIndDemand(
    const std::map<data::tuple2d, int>& ind_demand,
    const std::map<data::tuple2d, int>& available_rooms
){
    std::map<data::tuple2d, int> acc_demand;
    // Store all the bookable number of rooms.
    for(auto& demand: ind_demand){
        // Get the request number of demand
        int num_demand = demand.second;
        // Get the available number of room, if the key is not found in 
        // available_rooms, then put 0 to available_room.
        int available_room = 0;
        auto it_rooms = available_rooms.find(demand.first);
        if(it_rooms != available_rooms.end()){
            available_room = it_rooms->second;
        }
        // Store the min between request and available room into acc_demand
        acc_demand[demand.first] = std::min(num_demand, available_room);
    }
    return acc_demand;
}

// getRevIndDemand return the total price of given demand
double Experimentor::getRevIndDemand(std::map<data::tuple2d, int> acc_demand){
    double revenue = 0;
    for(auto& demand: acc_demand){
        int day = std::get<0>(demand.first), room = std::get<1>(demand.first);
        double price = this->hotel->getPrice(day, room);
        revenue += static_cast<double>(demand.second) * price;
    }
    return revenue;
}

// booking book the hotel rooms and return the revenue
bool Experimentor::booking(const OrderDecision& decision){
    // If this order is an empty order, then don't book and don't report error.
    if(!decision.order->is_order) return true;
    // If this order is not accepted, then don't book and don't report error.
    if(!decision.accepted) return true;
    // Return false and don't book any room if there is any invalid booking.
    for(auto& day: decision.order->request_days){
        for(auto& room: decision.upgraded_request_rooms){
            if(!this->hotel->canBook(day, room.first, room.second)) return false;
        }
    }

    // Book this order
    for(auto& day: decision.order->request_days){
        for(auto& room: decision.upgraded_request_rooms){
            this->hotel->forceBooking(day, room.first, room.second);
        }
    }
    return true;
}

bool Experimentor::booking(
    const std::map<data::tuple2d, int>& ind_demand
){
    // Check if there is any invalid booking
    for(auto& demand: ind_demand){
        int day = std::get<0>(demand.first), room = std::get<1>(demand.first);
        int num = demand.second;
        if(!this->hotel->canBook(day, room, num)) return false;
    }
    // Book the room
    for(auto& demand: ind_demand){
        int day = std::get<0>(demand.first), room = std::get<1>(demand.first);
        int num = demand.second;
        this->hotel->forceBooking(day, room, num);
    }
    return true;
}

}