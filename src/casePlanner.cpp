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

Experimentor::Experimentor(){}
Experimentor::Experimentor(const data::CaseData& data)
    :hotel(data), revenue(0), result(){}

// handleOrder handle the order received in given period and return revenue
double Experimentor::handleOrder(const int period, const Order& order){
    
    // Check if the order is acceptable
    if(!this->isAcceptable(order)){
        this->result.decisions[period] = OrderDecision(order);
        return 0;
    } 

    // Find the best decision of this order
    OrderDecision decision = this->findBestOrderDecision(period, order);
    this->result.decisions[period] = decision;

    // Return 0 if this order is not accepted
    if(!decision.accepted) return 0;
    
    // Book the rooms of this order if order is accepted
    bool success = this->booking(decision);

    // Handel booking error
    if(!success){
        std::cout << "Error in run1Period: Booking error at period "
            << period << " with decision info:" << decision;
        exit(1);
    }

    return decision.revenue;
}

// handleOrder handle the individual remand received in given period
// and return revenue
double Experimentor::handleIndDemand(
    const int period, const std::map<data::tuple2d, int>& ind_demand
){
    // Find the acceptable demands
    std::map<data::tuple2d, int> acc_demands = 
        this->getAcceptedIndDemand(ind_demand);
    this->result.accepted_demands[period] = acc_demands;

    // Book the rooms of these demand and get revenue
    bool success = this->booking(acc_demands);

    // Handel booking error
    if(!success){
        std::cout << "Error in run1Period: Booking error at period "
            << period << " with ind demand: " << acc_demands << "\n";
        exit(1);
    }

    return this->getRevIndDemand(acc_demands);
}


// run1Period run the period routine in given period, with given the random
// order and demand, and then return the total revenue in this period.
double Experimentor::run1Period(
    const int period, const Order& order, 
    const std::map<data::tuple2d, int>& ind_demand
){
    // Handle received order 
    double rev_order = this->handleOrder(period, order);

    // Handle individual demand
    double rev_ind_demand = this->handleIndDemand(period, ind_demand);
    return rev_order + rev_ind_demand;
}

// isAcceptable check if this order is acceptable with or without upgrade    
bool Experimentor::isAcceptable(const Order& order){
    
    // If given order is an empty order then return false
    if(!order.is_order) return false;

    // Find bottleneck of capacity during the request days
    std::map<int, int> min_caps = 
        this->hotel.getAllMinCapInPeriods(order.request_days);
    // Check if this order is acceptable considering upgrade

    int sum_req = 0;
    for(int r = this->hotel.getNumRoomType(); r >= 1; r--){
        // Get the maximum arrangable rooms of types that equals or upper then
        // type r.
        // If there are any type that not exist then put 0.
        int max_aval_rooms = min_caps[r];
        std::set<int> upgrade_upper = this->hotel.getUpgradeUpper(r);
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
    return this->getAcceptedIndDemand(ind_demand, this->hotel.getRooms()); 
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
        double price = this->hotel.getPrice(day, room);
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
            if(!this->hotel.canBook(day, room.first, room.second)) return false;
        }
    }

    // Book this order
    for(auto& day: decision.order->request_days){
        for(auto& room: decision.upgraded_request_rooms){
            this->hotel.forceBooking(day, room.first, room.second);
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
        if(!this->hotel.canBook(day, room, num)) return false;
    }
    // Book the room
    for(auto& demand: ind_demand){
        int day = std::get<0>(demand.first), room = std::get<1>(demand.first);
        int num = demand.second;
        this->hotel.forceBooking(day, room, num);
    }
    return true;
}

// getResult return the result of this experimentor
ExperimentorResult Experimentor::getResult(){
    return this->result;
}

// ======================================================================
// -------------------------- CaseExperimentor --------------------------
// ======================================================================

/* Constructors and destructor */
CaseExperimentor::CaseExperimentor(){
    this->scale = nullptr;
    this->orders = nullptr;
    this->ind_demands = nullptr;
    this->revenue = 0;
}

CaseExperimentor::CaseExperimentor(const data::CaseData& data)
    :Experimentor(data)
{
    this->scale = nullptr;
    this->orders = nullptr;
    this->ind_demands = nullptr;
    this->revenue = 0;
};

// addOrders add orders for each booking period   
void CaseExperimentor::addOrders(const std::map<int, Order>& orders){
    this->orders = &orders;
}

// addIndDemands add individual demands for each booking period
void CaseExperimentor::addIndDemands(
    const std::map<int, std::map<data::tuple2d, int> >& demands
){
    this->ind_demands = &demands;
}

// demandJoin return a demand map copied from org_demand and add elements
// {{day, roomtype}}:0} for those day that not in org_demand but in days.
std::map<data::tuple2d, int> CaseExperimentor::demandJoin(
    const std::map<data::tuple2d, int>& org_demand, 
    const std::set<int>& days
){
    // Initialize
    std::map<data::tuple2d, int> demand;
    for(auto& day: days){
        for(int room = 1; room <= this->hotel.getNumRoomType(); room++){
            demand[{day, room}] = 0;
        }
    }
    for(auto& d: org_demand){
        demand[d.first] = d.second;
    }
    return demand;
}

