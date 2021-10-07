#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"
#include "caseStructures.hpp"

namespace planner{

// ======================================================================
// ------------------------------- Order --------------------------------
// ======================================================================
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

} // End of namespace planner

std::ostream& operator<<(std::ostream& os, const planner::Order& order){
    os << "is_order: " << order.is_order << "\n"
       << "price: " << order.price << "\n"
       << "request_days: " << order.request_days << "\n"
       << "request_rooms: " << order.request_rooms << "\n"
       << "upgrade_price: " << order.upgrade_fees << "\n";
    return os;
}

std::ostream& operator<<(std::ostream& os, const planner::OrderDecision& od){
    os << "Order information:"
       << "------------------------------------------------------------\n"
       << "accepted: " << od.accepted << "\n"
       << "revenue: " << od.revenue << "\n"
       << "upgrade_fee: " << od.total_upgrade_fee << "\n"
       << "exp_acc_togo: " << od.exp_acc_togo << "\n"
       << "exp_rej_togo: " << od.exp_rej_togo << "\n"
       << "upgrade_info: " << od.upgrade_info << "\n"
       << "upgraded_rooms: " << od.upgraded_request_rooms << "\n";
    return os;
}