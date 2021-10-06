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