ExperimentorResult CaseExperimentor::run(){
    time_t start;
    time(&start);
    for(int t = this->scale->booking_period; t >= 1; t--){
        
        // Get order and demand received in this period
        auto it_order = this->orders->find(t);
        auto it_demand = this->ind_demands->find(t);

        // Check if order exist
        if(it_order == this->orders->end()){
            raiseKeyError("run", t, "order");
        }

        // Check if demand exist
        if(it_demand == this->ind_demands->end()){
            raiseKeyError("run", t, "demand");
        }

        // Run this period
        double revenue = this->run1Period(
            t, it_order->second, it_demand->second);
        this->result.revenue += revenue;
        
        // If there is no room then break
        if(!this->hotel.hasRooms()){
            this->result.stop_period = t;
            break;
        }
    }
    time_t end;
    time(&end);
    this->result.runtime = difftime(end, start);
    return this->result;
}

// ======================================================================
// -------------------------- DeterExperimentor -------------------------
// ======================================================================


/* Constructors and destructor */
DeterExperimentor::DeterExperimentor(){
    this->estimated_ind_demands = nullptr;
}
DeterExperimentor::DeterExperimentor(const data::CaseData& data)
    : CaseExperimentor(data)
{
    this->scale = &data.scale;
    this->estimated_ind_demands = nullptr;
    this->result.num_periods = data.scale.booking_period;
}

// decide() is called by findBestOrderDecision, it go through the
// upgrade algorithm and store the upgraded information into od. 
void DeterExperimentor::decide(const int period, OrderDecision& od){
    // Error message
    std::string location = "DeterExperimentor::decide";

    // Fetch the information needed in this function
    const auto& req_rooms = od.order->request_rooms;
    const auto& cap = 
        this->hotel.getAllMinCapInPeriods(od.order->request_days);
    auto it = this->estimated_ind_demands->find(period);
    if(it == this->estimated_ind_demands->end()){
        raiseKeyError<int>(location, period, "estimated_demands");
    }
    // Make sure key exist in demand for following use.
    std::map<data::tuple2d, int> demand = 
        this->demandJoin(it->second, od.order->request_days);
    
    // Initialize all containters used in this function
    std::map<int, int> upg_in, upg_out, must_upg;
    std::map<data::tuple2d, int> upg_info;
    for(int i = 1; i <= this->hotel.getNumRoomType(); i++){
        upg_in[i] = 0;
        upg_out[i] = 0;
        must_upg[i] = std::max(req_rooms.at(i) - cap.at(i), 0);
    }
    for(auto& pair: this->hotel.getUpgradePairs()){
        upg_info[pair] = 0;
    }
    
    // Define lambda function used in this function
    auto getSpace = [&](int s, int i) -> int {
        int space = this->hotel.getNumAvailableRooms(s, i) - req_rooms.at(i)
                    + upg_out.at(i) - upg_in.at(i);
        return space;
    };
    auto getSpaces = [&](int i) -> std::map<int, int> {
        std::map<int, int> spaces;
        for(const auto& s: od.order->request_days){
            spaces[s] = getSpace(s, i);
        }
        return spaces;
    };
    auto getBNSpace = [&](int i) -> int {
        return cap.at(i) - req_rooms.at(i) + upg_out.at(i) - upg_in.at(i);
    };
    auto upgGain = [&](int num, int from, int to) -> double {
        auto it = od.order->upgrade_fees.find({from, to});
        if(it == od.order->upgrade_fees.end()) return 0;
        return num * od.order->request_days.size() * it->second;
    };
    auto upgIndGain = [&](
        int num, int space, int ind, double price) -> double 
    {
        if(space + num <= 0) return 0;
        if(space <= 0) return price * std::min(num + space, ind);
        if(space >= ind) return 0;
        return price * std::min(num, ind - space);
    };
    auto upgIndLoss = [&](
        int num, int space, int ind, double price) -> double 
    {
        return price * (num - std::min(std::max(space - ind, 0), num));
    };
    auto upgUtil = [&](int num, int from, int to) -> double {
        double upg_gain = upgGain(num, from, to);
        double util = upg_gain;
        for(const auto& s: od.order->request_days){
            int space_from = getSpace(s, from);
            int space_to = getSpace(s, to);
            double price_from = this->hotel.getPrice(s, from);
            double price_to = this->hotel.getPrice(s, to);
            double upg_gain_ind = upgIndGain(
                num, space_from, demand.at({s, from}), price_from);
            double upg_loss_ind = upgIndLoss(
                num, space_to, demand.at({s, to}), price_to);
            util += upg_gain_ind - upg_loss_ind;
        }
        return util;
    };
    auto upgUB = [&](int from, int to) -> int {
        int limit_to = getBNSpace(to);
        int limit_from = req_rooms.at(from) - upg_out.at(from);
        return std::min(limit_from, limit_to);
    };
    auto upgLB = [&](int i) -> int{
        return std::max(-getBNSpace(i), 0);  
    };

    // Must upgrade
    for(int i = this->hotel.getNumRoomType(); i >= 1; i--){
        if(must_upg.at(i) <= 0) continue;

        // First compute lower bound of upgrade from type-i
        int lb = upgLB(i);
        while(lb > 0){
            int opt_to = 0, opt_num = 0;
            double min_loss_rate = INT_MAX;
            
            for(const auto& j: this->hotel.getUpgradeUpper(i)){
                int ub = upgUB(i, j);

                std::map<int, int> spaces = getSpaces(j);
                for(int x = 1; x <= ub; x++){
                    double loss = 0;
                    for(const auto& s: od.order->request_days){
                        double price = this->hotel.getPrice(s, j);
                        loss += upgIndLoss(
                            x, spaces.at(s), demand.at({s, j}), price);
                    }
                    double loss_rate = loss/static_cast<double>(x);
                    if((loss_rate < min_loss_rate) || 
                       (loss_rate == min_loss_rate && x > opt_num) ||
                       (loss_rate == min_loss_rate && x == opt_num && j < opt_to)
                    ){
                        opt_to = j, opt_num = x;
                        min_loss_rate = loss_rate;
                    }
                }
            }
            int upg_num = std::min(lb, opt_num);
            upg_info[{i, opt_to}] += upg_num;
            upg_out[i] += upg_num;
            upg_in[opt_to] += upg_num;
            int new_lb = upgLB(i);
            if(new_lb >= lb){
                std::cout << "Error: must upgrade error in " 
                    << location << "\n";
                exit(1);
            }
            lb = upgLB(i);
        }
    }

    // Find other upgrades if there has util
    bool has_util = true;
    while(has_util){
        int opt_from = 0, opt_to = 0, upg_num = 0;
        double max_util = 0.0;
        has_util = false;
        for(int i = 1; i <= this->hotel.getNumRoomType(); i++){
            for(const auto& j: this->hotel.getUpgradeUpper(i)){
                int ub = upgUB(i, j);
                for(int x = 0; x <= ub; x++){
                    double util = upgUtil(x, i, j);
                    if(util > max_util){
                        opt_from = i, opt_to = j, upg_num = x;
                        max_util = util;
                        has_util = true;
                    }
                }
            }
        }
        if(!has_util) {
            break;
        }
        upg_info[{opt_from, opt_to}] += upg_num;
        upg_out[opt_from] += upg_num;
        upg_in[opt_to] += upg_num;
    }

    // Store upgrade info to decision
    od.upgrade_info = upg_info;
    od.refreshUpgradeInfo();
    
    // Store revenue-togo to decision
    auto remain = this->hotel.showTryBooking(od);
    auto exp_demand_if_acc_order = this->getAcceptedIndDemand(demand, remain);
    auto exp_demand_if_rej_order = this->getAcceptedIndDemand(demand);
    od.exp_acc_togo = this->getRevIndDemand(exp_demand_if_acc_order);
    od.exp_rej_togo = this->getRevIndDemand(exp_demand_if_rej_order);
}


