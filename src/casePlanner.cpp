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
#include "caseStructures.hpp"

namespace planner {

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