// findBestOrderDecision return the best order decision
OrderDecision DeterExperimentor::findBestOrderDecision(
    const int period, const Order& order
){
    OrderDecision decision(order);
    this->decide(period, decision);
    decision.accepted = (decision.exp_acc_togo > decision.exp_rej_togo);
    return decision;
}

// addEstIndDemands add the estimated future individual demands. 
void DeterExperimentor::addEstIndDemands(
    const std::map<int, std::map<data::tuple2d, int> >& est_demands
){
    this->estimated_ind_demands = &est_demands;
}

// // ======================================================================
// // -------------------------- StochExperimentor -------------------------
// // ======================================================================

// StochExperimentor::StochExperimentor(){
//     this->estimated_ind_demands = nullptr;
// }

// StochExperimentor::StochExperimentor(const data::CaseData& data)
//     :CaseExperimentor(data)
// {
//     this->scale = &data.scale;
//     this->estimated_ind_demands = nullptr;
//     this->result.num_periods = data.scale.booking_period;
// }

// // ======================================================================
// // --------------------------- ADExperimentor ---------------------------
// // ======================================================================

// ADExperimentor::ADExperimentor(){}
// ADExperimentor::ADExperimentor(const data::CaseData& data)
//     : DeterExperimentor(data){}

// // ======================================================================
// // --------------------------- ASExperimentor ---------------------------
// // ======================================================================

// ASExperimentor::ASExperimentor(){}
// ASExperimentor::ASExperimentor(const data::CaseData& data)
//     : StochExperimentor(data){}

}// End of namespace

// raiseKeyError output the error message and exit
template<typename T>
void raiseKeyError(
    const std::string& location, const T key, const std::string container
){
    std::cout << "Error in " << location << ": "
         << "Key " << key << " not found in " << container << "\n";
    exit(1);
}
template void raiseKeyError<int>(
    const std::string& location, const int key, const std::string container
);
template void raiseKeyError<data::tuple2d>(
    const std::string& location, const data::tuple2d key, const std::string container
);
template void raiseKeyError<std::string>(
    const std::string& location, const std::string key, const std::string container
);