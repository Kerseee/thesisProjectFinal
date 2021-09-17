//
//  planner.cpp
//  thesisDP
//
//  Created by 黃楷翔 on 2021/3/9.
//  Copyright © 2021年 黃楷翔. All rights reserved.
//

#include <vector>
#include <time.h>
#include <fstream>
#include <iostream>
#include <chrono>
#include <random>
#include <math.h>

#include "planner.hpp"
#include "data.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

#undef DEBUG
//#define DEBUG

#undef TIME
#define TIME


namespace planner {



void createFolder(std::string folder){
     
#ifdef __APPLE__
    system(("mkdir -p "+ folder).c_str());
#elif _WIN32
    std::string WINDOWS_PREFIX = "C:/Users/racs1/Desktop/thesisProject/";
    CreateDirectory((WINDOWS_PREFIX + folder).c_str(), NULL);
#endif
}

int State::num_service_period = 0;
int State::num_room_type = 0;

   
State::State(){
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            this->rooms[{s,i}] = 0;
        }
    }
    this->prob = 0;
}
    
State::State(const std::map<int, int>& capacity){
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            this->rooms[{s,i}] = capacity.at(i);
        }
    }
    this->prob = 0;
}

State::State(const std::map<int, int>& capacity, double prob){
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            this->rooms[{s,i}] = capacity.at(i);
        }
    }
    this->prob = prob;
}

State::State(const State& state){
    this->rooms = state.rooms;
    this->prob = state.prob;
}

State::State(const std::map<data::tuple2d, int>& state_map){
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            this->rooms[{s,i}] = 0;
        }
    }
    for(auto& t: state_map){
        if(this->rooms.find(t.first) != this->rooms.end()){
            this->rooms[t.first] = t.second;
        }
    }
}

State::State(const std::map<data::tuple3d, int>& demand_order, const int order){
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            data::tuple3d key (order,s,i);
            if(demand_order.find(key) != demand_order.end()){
                this->rooms[{s,i}] = demand_order.at(key);
            } else{
                this->rooms[{s,i}] = 0;
            }
        }
    }
}

// Copy constructor
State::State(const State& state, const std::map<data::tuple2d, int>& cap_loss){
    this->rooms = state.rooms;
    for(auto& t: cap_loss){
        this->rooms[t.first] -= t.second;
    }
}

State& State::operator=(const State& state){
    this->rooms = state.rooms;
    this->prob = state.prob;
    return *this;
}

// setScale set the static variables (service_period_ and num_room_type)
// in the State
void State::setScale(const int service_period, const int num_room_type){
    State::num_service_period = service_period;
    State::num_room_type = num_room_type;
}

bool State::operator==(const State& state) const{

    // Check if all the keys in this->state.rooms are equal to state.rooms
    for(auto& s: this->rooms){
        if(s.second != state.rooms.at(s.first)){
            return false;
        }
    }
    return true;
}

bool State::operator<(const State& state) const{
    for(auto& s: this->rooms){
        if(s.second >= state.rooms.at(s.first)){
            return false;
        }
    }
    return true;
}

// findMinRooms return number of rooms in each room type based on given period.
std::map<int, int> State::findMinRooms(const std::set<int>& periods) const {
    std::map<int, int> avai_room_req;
    int start_day = *periods.begin();
    for(int i = 1; i <= State::num_room_type; i++){
        
        int min_avai_room = this->rooms.at({start_day, i});
        for(auto& s: periods){
            if(this->rooms.at({s,i}) <= min_avai_room) {
                min_avai_room = this->rooms.at({s,i});
            }
        }
        avai_room_req[i] = min_avai_room;
    }
    return avai_room_req;
}

bool State::hasMinus(){
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            if(this->rooms.at({s, i}) < 0){
                return true;
            }
        }
    }
    return false;
}

    
UpgradedOrder::UpgradedOrder(){
    this->upgrade_fee = 0;
}
UpgradedOrder::UpgradedOrder(const std::map<int, int>& rooms, const double fee){
    this->rooms = rooms;
    this->upgrade_fee = fee;
}

UpgradedOrder& UpgradedOrder::operator=(const UpgradedOrder& plan){
    this->rooms = plan.rooms;
    this->upgrade_fee = plan.upgrade_fee;
    return *this;
}

UpgradedOrder::UpgradedOrder(const UpgradedOrder& plan){
    *this = plan;
}
    
DPPlanner::DPPlanner(){
    this->dat_ = nullptr;
}

// This constructor store data into DPPlaner,
// and init this->states_.
DPPlanner::DPPlanner(const data::Params& dat){
    State::setScale(dat.scale.service_period, dat.scale.room_type);
    this->dat_ = &dat;
    
    // Init the set containing all possible state
    // Store the index of each level in dynamic nested loop
    // Each level from capacity[i] to 0, when the lower level run to 0,
    // then the one-level-upper level --1.
    // When the top level run to -1, then stop.
    State org_state(this->dat_->capacity);
    int num_states = 1;
    bool status = false;
    std::vector<data::tuple2d> state_key;
    std::map<data::tuple2d, int> cap_loss;

    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            state_key.push_back({s,i});
            num_states *= (this->dat_->capacity.at(i) + 1);
            cap_loss[{s,i}] = 0;
        }
    }
    while(!status){
        bool total_check = 1;

        for(auto& cap: cap_loss){
            bool check = (cap.second == this->dat_->capacity.at(std::get<1>(cap.first)));
            total_check *= check;
        }

        // test for exit condition
        if (total_check){
            status = true;
        }

        this->states_.push_back(State(org_state, cap_loss));

        bool change = true;
        int c = int(cap_loss.size()) - 1;  // start from the most lower level loop
        while (change && c >= 0) {
            // increment the innermost variable and check if spill overs
            int room_type = std::get<1>(state_key[c]);
            if (++cap_loss[state_key[c]] > this->dat_->capacity.at(room_type)) {
                // reset lower loop index
                cap_loss[state_key[c]] = 0;
                // Value of higher loop +1
                change = true;
            }
            else{
                // If higher loop is not affected, then don't move
                change = false;
            }
            // move to the higher loop
            c = c - 1;
        }
    }
}
    
DPPlanner::~DPPlanner(){
    ;
}

// individualRevenue return the revenue of a demand-state from individual custormers.
double DPPlanner::individualRevenue(const State& available_room, const State& ind_demand){
    double revenue = 0;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            int num_rooms = std::min(available_room.rooms.at({s,i}), ind_demand.rooms.at({s,i}));
            double price = this->dat_->price_cus.at({s, i});
            revenue += num_rooms * price;
        }
    }
    return revenue;
}
    
// getProdIndDemand return the probability of a demand-state
double DPPlanner::getProdIndDemand(const int booking_period, const State& demand){
    double prob = 1;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            prob *= this->dat_->demand.prob_ind.at({booking_period, s, i, demand.rooms.at({s,i})});
        }
    }
    return prob;
}

// stateToIndex return the index of state stored in this->states_
// Return -1 if this state is not found in this->states_.
int DPPlanner::stateToIndex(const State& state){
    for(int i = 0; i < this->states_.size(); i++){
        if(state == this->states_[i]){
            return i;
        }
    }
    return -1;
}

// findPosUpgradeOut: given a number of target room, return all possible plan that
// upgrades lower level rooms, whose sum equals to the number of target room.
// Only called by DPPlanner::travelUpgradePlans()
// Return an array of plans. Each plan contains the total number of upgraded
// room out of level-i room and extra-fee.
std::vector<UpgradedOrder> DPPlanner::findPosUpgradeOut(const std::map<int, int>& avai_room,
                                                         std::map<int, int> uplevel_upgrade_out,
                                                         std::map<int, int> upgrades_in,
                                                         const std::map<int, int>& req_room,
                                                         const int room_level, const int order){
    std::vector<UpgradedOrder> possible_out;
    int set_size = room_level - 1;
    int target = upgrades_in[room_level];
    bool status = false;
    
    // Initialize
    std::map<int, int> org_upgrade_out;
    std::map<int, int> capacity;
    for(int i = State::num_room_type; i >= 1; i--){
        org_upgrade_out[i] = 0;
        if(i == 1){
            org_upgrade_out[1] = target;
        }
        capacity[i] = req_room.at(i) - uplevel_upgrade_out.at(i);
    }

    while(!status){
        
        // Check stop condition
        if (org_upgrade_out.at(set_size) == target){
            status = true;
        }
        
        // Check feasibility and store result
        bool feasible_check = true;
        UpgradedOrder plan;
        for(int i = 1; i <= State::num_room_type; i++){
            plan.rooms[i] = uplevel_upgrade_out.at(i) + org_upgrade_out.at(i);
            if(plan.rooms[i] < req_room.at(i) - upgrades_in.at(i) - avai_room.at(i) ||
               plan.rooms[i] > req_room.at(i)){
                feasible_check = false;
                break;
            }
            if(i <= set_size){
                plan.upgrade_fee += org_upgrade_out.at(i)
                                    * this->dat_->upgrade_fee.at({order, i, room_level})
                                    * this->dat_->reqest_periods.at(order).size();
            }
        }

        if(feasible_check){
            possible_out.push_back(plan);
        }
        
        bool change = true;
        int r = 2;  // start from the most lower level loop
        while (change && r <= set_size && !status) {
            // Increment the innermost variable and check if spill overs
            // and the number of upgrade_out of level-1 rooms is decided.
            org_upgrade_out[r]++;
            int sum_upgrade = 0;
            for(int i = 2; i <= set_size; i++){
                sum_upgrade += org_upgrade_out.at(i);
            }
            org_upgrade_out[1] = target - sum_upgrade;
            
            if(org_upgrade_out.at(r) > capacity.at(r)){
                if(r == set_size){
                    status = true;
                }
                org_upgrade_out[r] = 0;
                change = true;
            } else {
                // If higher loop is not affected, then don't move
                change = false;
            }

            // move to the higher loop
            r++;
        }
    }
    return possible_out;
}

// travelUpgradePlans travel all possible upgrade plans recursively.
// All possible plans are stored in upgrade_plans.
// Only called by DPPlanner::findUpgradePlans.
void DPPlanner::travelUpgradePlans(const std::map<int, int>& avai_room,
                                   const std::map<int, int>& req_room,
                                   std::map<int, int> upgrade_out,
                                   std::map<int, int> uplevel_upgrade_in,
                                   const int room_level, double fee, const int order,
                                   std::vector<UpgradedOrder>& upgrade_plans){


    // Compute max upgrade_in
    int num_possible_upgrade_from = 0;
    for(auto& i: this->dat_->upgrade_from.at(room_level)){
        num_possible_upgrade_from += req_room.at(i) - upgrade_out[i];
    }
    int max_upgrade_in = std::min(upgrade_out[room_level] + avai_room.at(room_level) - req_room.at(room_level),
                                  num_possible_upgrade_from);
    
    // Stop condition
    if(max_upgrade_in < 0){
        return ;
    }
    
    // Find all possible upgrade plan
    for(int upgrade_in = 0; upgrade_in <= max_upgrade_in; upgrade_in++){
        std::map<int, int> upgrades_in = uplevel_upgrade_in;
        upgrades_in[room_level] = upgrade_in;
        // Find all possible upgrade_out plan given upgrade_in
        std::vector<UpgradedOrder> possble_out = findPosUpgradeOut(avai_room, upgrade_out, upgrades_in,
                                                                    req_room, room_level, order);
        for(auto& out: possble_out){
            if(room_level == 2){
                UpgradedOrder upgrade_plan;
                for(int i = 1; i <= State::num_room_type; i++){
                    upgrade_plan.rooms[i] = req_room.at(i) - out.rooms.at(i) + upgrades_in.at(i);
                }
                upgrade_plan.upgrade_fee = fee + out.upgrade_fee;
                upgrade_plans.push_back(upgrade_plan);
            } else {
                travelUpgradePlans(avai_room, req_room, out.rooms, upgrades_in,
                                   room_level - 1, fee + out.upgrade_fee, order, upgrade_plans);
            }
        }
    }
    
}


// findUpgradePlans find all possible upgrade plans givem an order.
// All upgrade plan are in the demand form. They can be regarded as adjusted demands.
// Only called by expRevToGoAccept()
std::vector<UpgradedOrder> DPPlanner::findUpgradePlans(const int booking_period,
                                                       const int order, const State& state){
    
    std::vector<UpgradedOrder> upgrade_plans;
    
    // Find requested rooms(d_i)
    std::map<int, int> req_room;
    for(int i = 1; i <= State::num_room_type; i++){
        req_room[i] = this->dat_->req_rooms.at({order, i});
    }
     
    
    // Find the min number of available room of each room level in the requested day:
    // min q_si for s in S_k
    std::map<int, int> avai_room_req = state.findMinRooms(this->dat_->reqest_periods.at(order));
    
    // Check if this order is unacceptable. If it is, then return null upgrade_plans.
    for(int i = State::num_room_type; i >= 1; i--){
        int max_avai_rooms = avai_room_req[i];
        for(auto& upgradeI: this->dat_->upgrade_to.at(i)){
            max_avai_rooms += avai_room_req[upgradeI];
        }
        if(req_room[i] > max_avai_rooms){
            return upgrade_plans;
        }
    }
    if(State::num_room_type <= 1){
        UpgradedOrder plan{req_room, 0};
        upgrade_plans.push_back(plan);
        return upgrade_plans;
    } else {
        // Find possible upgrade plan
        std::map<int, int> upgrade_out;
        std::map<int, int> upgrade_in;
        for(int i = State::num_room_type; i >= 1 ; i--){
            upgrade_out[i] = 0;
            upgrade_in[i] = 0;
        }
        travelUpgradePlans(avai_room_req, req_room, upgrade_out, upgrade_in,
                           State::num_room_type, 0, order, upgrade_plans);
    }

    
    return upgrade_plans;
}

    

// Return the expected revenue to from this booking period to the end of the
// booking horizon, given booking period, state, and that the order from the travel
// agency is rejected.
double DPPlanner::expRevToGoReject(const int booking_period, const State& state){
    double exp_rev = 0;
    
    // See each state in the this->states_ as a kind of individual demand
    for(int d = 0; d < this->states_.size(); d++){
        double ind_prob = this->getProdIndDemand(booking_period, this->states_[d]);
        double ind_rev = this->individualRevenue(state, this->states_[d]);
        
        // Don't need to consider expected value to go if booking_period == 1
        if(booking_period == 1){
            exp_rev += ind_prob * ind_rev;
        } else {
            // Compute expected revenue from this booking period to period 1
            int stateID = this->stateToIndex(state - this->states_[d]);
            double toGo_expRev = 0;
            for(int k = 0; k <= this->dat_->scale.order_type; k++){
                double order_prob = this->dat_->demand.prob_order.at({booking_period - 1, k});
                double toGo_rev = this->expected_revenue_.at({booking_period - 1, k, stateID});
                toGo_expRev += order_prob * toGo_rev;
            }
            exp_rev += (ind_rev + toGo_expRev) * ind_prob;
        }
    }
    return exp_rev;
}
   

void DPPlanner::debugFunc(){
    std::cout << this->dat_->upgrade_from << "\n";
    std::vector<UpgradedOrder> upgrade_plans = findUpgradePlans(1, 1, this->states_[0]);
    std::cout << "state: " << this->states_[0] << "\n";
    std::cout << "upgrade plan:\n";
    for(auto& plan: upgrade_plans){
        std::cout << plan.rooms << "| $"<< plan.upgrade_fee <<"\n";
    }
}
    
    
// expRevToGoAccpet return the expected revenue to from this booking period to the end of the
// booking horizon, given booking period, state, and that the order from the travel
// agency is accepted.
double DPPlanner::expRevToGoAccept(const int booking_period, const int order,
                                   const State& state){
    double max_exp_rev = 0;
    // See each state in the this->states_ as a kind of individual demand
    
    std::vector<UpgradedOrder> upgrade_plans = findUpgradePlans(booking_period, order, state);
    for(auto& plan: upgrade_plans){
        // Compute expected revenue if this plan is used
        double exp_rev = this->dat_->price_order.at(order);
        State upgraded_demand(plan.rooms);
        for(int d = 0; d < this->states_.size(); d++){
            double ind_prob = this->getProdIndDemand(booking_period, this->states_[d]);
            double ind_rev = this->individualRevenue(state - upgraded_demand, this->states_[d]);
            
            // Don't need to consider expected value to go if booking_period == 1
            if (booking_period == 1){
                exp_rev += ind_prob * (ind_rev + plan.upgrade_fee);
            } else {
                // Compute expected revenue from this booking period to period 1
                int stateID = this->stateToIndex(state - upgraded_demand - this->states_[d]);
                double toGo_expRev = 0;
                for(int k = 0; k <= this->dat_->scale.order_type; k++){
                    double order_prob = this->dat_->demand.prob_order.at({booking_period - 1, k});
                    double toGo_rev = this->expected_revenue_.at({booking_period - 1, k, stateID});
                    toGo_expRev += order_prob * toGo_rev;
                }
                exp_rev += (ind_rev + toGo_expRev + plan.upgrade_fee) * ind_prob;
            }
        }
        // Find maximum expected upgraded revenue
        if (exp_rev > max_exp_rev){
            max_exp_rev = exp_rev;
        }
    }

    return max_exp_rev;
}

    
// computeEV compute and store the expected revenue given booking_period,
// order id and state_id
void DPPlanner::computeExpRev(const int booking_period, const int order, const int state_id){
    
    // Compute EV if reject
    double expRevReject = this->expRevToGoReject(booking_period, this->states_[state_id]);
    // Compute EV if accept
    double expRevAccept = 0;
    if(order > 0){
        expRevAccept = this->expRevToGoAccept(booking_period, order, this->states_[state_id]);
    }
    
    this->exp_rev_reject_[{booking_period, order, state_id}] = expRevReject;
    this->exp_rev_accept_[{booking_period, order, state_id}] = expRevAccept;
    
    // Compare reject and accept
    if (expRevReject >= expRevAccept) {
        this->decision_[{booking_period, order, state_id}] = false;
        this->expected_revenue_[{booking_period, order, state_id}] = expRevReject;
    } else {
        this->decision_[{booking_period, order, state_id}] = true;
        this->expected_revenue_[{booking_period, order, state_id}] = expRevAccept;
    }
}
    
// run() is the main function of DPPlanner.
// It iterates all possible state, and store the expected revenues.
double DPPlanner::run(){
#ifdef TIME
    time_t run_start;
    time(&run_start);
#endif
    std::cout << "DP run start! Total " << this->states_.size() << " states\n";
    for(int t = 1; t <= this->dat_->scale.booking_period; t++){
        // k = 0 for no order
        for(int k = 0; k <= this->dat_->scale.order_type; k++){
            for(int state = 0; state < this->states_.size(); state++){
                this->computeExpRev(t, k, state);
#ifdef TIME
                int print_unit = this->states_.size();
                if(state % print_unit == print_unit - 1){
                    time_t time_now;
                    time(&time_now);
                    std::cout << "(t, k) = (" << t << "," << k <<") | ";
                    std::cout << "time: " << difftime(time_now, run_start) << "\n";
                    //                    std::cout << "state:" << this->states_[state] << "\n";
                }
#endif
            }
        }
    }
    
    double exp_rev = 0;
    int period = this->dat_->scale.booking_period;
    int state = 0;
    for(int k = 0; k <= this->dat_->scale.order_type; k++){
        exp_rev += this->expected_revenue_.at({period, k, state}) 
                   * this->dat_->demand.prob_order.at({period, k});
    }

    return exp_rev;
}
    
std::vector<State>& DPPlanner::getStatesSet(){
    return this->states_;
}

// printExpRev print the expected revenue stored in DPPlanner.
// Use enum expValue in DPPlanner to switch expected revenues to print.
void DPPlanner::printExpRev(expValue ev){
    std::map<data::tuple3d, double>* exp_rev_ptr = nullptr;
    std::string prefix = "";
    switch (ev) {
        case max:
            prefix = "Max";
            exp_rev_ptr = &this->expected_revenue_;
            break;
        case accpet:
            prefix = "Accept";
            exp_rev_ptr = &this->exp_rev_accept_;
            break;
        case reject:
            prefix = "Reject";
            exp_rev_ptr = &this->exp_rev_reject_;
            break;
        default:
            break;
    }
    
    std::cout << prefix + " expected Value:******************************************************************************\n";
    for(int t = 1; t <= this->dat_->scale.booking_period; t++){
        for(int k = 0; k <= this->dat_->scale.order_type; k++){
            std::cout << "Period " << t << ", order " << k << ":--------------------------------------------\n";
            for(int state = 0; state < this->states_.size(); state++){
                std::cout << "[" << state + 1 << ":" << exp_rev_ptr->at({t, k, state}) << "]";
            }
            std::cout << "\n\n";
        }
    }
}
    // TODO 3/22
//    void DPPlanner::printDecision();
   
void DPPlanner::storeResult(result type, const std::string& file_path){
    std::string result_type = "";
    switch (type) {
        case revenue:
            result_type = "R_";
            break;
        case decision:
            result_type = "D_";
            break;
        default:
            ;
    }
    
    std::string general_path = file_path + "/" + result_type +
       "I" + std::to_string(this->dat_->scale.room_type) +
       "K" + std::to_string(this->dat_->scale.order_type) +
       "T" + std::to_string(this->dat_->scale.booking_period) +
       "S" + std::to_string(this->dat_->scale.service_period) +
       "C" + std::to_string(this->dat_->capacity.at(1));
    
    for(int t = 1; t <= this->dat_->scale.booking_period; t++){
        std::string path = general_path + "_t" + std::to_string(t) + ".csv";
        std::ofstream output;
        output.open(path);
        output << "State";
        for(int k = 0; k <= this->dat_->scale.order_type; k++){
            output << ",k=" << k;
        }
        output << "\n";
        for(int state = 0; state < this->states_.size(); state++){
            output << state;
            for(int s = 1; s <= State::num_service_period; s++){
                output << "|s" << s << ":";
                for(int i = 1; i <= State::num_room_type; i++){
                    output << this->states_.at(state).rooms.at({s,i});
                    if (i < State::num_room_type){
                        output << "_";
                    }
                }
            }
            output << "|";
            for(int k = 0; k <= this->dat_->scale.order_type; k++){
                switch (type) {
                    case revenue:
                        output << "," << this->expected_revenue_.at({t, k, state});
                        break;
                    case decision:
                        output << "," << this->decision_.at({t, k, state});
                        break;
                    default:
                        ;
                }
            }
            output << "\n";
        }
        output.close();
    }
}

void DPPlanner::storeTime(const std::string& file_path, const double& time){
    std::ofstream output(file_path, std::ofstream::out|std::ofstream::app);
    output << this->dat_->scale.room_type<< "," 
           << this->dat_->scale.order_type << ","
           << this->dat_->scale.booking_period << ","
           << this->dat_->scale.service_period << ","
           << this->dat_->capacity.at(1) << ","
           << time << "\n";
    output.close();
}

MyopicUpgradePlan::MyopicUpgradePlan(){
    this->id = 0;
    this->upgrade_fee = 0.0;
    this->exp_acc_togo = 0.0;
    this->exp_rej_togo = 0.0;
    this->price = 0.0;
    this->revenue = 0.0;
    this->decision = false;
}

MyopicUpgradePlan::MyopicUpgradePlan(const int id, const State& order, const std::set<int>& days,
                                     const double price, const double upgrade_fee,
                                     const std::map<data::tuple2d, int>& upgrade){
    this->id = id;
    this->order = order;
    this->exp_acc_togo = 0;
    this->exp_rej_togo = 0;
    this->price = price;
    this->upgrade_fee = upgrade_fee;
    this->upgrade = upgrade;
    this->days = days;
    this->computeUpgradedRooms();
}

MyopicUpgradePlan::MyopicUpgradePlan(const int id, const State& order, const std::set<int>& days,
                                     const double exp_acc_togo, const double exp_rej_togo,
                                     const double price, const double upgrade_fee,
                                     const std::map<data::tuple2d, int>& upgrade){
    this->id = id;
    this->order = order;
    this->exp_acc_togo = exp_acc_togo;
    this->exp_rej_togo = exp_rej_togo;
    this->price = price;
    this->upgrade_fee = upgrade_fee;
    this->upgrade = upgrade;
    this->days = days;
    this->computeUpgradedRooms();
}

MyopicUpgradePlan::MyopicUpgradePlan(const MyopicUpgradePlan& plan){
    this->id = plan.id;
    this->order = plan.order;
    this->upgrade_fee = plan.upgrade_fee;
    this->exp_acc_togo = plan.exp_acc_togo;
    this->exp_rej_togo = plan.exp_rej_togo;
    this->upgrade = plan.upgrade;
    this->price = plan.price;
    this->revenue = plan.revenue;
    this->decision = plan.decision;
    this->days = plan.days;
    this->upgraded_order = plan.upgraded_order;
}

MyopicUpgradePlan& MyopicUpgradePlan::operator=(const MyopicUpgradePlan& plan){
    this->id = plan.id;
    this->order = plan.order;
    this->upgrade_fee = plan.upgrade_fee;
    this->exp_acc_togo = plan.exp_acc_togo;
    this->exp_rej_togo = plan.exp_rej_togo;
    this->upgrade = plan.upgrade;
    this->price = plan.price;
    this->revenue = plan.revenue;
    this->decision = plan.decision;
    this->days = plan.days;
    this->upgraded_order = plan.upgraded_order;
    return *this;
}

void MyopicUpgradePlan::decide(const bool accept){
    if(accept){
        this->decision = true;
        this->revenue = this->price + this->upgrade_fee;
    } else {
        this->decision = false;
        this->revenue = 0.0;
    }
}


// computeUpgradedRomms return the upgraded rooms arrangement
void MyopicUpgradePlan::computeUpgradedRooms() {
    // Combine information in this->upgrade into two map
    std::map<int, int> upgrade_in;
    std::map<int, int> upgrade_out;
    for(int i = 1; i <= State::num_room_type; i++){
        upgrade_in[i] = 0;
        upgrade_out[i] = 0;
    }
    for(auto& u: this->upgrade){
        int from = std::get<0>(u.first);
        int to = std::get<1>(u.first);
        int num = u.second;
        upgrade_out[from] += num;
        upgrade_in[to] += num;
    }
    
    // Update the given state
    for(int s: this->days){
        for(int i = 1; i <= State::num_room_type; i++){
            this->upgraded_order.rooms[{s, i}] = this->order.rooms.at({s, i}) - upgrade_out[i] + upgrade_in[i];
        }
    }
}

void MyopicUpgradePlan::print(){
    std::cout << "Upgraded Plan from order " << this->id << ":\n";
    std::cout << "order: " << this->order.rooms << "\n";
    std::cout << "upgraded_order: " << this->upgraded_order.rooms << "\n";
    std::cout << "upgrade: " << this->upgrade << "\n";
    std::cout << "price: " << this->price
              << " | upgrade_fee: " << this->upgrade_fee
              << " | exp_acc_togo: " << this->exp_acc_togo
              << " | exp_rej_togo: " << this->exp_rej_togo  << "\n";
    std::cout << "decision: " << this->decision
              << " | revenue: " << this->revenue << "\n";
    std::cout << "----------------------------------------\n";
}

MyopicPlanner::MyopicPlanner(){
    this->dat_ = nullptr;
    this->num_experiment_ = 1000;
    this->num_exper_SP_ = 1000;
    this->threshold_ = 0.01;
    this->early_stop_ = 50;
    this->smart_gamma_ = 1;
    this->baseline_beta_ = 1;
    this->random_first_ = false;
}

MyopicPlanner::MyopicPlanner(const data::Params& dat, MyopicMode m, MyopicModelType mt){
    State::setScale(dat.scale.service_period, dat.scale.room_type);
    this->dat_ = &dat;
    this->num_experiment_ = 1000;
    this->num_exper_SP_ = 1000;
    this->threshold_ = 0.01;
    this->early_stop_ = 50;
    this->setMode(m);
    this->setModelType(mt);
    this->storeDemandCdf();
    this->smart_gamma_ = 1;
    this->baseline_beta_ = 1;
    this->random_first_ = false;

    for(int t = 1; t <= this->dat_->scale.booking_period; t++){
        double sum_prod = 0; 
        for(int k = 0; k <= this->dat_->scale.order_type; k++){
            sum_prod += this->dat_->demand.prob_order.at({t,k});
            this->cum_prob_order[{t,k}] = sum_prod;
        }
    }
}

void MyopicPlanner::setMode(MyopicMode m){
    this->mode_ = m;
}

void MyopicPlanner::setModelType(MyopicModelType mt){
    this->model_type_ = mt;
}

void MyopicPlanner::storeDemandCdf(){
    for(int t = 1; t <= this->dat_->scale.booking_period; t++){
        for(int s = 1; s <= this->dat_->scale.service_period; s++){
            for(int i = 1; i <= this->dat_->scale.room_type; i++){
                double sum_prod = 0;
                for(int c = 0; c <= this->dat_->capacity.at({i}); c++){
                    sum_prod += this->dat_->demand.prob_ind.at({t, s, i, c});
                    this->demand_cdf_[{t, s, i, c}] = sum_prod;
                }
            }
        }
    }
}

// resetState reset the num of available rooms to the origin capacity
void MyopicPlanner::resetState(){
    State full_state(this->dat_->capacity);
    this->state_ = full_state;
}

// setNumExpt set the number of experiments
void MyopicPlanner::setNumExpt(const int num){
    this->num_experiment_ = num;
}

void MyopicPlanner::setNumSample(const int num){
    this->num_exper_SP_ = num;
}

// setThreshold set the threshold
void MyopicPlanner::setThreshold(const double thd){
    this->threshold_ = thd;
}

// randomOrder return the index of order arriving at this period
int MyopicPlanner::randomOrder(const int period){
#ifdef __APPLE__
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
#elif _WIN32
    srand(this->exper_id_);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count() / (rand() * period);
#endif

    std::default_random_engine generator (seed);
    std::uniform_real_distribution<double> distribution (0.0, 1.0);
    double prob = distribution(generator);
    // std::cout << "DEBUG: " << seed << ", " << prob << "," << std::chrono::system_clock::now().time_since_epoch().count() << "\n";

    for(int k = 0; k <= this->dat_->scale.order_type; k++){
        if(prob <= this->cum_prob_order.at({period, k})){
            // std::cout << "DEBUG: seed: " << seed << " | prob: " << prob << " | k: " << k << "\n";
            return k;
        }
    }
    // std::cout << "impossible!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    return 0;
}

// randomOrder return the map of order arriving at each period
std::map<int, int> MyopicPlanner::randomOrder(){
    std::map<int, int> random_orders;

#ifdef __APPLE__
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
#elif _WIN32
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count() / this->exper_id_;
#endif

    std::default_random_engine generator (seed);
    // std::cout << "DEBUG | exper: " << this->exper_id_ << " | seed: " << seed << "\n";
    std::uniform_real_distribution<double> distribution (0.0, 1.0);

    for(int t = this->dat_->scale.booking_period; t >= 1; t--){
        double prob = distribution(generator);
        for(int k = 0; k <= this->dat_->scale.order_type; k++){
            if(prob <= this->cum_prob_order.at({t, k})){
                random_orders[t] = k;
                // std::cout << "DEBUG: seed: " << seed << " | prob: " << prob << " | k: " << k << "\n";
                break;
            }
        }
    }
    return random_orders;
}

// isAcceptable check if this order is acceptable with or without upgrade
bool MyopicPlanner::isAcceptable(const int order, const State& state){
    std::map<int, int> aval_rooms = state.findMinRooms(this->dat_->reqest_periods.at(order));
    // Check if this order is acceptable considering upgrade
    int sum_req = 0;
    for(int i = State::num_room_type; i >= 1; i--){
        int max_aval_rooms = aval_rooms[i];
        for(auto& upgradeI: this->dat_->upgrade_to.at(i)){
            max_aval_rooms += aval_rooms.at(upgradeI);
        }
        sum_req += this->dat_->req_rooms.at({order, i});
        if(sum_req > max_aval_rooms){
            return false;
        }
    }
    return true;
}

bool MyopicPlanner::isAcceptable(const MyopicUpgradePlan& plan, const State& state){
    std::map<int, int> aval_rooms = state.findMinRooms(plan.days);
    // Check if this order is acceptable considering upgrade
    int sum_req = 0, start_day = *plan.days.begin();
    for(int i = State::num_room_type; i >= 1; i--){
        int max_aval_rooms = aval_rooms[i];
        for(auto& upgradeI: this->dat_->upgrade_to.at(i)){
            max_aval_rooms += aval_rooms.at(upgradeI);
        }
        sum_req += plan.order.rooms.at({start_day, i});
        if(sum_req > max_aval_rooms){
            return false;
        }
    }
    return true;
}

// guessInvDemand return the random demand we guess from individuals in the future
State MyopicPlanner::guessInvDemand(const int period){
    State demand;
    
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            // double prob = this->dist_params_demand_.at({period, s, i});
            // demand.rooms[{s,i}] = getExpValNB(period, prob);
            double exp_num_rooms = 0;
            for(int t = period; t >= 1; t--){
                for(int c = 0; c <= this->dat_->capacity.at(i); c++){
                    exp_num_rooms += c * this->dat_->demand.prob_ind.at({t, s, i, c});
                }
            }
            demand.rooms[{s, i}] = round(exp_num_rooms);
        }
    }
    
    return demand;
}

// getAdjOrderPrices return ths adjustments order prices
std::map<data::tuple2d, double> MyopicPlanner::getAdjOrderPrices(const int period){
    // Guess the demand and price of future orders
    // Set the unit price of orders
    std::map<data::tuple3d, double> unit_price_orders;
    for(int k = 1; k <= this->dat_->scale.order_type; k++){
        double total_price = this->dat_->price_order.at(k);
        // 計算這個量如果是散客的話總共要多少錢
        double full_price = 0;
        // 訂單於 {s, i} 這部分金額佔 full_price 多少比例
        std::map<data::tuple2d, double> price_occupy;
        for(int i = 1; i <= State::num_room_type; i++){
            for(int s: this->dat_->reqest_periods.at(k)){
                int num = this->dat_->demand_order.at({k, s, i});
                price_occupy[{s, i}] = this->dat_->price_cus.at({s, i}) * num;
                full_price += price_occupy.at({s, i});
            }
        }
        for(int i = 1; i <= State::num_room_type; i++){
            for(int s: this->dat_->reqest_periods.at(k)){
                double unit_price = 0;
                if(price_occupy.at({s, i}) > 0){
                    unit_price = (price_occupy.at({s, i}) / full_price)
                                 * total_price
                                 / static_cast<double>(this->dat_->demand_order.at({k, s, i}));
                }
                unit_price_orders[{k, s, i}] = unit_price;
            }
        }
    }

    // 計算各訂單 k 於 {s, i} 的權重
    std::map<data::tuple3d, double> weight_orders;
    std::map<data::tuple2d, double> total_weight;
    for(int k = 1; k <= this->dat_->scale.order_type; k++){
        // 計算訂單 k 在接下來的 period 至少出現一次的機率
        double prob_no_show = 1;
        for(int t = period - 1; t >= 1; t--){
            prob_no_show *= (1 - this->dat_->demand.prob_order.at({t, k}));
        }
        double prob_order_appear = 1 - prob_no_show;

        // 將機率乘上這個訂單 k 於 {s, i} 的訂貨量
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                int num = 0;
                if(s >= this->dat_->checkin.at(k) &&
                    s <= this->dat_->checkout.at(k)){
                    num = this->dat_->req_rooms.at({k, i});
                }
                double weight = prob_order_appear * num;
                weight_orders[{k, s, i}] = weight;
                total_weight[{s, i}] += weight;
            }
        }
    }

    // std::cout << "DEBUG: unit_price_orders: " << unit_price_orders << "\n";
    // std::cout << "DEBUG: wight_orders: " << weight_orders << "\n";
    // std::cout << "DEBUG: total_weights: " << total_weight << "\n";

    // 用上面的權重和unit_price_order 算加權平均，做為 {s, i} 在
    // 旅行社下訂之下的單位價格
    std::map<data::tuple2d, double> exp_price_orders;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            double price = 0;
            if(total_weight.at({s, i}) > 0){
                for(int k = 1; k <= this->dat_->scale.order_type; k++){
                    double unit_weight = weight_orders.at({k, s, i}) / total_weight.at({s, i});
                    if(unit_weight > 0){
                        price += unit_weight * unit_price_orders.at({k, s, i});
                    }
                }
            }
            exp_price_orders[{s, i}] = price;
        }
    }

    return exp_price_orders;
}

// getAdjOrderdemands return ths adjustments order demand
State MyopicPlanner::getAdjOrderdemands(const int period){
    State orders;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            double exp_nums = 0;
            for(int t = period - 1; t >= 1; t--){
                double exp_num = 0;
                for(int k = 1; k <= this->dat_->scale.order_type; k++){
                    int num = 0;
                    if(s >= this->dat_->checkin.at(k) &&
                        s <= this->dat_->checkout.at(k)){
                        num = this->dat_->req_rooms.at({k, i});
                    }
                    exp_num += this->dat_->demand.prob_order.at({t, k}) * num;
                }
                exp_nums += exp_num;
            }
            orders.rooms[{s, i}] = round(exp_nums * this->smart_gamma_);
            
        }
    }
    return orders;
}

// getAdjAllDemandPrices return ths adjustments prices, which are weighted average of individual prices
// and order prices, using demand quantities as weights.
std::map<data::tuple2d, double> 
MyopicPlanner::getAdjAllDemandPrices(const State& demand, const State& orders,
                                     const std::map<data::tuple2d, double>& order_prices){
    
    // std::cout << "DEBUG: gAADP - price_orders " << order_prices << "\n";
    std::map<data::tuple2d, double> prices;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){            
            // 按散客和旅行社的比例調整 {s, i} unit price
            // std::cout << "DEBUG: gAADP - i " << "\n";
            double price_order = orders.rooms.at({s, i}) * order_prices.at({s, i});
            double price_demand = demand.rooms.at({s, i}) * this->dat_->price_cus.at({s, i});
            int total_num = orders.rooms.at({s, i}) + demand.rooms.at({s, i});
            if(total_num > 0){
                prices[{s, i}] = (price_order * this->smart_gamma_ + price_demand)
                                  / static_cast<double>(orders.rooms.at({s, i}) * this->smart_gamma_ 
                                                        + demand.rooms.at({s, i}));
            } else {
                prices[{s, i}] = this->dat_->price_cus.at({s, i});
            }
        }
    }
    // std::cout << "DEBUG: gAADP - price " << prices << "\n";
    return prices;
}

// guessFutureDemand return the random demand we guess from individuals 
// and travel agencies in the future, and also return the guessed price
// of these rooms.
std::tuple<State, std::map<data::tuple2d, double> > 
MyopicPlanner::guessFutureDemand(const int period, const State& demand){

    if(period <= 1){
        std::map<data::tuple2d, double> prices;
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                prices[{s, i}] = this->dat_->price_cus.at({s, i});
            }
        }
        return std::make_tuple(demand, prices);
    }

    std::map<data::tuple2d, double> exp_price_orders = this->getAdjOrderPrices(period);
    State orders = this->getAdjOrderdemands(period);
    std::map<data::tuple2d, double> prices = this->getAdjAllDemandPrices(demand, orders, exp_price_orders);
 
    return std::make_tuple(orders + demand, prices);
}

// guessFutureDemand return the random demand we guess from individuals 
// and travel agencies in the future, and also return the guessed price
// of these rooms.
void MyopicPlanner::guessFutureDemandSP(const int period, const std::vector<State>& s_demands,
                             std::vector<State>& adj_demands, 
                             std::vector<std::map<data::tuple2d, double> >& adj_prices){
    adj_demands.clear();
    adj_prices.clear();

    if(period <= 1){
        std::map<data::tuple2d, double> prices;
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                prices[{s, i}] = this->dat_->price_cus.at({s, i});
            }
        }
        adj_demands = s_demands;
        for(int w = 0; w < s_demands.size(); w++){
            adj_prices.push_back(prices);
        }
        return ;
    }

    std::map<data::tuple2d, double> exp_price_orders = this->getAdjOrderPrices(period);
    State orders = this->getAdjOrderdemands(period);

    for(int w = 0; w < s_demands.size(); w++){
        std::map<data::tuple2d, double> prices = this->getAdjAllDemandPrices(s_demands[w], orders, exp_price_orders);
        State demand = orders + s_demands[w];
        demand.prob = s_demands[w].prob;
        adj_demands.push_back(demand);
        adj_prices.push_back(prices);
    }
}

// getUpgradeFee return the upgrade fee of given upgrade plan
double MyopicPlanner::getUpgradeFee(const int order,
                                    const std::map<data::tuple2d, int>& upgrade){
    double total_upgrade_fee = 0;
    for(auto& u: upgrade){
        int from = std::get<0>(u.first);
        int to = std::get<1>(u.first);
        int num = u.second;
        double fee = this->dat_->upgrade_fee.at({order, from, to});
        int days = this->dat_->reqest_periods.at(order).size();
        total_upgrade_fee += fee * num * days;
        // std::cout << "(" << from << "," << to << "," << fee << "," << num << "," << days << "): " << fee * num * days << " | ";
    }
    return total_upgrade_fee;
}

// findOptPlanIP return the optimal revenue from the optimal decision to
// the given order, considering the demand we guess. This problem is formulated
// in an integer programming problem in this function.
void MyopicPlanner::findOptPlanIP(const int period, MyopicUpgradePlan& plan, 
                                  const State& state, const State& demand,
                                  const std::map<data::tuple2d, double>& prices){

    // Initialize all containters used in this function
    const auto& upg_to = this->dat_->upgrade_to;
    const auto& upg_from = this->dat_->upgrade_from;
    const auto& req_days = plan.days;
    const double order_price = plan.price;
    std::map<int, int> cap = state.findMinRooms(req_days); 

    std::map<int, int> req_rooms, upg_in, upg_out, must_upg;
    std::map<data::tuple2d, int> upg_info;
    std::map<data::tuple2d, double> upg_fee;
    int start_day = *req_days.begin();
    for(int i = 1; i <= State::num_room_type; i++){
        req_rooms[i] = plan.order.rooms.at({start_day, i});
        upg_in[i] = 0;
        upg_out[i] = 0;
        must_upg[i] = std::max(req_rooms.at(i) - cap.at(i), 0);
        for(const auto& j: upg_to.at(i)){
            upg_info[{i, j}] = 0;
            upg_fee[{i, j}] = this->dat_->upgrade_fee.at({plan.id, i, j});
        }
    }
    
    // std::cout << "DEBUG | cap: " << cap << "\n";
    // std::cout << "DEBUG | req_days: " << req_days << "\n";
    // std::cout << "DEBUG | req_rooms: " << req_rooms << "\n";
    // std::cout << "DEBUG | upg_to: " << upg_to << "\n";
    // std::cout << "DEBUG | upg_from: " << upg_from << "\n";
    // std::cout << "DEBUG | upg_in: " << upg_in << "\n";
    // std::cout << "DEBUG | upg_out: " << upg_out << "\n";
    // std::cout << "DEBUG | upg_fee: " << upg_fee << "\n";
    // std::cout << "DEBUG | upg_info: " << upg_out << "\n";
    // std::cout << "DEBUG | must_upg: " << must_upg << "\n";
    // std::cout << "DEBUG | order_price: " << order_price << "\n";

    auto getSpace = [&](int s, int i) -> int {
        return state.rooms.at({s, i}) - req_rooms.at(i) + upg_out.at(i) - upg_in.at(i);
    };
    auto getSpaces = [&](int i) -> std::map<int, int> {
        std::map<int, int> spaces;
        for(const auto& s: req_days){
            spaces[s] = getSpace(s, i);
        }
        return spaces;
    };
    auto getBNSpace = [&](int i) -> int {
        return cap.at(i) - req_rooms.at(i) + upg_out.at(i) - upg_in.at(i);
    };
    auto upgGain = [&](int num, int from, int to) -> double {
        return num * req_days.size() * upg_fee.at({from, to});
    };
    auto upgIndGain = [&](int num, int space, int ind, double price) -> double {
        if(space + num <= 0) return 0;
        if(space <= 0) return price * std::min(num + space, ind);
        if(space >= ind) return 0;
        return price * std::min(num, ind - space);
    };
    auto upgIndLoss = [&](int num, int space, int ind, double price) -> double {
        return price * (num - std::min(std::max(space - ind, 0), num));
    };
    auto upgUtil = [&](int num, int from, int to) -> double {
        double upg_gain = upgGain(num, from, to);
        double util = upg_gain;
        for(const auto& s: req_days){
            int space_from = getSpace(s, from);
            int space_to = getSpace(s, to);
            double upg_gain_ind = upgIndGain(num, space_from, demand.rooms.at({s, from}), prices.at({s, from}));
            double upg_loss_ind = upgIndLoss(num, space_to, demand.rooms.at({s, to}), prices.at({s, to}));
            // std::cout << "s: " << s << " | upg_g: " << upg_gain << " | upg_gi: " << upg_gain_ind
            //           << " | upg_li: " << upg_loss_ind << "\n"; 
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
    for(int i = State::num_room_type; i >= 1; i--){
        if(must_upg.at(i) <= 0) continue;

        // First compute lower bound of upgrade from type-i
        int lb = upgLB(i);
        while(lb > 0){
            int opt_to = 0, opt_num = 0;
            double min_loss_rate = INT_MAX;
            for(const auto& j: upg_to.at(i)){
                int ub = upgUB(i, j);
                // std::cout << "(" << i << "," << j << ") | LB: " << lb << " | UB: " << ub;

                std::map<int, int> spaces = getSpaces(j);
                for(int x = 1; x <= ub; x++){
                    double loss = 0;
                    for(const auto& s: req_days){
                        loss += upgIndLoss(x, spaces.at(s), demand.rooms.at({s, j}), prices.at({s, j}));
                    }
                    double loss_rate = loss/static_cast<double>(x);
                    // std::cout << " | " << x << ": " << loss << " (" << loss_rate << ")";
                    // Find best upgrade plan
                    if((loss_rate < min_loss_rate) || 
                       (loss_rate == min_loss_rate && x > opt_num) ||
                       (loss_rate == min_loss_rate && x == opt_num && j < opt_to)){
                        opt_to = j, opt_num = x;
                        min_loss_rate = loss_rate;
                    }
                }
                // std::cout  << "\n";
            }
            int upg_num = std::min(lb, opt_num);
            upg_info[{i, opt_to}] += upg_num;
            upg_out[i] += upg_num;
            upg_in[opt_to] += upg_num;
            int new_lb = upgLB(i);
            if(new_lb >= lb){
                std::cout << "Error: must upgrade error in findOptPlanIP!!!\n";
                exit(1);
            }
            lb = upgLB(i);
            // std::cout << "DEBUG | opt_to: " << opt_to
            //           << " | opt_num: " << opt_num
            //           << " | upg_num: " << upg_num
            //           << " | min_loss: " << min_loss_rate
            //           << " | upg_info: " << upg_info << "\n";
            // std::cout << "--------------------------------\n";
        }
    }

    // Find other upgrades if there has util
    bool has_util = true;
    while(has_util){
        int opt_from = 0, opt_to = 0, upg_num = 0;
        double max_util = 0.0;
        has_util = false;
        for(int i = 1; i <= State::num_room_type; i++){
            for(const auto& j: upg_to.at(i)){
                int ub = upgUB(i, j);
                // std::cout << "(" << i << "," << j  << ") | UB: " << ub << "\n";
                for(int x = 0; x <= ub; x++){
                    double util = upgUtil(x, i, j);
                    if(util > max_util){
                        opt_from = i, opt_to = j, upg_num = x;
                        max_util = util;
                        has_util = true;
                    }
                    // std::cout << "x: " << x << " | util: " << util << "\n";
                }
                // std::cout << "has_util: " << has_util << " | max_util: " << max_util << "\n";
            }
        }
        if(!has_util) {
            break;
        }
        upg_info[{opt_from, opt_to}] += upg_num;
        upg_out[opt_from] += upg_num;
        upg_in[opt_to] += upg_num;
        // std::cout << "DEBUG | opt_from: " << opt_from
        //           << " | opt_to: " << opt_to
        //           << " | upg_num: " << upg_num
        //           << " | max_util: " << max_util
        //           << " | upg_info: " << upg_info << "\n";
        // std::cout << "--------------------------------\n";
    }

    // Compute total upgrade fee.
    double total_upg_fee = this->getUpgradeFee(plan.id, upg_info);
    
    // Compute expected revenue if we accept/reject.
    plan.upgrade_fee = total_upg_fee;
    plan.upgrade = upg_info;
    plan.computeUpgradedRooms();
    State state_accept = forceMinus(state, plan.upgraded_order);
    if(state_accept.hasMinus()){
        std::cout << "Error: State error in findOptPlanIP!!!\n";
        exit(1);
    }
    double rev_ind_acc = this->getRevDemand(demand, state_accept, prices);
    // std::cout << "DEBUG | rev_id_acc: " << rev_ind_acc << "\n";
    plan.exp_acc_togo = rev_ind_acc + order_price + total_upg_fee;
    plan.exp_rej_togo = this->getRevDemand(demand, state, prices);

    if(plan.exp_rej_togo >= plan.exp_acc_togo){
        plan.decide(false);
    } else {
        plan.decide(true);
    }
}

// guessDemandSP return a vector of demand we guess in this period
std::vector<State> MyopicPlanner::guessDemandSP(const int period){
    // Guess demands
    std::vector<State> s_demands;
    for(int w = 0; w < this->num_exper_SP_; w++){
        // Initialize
#ifdef __APPLE__
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
#elif _WIN32
        srand(w);
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count() / rand();
#endif

        std::default_random_engine generator (seed);
        std::uniform_real_distribution<double> distribution (0.0, 1.0);
        // std::cout << "DEBUG | w: " << w << " | seed: " << seed << "\n";
        // Guess demand
        State demand;
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                int guess_rooms = 0;
                int capacity = this->dat_->capacity.at(i);
                for(int t = period; t >= 1; t--){
                    
                    double prob = distribution(generator);
                    for(int c = 0; c <= capacity; c++){
                        if(prob <= this->demand_cdf_.at({t, s, i, c})){
                            guess_rooms += c;
                            break;
                        }
                    }
                }
                demand.rooms[{s, i}] = guess_rooms;
            }
        }
        demand.prob = static_cast<double>(1) / static_cast<double>(this->num_exper_SP_);
        // std::cout << "DEBUG: Scenario " << w << ": " << demand << "\n";
        s_demands.push_back(demand);
    }
    return s_demands;
}

// allPosDemand return all possible individual demand 
std::vector<State> MyopicPlanner::allPosIndDemand(const int period){
    std::vector<State> s_demands;
    
    // Init the set containing all possible state
    // Store the index of each level in dynamic nested loop
    // Each level from capacity[i] to 0, when the lower level run to 0,
    // then the one-level-upper level --1.
    // When the top level run to -1, then stop.
    State org_state(this->dat_->capacity);
    int num_states = 1;
    bool status = false;
    std::vector<data::tuple2d> state_key;
    std::map<data::tuple2d, int> cap_loss;

    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            state_key.push_back({s,i});
            num_states *= (this->dat_->capacity.at(i) + 1);
            cap_loss[{s,i}] = 0;
        }
    }
    while(!status){
        bool total_check = 1;

        for(auto& cap: cap_loss){
            bool check = (cap.second == this->dat_->capacity.at(std::get<1>(cap.first)));
            total_check *= check;
        }

        // test for exit condition
        if (total_check){
            status = true;
        }

        State demand(org_state, cap_loss);
        double prob = 1;
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                int num = demand.rooms.at({s, i});
                prob *= this->dat_->demand.prob_ind.at({period, s, i, num});
            }
        }
        demand.prob = prob;
        s_demands.push_back(demand);

        bool change = true;
        int c = int(cap_loss.size()) - 1;  // start from the most lower level loop
        while (change && c >= 0) {
            // increment the innermost variable and check if spill overs
            int room_type = std::get<1>(state_key[c]);
            if (++cap_loss[state_key[c]] > this->dat_->capacity.at(room_type)) {
                // reset lower loop index
                cap_loss[state_key[c]] = 0;
                // Value of higher loop +1
                change = true;
            }
            else{
                // If higher loop is not affected, then don't move
                change = false;
            }
            // move to the higher loop
            c = c - 1;
        }
    }
    // std::cout << "DEBUG: allPosIndDemand - s_demands: " << s_demands.size() << "\n";
    return s_demands;
}

// findOptPlanSP return the optimal revenue from the optimal decision to
// the given order, considering the demand we guess. This problem is formulated
// in an integer programming problem in this function.
void MyopicPlanner::findOptPlanSP(const int period, MyopicUpgradePlan& plan, 
                                  const State& state, const std::vector<State>& s_demands,
                                  const std::vector<std::map<data::tuple2d, double> >& prices){
    // Initialize all containters used in this function
    const auto& upg_to = this->dat_->upgrade_to;
    const auto& upg_from = this->dat_->upgrade_from;
    const auto& req_days = plan.days;
    const double order_price = plan.price;
    std::map<int, int> cap = state.findMinRooms(req_days); 

    std::map<int, int> req_rooms, upg_in, upg_out, must_upg;
    std::map<data::tuple2d, int> upg_info;
    std::map<data::tuple2d, double> upg_fee;
    int start_day = *req_days.begin();
    for(int i = 1; i <= State::num_room_type; i++){
        req_rooms[i] = plan.order.rooms.at({start_day, i});
        upg_in[i] = 0, upg_out[i] = 0;
        must_upg[i] = std::max(req_rooms.at(i) - cap.at(i), 0);
        for(const auto& j: upg_to.at(i)){
            upg_info[{i, j}] = 0;
            upg_fee[{i, j}] = this->dat_->upgrade_fee.at({plan.id, i, j});
        }
    }
    
    // std::cout << "DEBUG | cap: " << cap << "\n";
    // std::cout << "DEBUG | req_days: " << req_days << "\n";
    // std::cout << "DEBUG | req_rooms: " << req_rooms << "\n";
    // std::cout << "DEBUG | upg_to: " << upg_to << "\n";
    // std::cout << "DEBUG | upg_from: " << upg_from << "\n";
    // std::cout << "DEBUG | upg_in: " << upg_in << "\n";
    // std::cout << "DEBUG | upg_out: " << upg_out << "\n";
    // std::cout << "DEBUG | upg_fee: " << upg_fee << "\n";
    // std::cout << "DEBUG | upg_info: " << upg_out << "\n";
    // std::cout << "DEBUG | must_upg: " << must_upg << "\n";
    // std::cout << "DEBUG | order_price: " << order_price << "\n";

    auto getSpace = [&](int s, int i) -> int {
        return state.rooms.at({s, i}) - req_rooms.at(i) + upg_out.at(i) - upg_in.at(i);
    };
    auto getSpaces = [&](int i) -> std::map<int, int> {
        std::map<int, int> spaces;
        for(const auto& s: req_days){
            spaces[s] = getSpace(s, i);
        }
        return spaces;
    };
    auto getBNSpace = [&](int i) -> int {
        return cap.at(i) - req_rooms.at(i) + upg_out.at(i) - upg_in.at(i);
    };
    auto upgGain = [&](int num, int from, int to) -> double {
        return num * req_days.size() * upg_fee.at({from, to});
    };
    auto upgIndGain = [&](int num, int space, int ind, double price) -> double {
        if(space + num <= 0) return 0;
        if(space <= 0) return price * std::min(num + space, ind);
        if(space >= ind) return 0;
        return price * std::min(num, ind - space);
    };
    auto upgIndLoss = [&](int num, int space, int ind, double price) -> double {
        return price * (num - std::min(std::max(space - ind, 0), num));
    };
    auto upgUtil = [&](int num, int from, int to) -> double {
        if(num == 0){
            return 0;
        }
        double upg_gain = upgGain(num, from, to);
        double util = upg_gain;
        std::map<int, int> space_from = getSpaces(from);
        std::map<int, int> space_to = getSpaces(to);
        for(int w = 0; w < s_demands.size(); w++){
            double util_w = 0.0;
            int price_id = w;
            if(this->mode_ != MyopicMode::smart){
                price_id = 0;
            }
            for(const auto& s: req_days){
                int ind_from = s_demands[w].rooms.at({s, from});
                int ind_to = s_demands[w].rooms.at({s, to});
                double price_from = prices[price_id].at({s, from});
                double price_to = prices[price_id].at({s, to});
                double upg_gain_ind = upgIndGain(num, space_from.at(s), ind_from, price_from);
                double upg_loss_ind = upgIndLoss(num, space_to.at(s), ind_to, price_to);
                // std::cout << "s: " << s << " | upg_g: " << upg_gain << " | upg_gi: " << upg_gain_ind
                //           << " | upg_li: " << upg_loss_ind << "\n"; 
                util_w += upg_gain_ind - upg_loss_ind;
            }
            util += util_w * s_demands[w].prob;
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
    for(int i = State::num_room_type; i >= 1; i--){
        if(must_upg.at(i) <= 0) continue;

        // First compute lower bound of upgrade from type-i
        int lb = upgLB(i);
        while(lb > 0){
            int opt_to = 0, opt_num = 0;
            double min_loss_rate = INT_MAX;
            for(const auto& j: upg_to.at(i)){
                int ub = upgUB(i, j);
                // std::cout << "(" << i << "," << j << ") | LB: " << lb << " | UB: " << ub;
                std::map<int, int> spaces = getSpaces(j);
                for(int x = 1; x <= ub; x++){
                    double loss = 0.0;
                    for(int w = 0; w < s_demands.size(); w++){
                        int price_id = w;
                        if(this->mode_ != MyopicMode::smart){
                            price_id = 0;
                        }
                        double loss_w = 0.0;
                        for(const auto& s: req_days){
                            double ind = s_demands[w].rooms.at({s, j});
                            double price = prices[price_id].at({s, j});
                            loss_w += upgIndLoss(x, spaces.at(s), ind, price);
                        }
                        loss += loss_w * s_demands[w].prob;
                    }

                    double loss_rate = loss/static_cast<double>(x);
                    // std::cout << " | " << x << ": " << loss << " (" << loss_rate << ")";
                    // Find best upgrade plan
                    if((loss_rate < min_loss_rate) || 
                       (loss_rate == min_loss_rate && x > opt_num) ||
                       (loss_rate == min_loss_rate && x == opt_num && j < opt_to)){
                        opt_to = j, opt_num = x;
                        min_loss_rate = loss_rate;
                    }
                }
                // std::cout << "\n";
            }
            int upg_num = std::min(lb, opt_num);
            upg_info[{i, opt_to}] += upg_num;
            upg_out[i] += upg_num;
            upg_in[opt_to] += upg_num;
            int new_lb = upgLB(i);
            if(new_lb >= lb){
                std::cout << "Error: must upgrade error in findOptPlanSP!!!\n";
                exit(1);
            }
            lb = upgLB(i);
            // std::cout << "DEBUG | opt_to: " << opt_to
            //           << " | opt_num: " << opt_num
            //           << " | upg_num: " << upg_num
            //           << " | min_loss: " << min_loss_rate
            //           << " | upg_info: " << upg_info << "\n";
            // std::cout << "--------------------------------\n";
        }
    }

    // Find other upgrades if there has util
    bool has_util = true;
    while(has_util){
        int opt_from = 0, opt_to = 0, upg_num = 0;
        double max_util = 0.0;
        has_util = false;
        for(int i = 1; i <= State::num_room_type; i++){
            for(const auto& j: upg_to.at(i)){
                int ub = upgUB(i, j);
                // std::cout << "(" << i << "," << j  << ") | UB: " << ub << "\n";
                for(int x = 0; x <= ub; x++){
                    double util = upgUtil(x, i, j);
                    if(util > max_util){
                        opt_from = i, opt_to = j, upg_num = x;
                        max_util = util;
                        has_util = true;
                    }
                    // std::cout << "x: " << x << " | util: " << util << "\n";
                }
                // std::cout << "has_util: " << has_util << " | max_util: " << max_util << "\n";
            }
        }
        if(!has_util) {
            break;
        }
        upg_info[{opt_from, opt_to}] += upg_num;
        upg_out[opt_from] += upg_num;
        upg_in[opt_to] += upg_num;
        // std::cout << "DEBUG | opt_from: " << opt_from
        //           << " | opt_to: " << opt_to
        //           << " | upg_num: " << upg_num
        //           << " | max_util: " << max_util
        //           << " | upg_info: " << upg_info << "\n";
        // std::cout << "--------------------------------\n";
    }

    // Compute total upgrade fee.
    double total_upg_fee = this->getUpgradeFee(plan.id, upg_info);

     // Compute expected revenue if we accept/reject.
    plan.upgrade_fee = total_upg_fee;
    plan.upgrade = upg_info;
    plan.computeUpgradedRooms();
    State state_accept = forceMinus(state, plan.upgraded_order);
    if(state_accept.hasMinus()){
        std::cout << "Error: State error in findOptPlanIP!!!\n";
        exit(1);
    }
    double rev_ind_acc = 0, rev_ind_rej = 0;
    if(this->mode_ == MyopicMode::smart){
        for(int w = 0; w < s_demands.size(); w++){
            double rev_acc = this->getRevDemand(s_demands[w], state_accept, prices[w]);
            rev_ind_acc += rev_acc * s_demands[w].prob;
            double rev_rej = this->getRevDemand(s_demands[w], state, prices[w]);
            rev_ind_rej += rev_rej * s_demands[w].prob;
        }
    } else {
        for(int w = 0; w < s_demands.size(); w++){
            double rev_acc = this->getRevDemand(s_demands[w], state_accept, prices[0]);
            rev_ind_acc += rev_acc * s_demands[w].prob;
            double rev_rej = this->getRevDemand(s_demands[w], state, prices[0]);
            rev_ind_rej += rev_rej * s_demands[w].prob;
        }
    }
    
    // std::cout << "DEBUG | rev_id_acc: " << rev_ind_acc << "\n";
    plan.exp_acc_togo = rev_ind_acc + order_price + total_upg_fee;
    plan.exp_rej_togo = rev_ind_rej;

    if(plan.exp_rej_togo >= plan.exp_acc_togo){
        plan.decide(false);
    } else {
        plan.decide(true);
    }
}

// getIndPrices return the prices of individual customers given period
std::map<data::tuple2d, double> MyopicPlanner::getIndPrices(const int period){
    std::map<data::tuple2d, double> prices;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            prices[{s, i}] = this->dat_->price_cus.at({s, i});
        }
    }
    return prices;
}

// getExpDemand1P return the expected demand in the given single period.
State MyopicPlanner::getExpDemand1P(const int period){
    State demand;
    
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            double exp_num_rooms = 0;
            for(int c = 0; c <= this->dat_->capacity.at(i); c++){
                exp_num_rooms += c * this->dat_->demand.prob_ind.at({period, s, i, c});
            }
            demand.rooms[{s, i}] = round(exp_num_rooms);
        }
    }
    return demand;
}

// getReservedCap return the reserved capacities for future order.
// This function is mainly for reserved mode.
State MyopicPlanner::getReservedCap(const int period, const State& state){

    // Init the remain capacity at the end of this period
    State& demand = this->exp_demand_1p_.at(period);
    
    State remain = state - demand;
    
    // Init the expected reserved capacity
    std::map<data::tuple2d, double> exp_rsvd_cap;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            exp_rsvd_cap[{s, i}] = 0;
        }
    }

    // Compute expected reserved capacity in each period from now to the end.
    for(int t = period - 1; t >= 1; t--){
        
        // Init the expected reserved capacity in period t
        std::map<data::tuple2d, double> exp_rsvd_cap_t;
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                exp_rsvd_cap_t[{s, i}] = 0;
            }
        }
        // Compute plan of each order
        for(int k = 1; k <= this->dat_->scale.order_type; k++){
            
            if(!this->isAcceptable(k, remain)){
                continue;
            }
            MyopicUpgradePlan plan = this->orderToPlan(k);
            switch (this->model_type_) {
            case MyopicModelType::ip:{
                // std::cout << "DEBUG | call fOPIP from gRC\n";
                this->findOptPlanIP(t, plan, remain, this->guess_demand_md_.at(t),
                                    this->guess_price_md_.at(t));
                break;
            }
            case MyopicModelType::sp:{
                this->findOptPlanSP(t, plan, remain, this->guess_demand_ms_.at(t),
                                    this->guess_price_ms_.at(t));
                // std::cout << "DEBUG in gRC| (" << t << "," << k << ")"
                //           << " | decision: " << plan.decision 
                //           << " | revenue: " << plan.revenue << "\n"; 
                break;
            }}
            // Add the requested number of rooms in plan multiply the probability of
            // this period, if this plan will be accepted.
            if(plan.decision == true){
                double prod = this->dat_->demand.prob_order.at({t, k});
                // State upgraded_order = plan.getUpgradedRooms();
                for(int s: this->dat_->reqest_periods.at(plan.id)){
                    for(int i = 1; i <= State::num_room_type; i++){
                        int num = plan.upgraded_order.rooms.at({s, i});
                        exp_rsvd_cap_t[{s, i}] += prod * static_cast<double>(num);
                    
                    }
                }
            }
        }
        
        // Update remain and expected reserved capacities
        State& demand = this->exp_demand_1p_.at(period);
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                int num = round(exp_rsvd_cap_t.at({s, i})) + demand.rooms.at({s, i});
                remain.rooms[{s, i}] = std::max(remain.rooms[{s, i}] - num, 0);
                exp_rsvd_cap[{s, i}] += exp_rsvd_cap_t.at({s, i});
            }
        }
    }

    // Finally, round the expected reserved capacity and return.
    State reserved_cap;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            reserved_cap.rooms[{s, i}] = round(exp_rsvd_cap.at({s, i}));
        }
    }
    return reserved_cap;
}

// updateState update this->state_ with the given plan
void MyopicPlanner::updateState(State& state, const MyopicUpgradePlan& plan){

    if(plan.decision == false){
        return ;
    }

    // State upgraded_order = plan.getUpgradedRooms();

    // Update the given state
    for(int s: this->dat_->reqest_periods.at(plan.id)){
        for(int i = 1; i <= State::num_room_type; i++){
            state.rooms[{s, i}] -= plan.upgraded_order.rooms.at({s, i});
            if(state.rooms.at({s, i}) < 0){
                std::cout << "ERROR: Upgrade plan error in updateState()!!!\n";
                exit(1);
            }
        }
    }
}

// TODO: 尚未 debug
MyopicUpgradePlan MyopicPlanner::findOptPlanNC(const int period, const MyopicUpgradePlan& plan,
                                               const State& state){
    // Find available future orders with upgrade
    std::vector<MyopicUpgradePlan> future_orders;
    
    // To consider upgrade, we need the capacities after accepting and rejecting 
    // the order in this period.
    std::vector<State> states_nc;
    states_nc.push_back(state);
    // std::cout << "fOPNC | " << states_nc[0].rooms << "\n";
    MyopicUpgradePlan temp_plan(plan);
    if(this->model_type_ == MyopicModelType::ip){
        this->findOptPlanIP(period, temp_plan, state,
                            this->guess_demand_md_[period],
                            this->guess_price_md_[period]);
    } else if (this->model_type_ == MyopicModelType::sp){
        this->findOptPlanSP(period, temp_plan, state, 
                            this->guess_demand_ms_[period],
                            this->guess_price_ms_[period]);
    }
    State state_acc = forceMinus(state, temp_plan.upgraded_order);
    if(state_acc.hasMinus()){
        std::cout << "Error in findOptPlanNC when initializing state.\n";
        exit(1);
    }
    states_nc.push_back(state_acc);

    // // DEBUG
    // std::cout << "fOPNC | temp_plan: ";
    // temp_plan.print();
    // std::cout <<  "\n";
    // for(auto& plan: states_nc){
    //     std::cout << "fOPNC | " << plan.rooms << "\n";
    // }
    // std::cout << "fOPNC | upgrade future orders...\n";

    // Then try upgrading these future orders with and without considering
    // the order in this period.
    for(int n = period - 1; n >= 1; n--){
        // std::cout << "fOPNC | n: " << n << "-----------------------------------------------\n";
        for(auto& f_ord: this->compounded_orders_nc_.at(n)){
            for(auto& cap: states_nc){
                // DEBUG
                
                // std::cout << "fOPNC | cap: " << cap.rooms << "\n";
                // f_ord.print();

                if(!this->isAcceptable(f_ord, cap)){
                    // std::cout << "fOPNC | Not acceptable \n";
                    continue;
                } 
                // std::cout << "fOPNC | Acceptable \n";  
                // If this future order is acceptable, then try upgrade it.
                MyopicUpgradePlan future_order(f_ord);
                if(this->model_type_ == MyopicModelType::ip){
                    this->findOptPlanIP(period, future_order, cap, 
                                        this->guess_demand_md_[period],
                                        this->guess_price_md_[period]);
                } else if (this->model_type_ == MyopicModelType::sp){
                    this->findOptPlanSP(period, future_order, cap, 
                                        this->guess_demand_ms_[period],
                                        this->guess_price_ms_[period]);
                }
                // DEBUG
                // future_order.print();

                if(future_order.decision == false) continue;
                future_orders.push_back(future_order);
            }
        }
    }

    // Add an empty order into future_orders
    MyopicUpgradePlan empty_f_order;   
    future_orders.push_back(empty_f_order);
    // empty_f_order.print();

    // std::cout << "fOPNC | Reserve spaces...\n";

    // Reserve spaces for future orders and run
    std::vector<MyopicUpgradePlan> plans;
    // And find the best expected value togo
    int opt_plan_id = -1, plan_id = -1;
    double max_plan_rev = -1;
    // For each possible future order...
    for(auto& future_order: future_orders){
        // we reserved the space for it, and compute the upgraded plan for
        // the order comming in this period.
        State cap = forceMinus(state, future_order.upgraded_order);
        if(cap.hasMinus()){
            // std::cout << "Error in findOptPlanNC when reserving spaces for future orders.\n";
            exit(1);
        }
        MyopicUpgradePlan temp_plan(plan);
        if(!this->isAcceptable(plan, cap)) continue;
        if(this->model_type_ == MyopicModelType::ip){
            this->findOptPlanIP(period, temp_plan, cap, 
                                this->guess_demand_md_[period],
                                this->guess_price_md_[period]);
        } else if (this->model_type_ == MyopicModelType::sp){
            this->findOptPlanSP(period, temp_plan, cap, 
                                this->guess_demand_ms_[period],
                                this->guess_price_ms_[period]);
        }
        plans.push_back(temp_plan);
        plan_id ++;

        // std::cout << "\nfOPNC | space: " << cap.rooms << "\n";
        // std::cout << "fOPNC | future order: " << future_order.upgraded_order.rooms << "\n";
        // temp_plan.print();

        // Find the best expected_revenue_togo among each situations.
        double exp_val_togo = std::max(temp_plan.exp_acc_togo, temp_plan.exp_rej_togo)
                              + future_order.revenue;
        if(exp_val_togo > max_plan_rev){
            max_plan_rev = exp_val_togo;
            opt_plan_id = plan_id;
        }
        // std::cout << "fOPNC | exp_val_togo: " << exp_val_togo
        //           << " | max_plan_rev: " << max_plan_rev
        //           << " | opt_plan_id: " << opt_plan_id
        //           << "\n";
        
    }
    // std::cout << "fOPNC | opt plan: " << opt_plan_id << "\n";
    // plans[opt_plan_id].print();
    // std::cout << "fOPNC | opt plan printed!\n";


    // Finally return the optimal plan
    return plans[opt_plan_id];
}

// funcTest test the target function
void MyopicPlanner::funcTest(){
    this->resetState();
    this->storeGuessFuture();
    // this->storeCompoundedOrdersNC();
    int period = 10, order = 9;
    this->findBestRevOrder(period, order);
}

// findBaselineUB find the baseline in bs mode
double MyopicPlanner::findBaselineBS(const int period, const MyopicUpgradePlan& plan, 
                                     const State& state){

    // First, find the capacities of requested periods.
    std::map<int, int> cap = state.findMinRooms(plan.days);
    // Find the total number of rooms requested in one day in this plan
    int total_rooms = 0, start_day = *plan.days.begin();
    for(int i = 1; i <= State::num_room_type; i++){
        total_rooms += plan.order.rooms.at({start_day, i});
    }
    
    // Find the upgrade plan which upgrade as much as possible to the higher type.
    std::map<int, int> max_upg_order;
    for(int i = State::num_room_type; i >= 1; i--){
        int upg_num = std::min(total_rooms, cap.at(i));
        max_upg_order[i] = upg_num;
        total_rooms -= upg_num;
    }

    // Compute the price in the case we sold these room to individuals.
    double price = 0;
    for(const auto& s: plan.days){
        for(int i = 1; i <= State::num_room_type; i++){
            price += max_upg_order.at(i) * this->dat_->price_cus.at({s, i});
        }
    }

    

    // Compute the baseline based on ub and lb
    double baseline_ub = price;
    double baseline_lb = plan.exp_rej_togo 
                         - (plan.exp_acc_togo - plan.price - plan.upgrade_fee);
    double baseline = std::max(baseline_ub - baseline_lb, 0.0) * this->baseline_beta_
                      + baseline_lb;
    return baseline;
}

// decideBS decide whether to accept this plan (upgraded order) or not.
// It is only used in mode bs.
void MyopicPlanner::decideBS(MyopicUpgradePlan& plan, double baseline){
    if(plan.price + plan.upgrade_fee > baseline){
        plan.decide(true);
    } else {
        plan.decide(false);
    }
}



// findBestRevOrder return the best revenue obtained from random order 
double MyopicPlanner::findBestRevOrder(const int period, const int order){    

    // order 0 represent no order
    if(order == 0){
        return 0;
    }

    // Adjust state if the reserved mode was chosen.
    State state = this->state_;
    if(this->mode_ == MyopicMode::reserved){
        State reserved_cap = this->getReservedCap(period, this->state_);
        state = this->state_ - reserved_cap;
        // std::cout << "DEBUG | reserved: " << reserved_cap.rooms << "\n";
        // std::cout << "DEBUG | state_remain: " << state.rooms << "\n";
    }

    // First, check if this order is acceptable. 
    if(!this->isAcceptable(order, state)){
        this->acceptable_record_[{this->exper_id_, period}] = 0;
        return 0;
    }
    this->acceptable_record_[{this->exper_id_, period}] = 1;
    
    // Find upgrade plan
    MyopicUpgradePlan plan = this->orderToPlan(order);
    if(this->model_type_ == MyopicModelType::ip){
        if(this->mode_ == MyopicMode::nc){
            // Store the best plan
            plan = this->findOptPlanNC(period, plan, state);

        } else {
            this->findOptPlanIP(period, plan, state, 
                                this->guess_demand_md_[period],
                                this->guess_price_md_[period]);
        }
    }
    else if(this->model_type_ == MyopicModelType::sp){
        if(this->mode_ == MyopicMode::nc){
            plan = this->findOptPlanNC(period, plan, state);
        } else {
            this->findOptPlanSP(period, plan, state, 
                                this->guess_demand_ms_[period],
                                this->guess_price_ms_[period]);
        }
    }

    if(this->mode_ == MyopicMode::bs){
        double baseline = this->findBaselineBS(period, plan, state);
        this->decideBS(plan, baseline);
    }

    // Update this->state_
    this->updateState(this->state_, plan);

    // Record the decision
    this->rev_acc_record_[{this->exper_id_, period}] = plan.exp_acc_togo;
    this->rev_rej_record_[{this->exper_id_, period}] = plan.exp_rej_togo;
    this->decision_record_[{this->exper_id_, period}] = plan.decision;

    // Return the revenue
    return plan.revenue;
}

// randomDemand return the random demand from individuals
// Currently implemented in geometric distribution
State MyopicPlanner::randomDemand(const int period){
    // Guess demands
    // Initialize
#ifdef __APPLE__
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
#elif _WIN32
    srand(this->exper_id_);
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count() / (rand() * period);
#endif

    // std::cout << "DEBUG | exper: " << this->exper_id_ << " | period: " << period << " | seed: " << seed << "\n";
    std::default_random_engine generator (seed);
    std::uniform_real_distribution<double> distribution (0.0, 1.0);
    // Guess demand
    State demand;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            double prob = distribution(generator);
            // std::cout << "DEBUG | e: " << this->exper_id_ << " | t: " << period << " | seed: " << seed << " | prob: " << prob << "\n";
            int num = 0;
            int capacity = this->dat_->capacity.at(i);
            for(int c = 0; c <= capacity; c++){
                if(prob <= this->demand_cdf_.at({period, s, i, c})){
                    num = c;
                    break;
                }
            }
            demand.rooms[{s, i}] = num;
        }
    }
    return demand;
}

// getRevDemand return the revenue of random demand from individuals
double MyopicPlanner::getRevDemand(const State& demand){
    double revenue = 0;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            int num_demand = std::min(demand.rooms.at({s,i}), this->state_.rooms.at({s,i}));
            this->state_.rooms[{s, i}] -= num_demand;
            if(this->state_.rooms.at({s, i}) < 0){
                std::cout << "ERROR: State num error in MyopicPlanner::getRevDemand(period, demand) !!!\n";
                exit(1);
            }
            revenue += num_demand * this->dat_->price_cus.at({s,i});
        }
    }
    return revenue;
}

// getRevDemand return the revenue of random demand from individuals, given a state.
double MyopicPlanner::getRevDemand(const State& demand, const State& state){
    double revenue = 0;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            int num_demand = std::min(demand.rooms.at({s,i}), state.rooms.at({s,i}));
            revenue += num_demand * this->dat_->price_cus.at({s,i});
            
        }
    }
    return revenue;
}

// getRevDemand return the revenue of random demand from individuals, given a state, and price.
double MyopicPlanner::getRevDemand(const State& demand, const State& state, 
                                   const std::map<data::tuple2d, double> prices){
    double revenue = 0;
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            int num_demand = std::min(demand.rooms.at({s,i}), state.rooms.at({s,i}));
            revenue += num_demand * prices.at({s, i});
            // std::cout << "getRevDemand | {s,i}: {" << s << "," << i << "}"
            //           << " | demand: " << demand.rooms.at({s,i})
            //           << " | state: " << state.rooms.at({s,i}) 
            //           << " | num_demand: " << num_demand
            //           << " | price: " << this->dat_->price_cus.at({s,i})
            //           << " | revenue: " << num_demand * this->dat_->price_cus.at({s,i})
            //           << " | total_rev: " << revenue << "\n";
        }
    }
    return revenue;
}

// isNoAvlRoom return if there are available rooms
bool MyopicPlanner::isNoAvlRoom(){
    for(int s = 1; s <= State::num_service_period; s++){
        for(int i = 1; i <= State::num_room_type; i++){
            if(this->state_.rooms.at({s,i}) > 0){
                return false;
            }
        }
    }
    return true;
}

// 回來這裡
// experiment perfor 1 experiment from booking period |T| to 1,
// and return the revenue of this experiment.
double MyopicPlanner::experiment(){
    time_t start, end;
    time(&start);
    double revenue = 0;
    std::map<int, int> orders = this->randomOrder();
    
    for(int t = this->dat_->scale.booking_period; t >= 1; t--){
        // std::cout << "\nDEBUG | period " << t 
        //           << " 開始 ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        // time_t start, end;
        // time(&start);
        int order;
        if(this->random_first_){
            order = this->random_orders_.at({this->exper_id_, t});
        } else {
            // order = this->randomOrder(t);
            order = orders.at(t);
        }

        this->order_record_[{this->exper_id_, t}] = order;
        // time(&end);
        // std::cout << "DEBUG: order 進來 \n";
        // time(&start);
        double rev_order = this->findBestRevOrder(t, order);
        this->rev_order_record_[{this->exper_id_, t}] = rev_order;
        // exit(1);
        // time(&end);
        // std::cout << "DEBUG: 看怎麼接 \n";
        // time(&start);
        State demand;
        if(this->random_first_){
            demand = this->random_demands_.at({this->exper_id_, t});
        } else {
            demand = this->randomDemand(t);
        } 
        // time(&end);
        // std::cout << "DEBUG | Now state:\t" <<  this->state_.rooms << "\n"; 
        // std::cout << "DEBUG: demand 進來 \n";
        // time(&start);
        double rev_demand = this->getRevDemand(demand);
        this->rev_ind_record_[{this->exper_id_, t}] = rev_demand;
        // time(&end);
        // std::cout << "DEBUG: 算 demand rev \n";
        revenue += rev_order + rev_demand;
        
        // std::cout << "DEBUG | demand:\t\t" << demand.rooms << "\n";
        // std::cout << "DEBUG | Now state: \t" <<  this->state_.rooms << "\n"; 
        // std::cout << "DEBUG | rev_order: " << rev_order
        //           << " | rev_demand: " << rev_demand
        //           << " | revenue: " << revenue << "\n";

        // std::cout << "DEBUG: period " << t << " 結束-------------\n";
        if(this->isNoAvlRoom()){
            this->sold_out_record_[this->exper_id_] = t;
            // std::cout << "DEBUG: 沒空間ㄌ" << "\n";
            time(&end);
            this->time_record_[this->exper_id_] = difftime(end, start);
            return revenue;
        }
    }
    
    this->sold_out_record_[this->exper_id_] = 1;
    time(&end);
    this->time_record_[this->exper_id_] = difftime(end, start);
    return revenue;
}

// isAllPosIndDemLess return true if the number of all possible individual
// demand scenarios is less then this->num_exper_MS
bool MyopicPlanner::isAllPosIndDemLess(){
    int s = this->dat_->scale.service_period;
    int i = this->dat_->scale.room_type;
    int c = this->dat_->capacity.at(1);
    return (s * i <= log(this->num_exper_SP_)/log(c + 1));
}

// findBestOrderNC store the best orders in mode naively conservative
void MyopicPlanner::findBestOrderNC(){
    // First store price and discount of each order
    std::map<int, double> prices;
    std::map<int, double> discounts;
    for(int k = 1; k <= this->dat_->scale.booking_period; k++){
        prices[k] = this->dat_->price_order.at(k);
        double org_price = 0;
        for(const auto& s: this->dat_->reqest_periods.at(k)){
            for(int i = 1; i <= State::num_room_type; i++){
                org_price += this->dat_->price_cus.at({s, i}) 
                             * static_cast<double>(this->dat_->req_rooms.at({k, i}));
            }
        }
        discounts[k] = org_price - prices.at(k);
    }

    // Find those order on the boreto_front: higher price and lower discount
    std::map<int, bool> on_boreto_front;
    for(int k = 1; k <= this->dat_->scale.booking_period; k++){
        on_boreto_front[k] = true;
    }
    for(int k = 1; k < this->dat_->scale.booking_period; k++){
        if(!on_boreto_front.at(k)) continue;
        for(int h = k + 1; h <= this->dat_->scale.booking_period; h++){
            // check if k dominate h
            if(prices.at(k) >= prices.at(h) && discounts.at(k) <= discounts.at(h)){
                on_boreto_front[h] = false;
            }
            // check if k is dominated by h
            if(prices.at(h) >= prices.at(k) && discounts.at(h) <= discounts.at(k)){
                on_boreto_front[k] = false;
                break;
            }
        }
    }

    // Store the order with max price and order with min discounts on the boreto front
    int max_price = 0, max_order = 0, min_discount = INT_MAX, min_order = 0;
    for(int k = 1; k <= this->dat_->scale.booking_period; k++){
        if(!on_boreto_front.at(k)) continue;
        double price = prices.at(k), discount = discounts.at(k);
        if(price > max_price){
            max_price = price;
            max_order = k;
        }
        if(discount < min_discount){
            min_discount = discount;
            min_order = k;
        }
    }

    if(max_order == 0 || min_order == 0){
        std::cout << "Error in findBestOrderNC !!\n";
        exit(1);
    }

    this->best_order_nc_.insert(max_order);
    this->best_order_nc_.insert(min_order);
}

// orderToPlan return a plan instance of order
MyopicUpgradePlan MyopicPlanner::orderToPlan(const int order, const int num){
    MyopicUpgradePlan plan;
    plan.id = order;
    plan.days = this->dat_->reqest_periods.at(order);
    plan.price = this->dat_->price_order.at(order) * num;
    for(const auto& s: plan.days){
        for(int i = 1; i <= State::num_room_type; i++){
            plan.order.rooms[{s, i}] = this->dat_->req_rooms.at({order, i}) * num;
        }
    }

    return plan;
}

// storeCompoundedOrdersNC store the compounded orders in mode naively conservative
void MyopicPlanner::storeCompoundedOrdersNC(){
    this->findBestOrderNC();
    
    // Store compounded orders
    for(int n = 1; n < this->dat_->scale.booking_period; n++){
        this->compounded_orders_nc_[n].clear();
    }
    
    for(const auto& k: this->best_order_nc_){
        for(int n = 1; n < this->dat_->scale.booking_period; n++){
            this->compounded_orders_nc_[n].push_back(this->orderToPlan(k, n));
        }
    }
}

// storeGuessFuture store the guessed demand and price of each period
// in order to make better decision whether we should accept the order
// in each period.
void MyopicPlanner::storeGuessFuture(){

    switch (this->model_type_){
    case MyopicModelType::ip:{
        switch (this->mode_) {
        case MyopicMode::naive:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                this->guess_demand_md_[t] = this->guessInvDemand(t);
                this->guess_price_md_[t] = this->getIndPrices(t);
                // std::cout << "DEBUG: guess_demand: " << t << ": " << this->guess_demand_md_[t] << "\n";
                // std::cout << "DEBUG: guess_price: " << t << ": " << this->guess_price_md_[t] << "\n";
            }
            break;
        }
        case MyopicMode::smart:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                State demand = this->guessInvDemand(t);
                std::tuple<State, std::map<data::tuple2d, double> > guess 
                    = this->guessFutureDemand(t, demand);
                this->guess_demand_md_[t] = std::get<0>(guess);
                this->guess_price_md_[t] = std::get<1>(guess);
                // std::cout << "DEBUG: guess_demand: " << t << ": " << this->guess_demand_md_[t] << "\n";
                // std::cout << "DEBUG: guess_price: " << t << ": " << this->guess_price_md_[t] << "\n";
            }
            break;
        }
        case MyopicMode::reserved:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                this->guess_demand_md_[t] = this->guessInvDemand(t);
                this->guess_price_md_[t] = this->getIndPrices(t);
                this->exp_demand_1p_[t] = this->getExpDemand1P(t);
            }
            break;
        }
        case MyopicMode::nc:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                this->guess_demand_md_[t] = this->guessInvDemand(t);
                this->guess_price_md_[t] = this->getIndPrices(t);
                this->storeCompoundedOrdersNC();
            }
            break;
        }
        case MyopicMode::bs:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                this->guess_demand_md_[t] = this->guessInvDemand(t);
                this->guess_price_md_[t] = this->getIndPrices(t);
            }
            break;
        }}
        break;
    }
    case MyopicModelType::sp:{
        bool all_pos = this->isAllPosIndDemLess();
        switch (this->mode_) {
        case MyopicMode::naive:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                if(all_pos){
                    this->guess_demand_ms_[t] = this->allPosIndDemand(t);
                } else {
                    this->guess_demand_ms_[t] = this->guessDemandSP(t);
                }
                this->guess_price_ms_[t].push_back(this->getIndPrices(t));
            }
            break;
        }
        case MyopicMode::smart:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                std::vector<State> s_demands;
                if(all_pos){
                    s_demands = this->allPosIndDemand(t);
                    // this->num_exper_SP_ = s_demands.size();
                    
                } else {
                    s_demands = this->guessDemandSP(t);
                }
                this->guessFutureDemandSP(t, s_demands, 
                                        this->guess_demand_ms_[t],
                                        this->guess_price_ms_[t]);
            }
            break;
        }
        case MyopicMode::reserved:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                if(all_pos){
                    this->guess_demand_ms_[t] = this->allPosIndDemand(t);
                } else {
                    this->guess_demand_ms_[t] = this->guessDemandSP(t);
                }
                this->guess_price_ms_[t].push_back(this->getIndPrices(t));
                this->exp_demand_1p_[t] = this->getExpDemand1P(t);
            }
            break;
        }
        case MyopicMode::nc:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                if(all_pos){
                    this->guess_demand_ms_[t] = this->allPosIndDemand(t);
                } else {
                    this->guess_demand_ms_[t] = this->guessDemandSP(t);
                }
                this->guess_price_ms_[t].push_back(this->getIndPrices(t));
            }
            this->storeCompoundedOrdersNC();
            break;
        }
        case MyopicMode::bs:{
            for(int t = this->dat_->scale.booking_period; t >= 1; t--){
                if(all_pos){
                    this->guess_demand_ms_[t] = this->allPosIndDemand(t);
                } else {
                    this->guess_demand_ms_[t] = this->guessDemandSP(t);
                }
                this->guess_price_ms_[t].push_back(this->getIndPrices(t));
            }
            break;
        }}
        break;
    }}
}

// run() is the main function of MyopicPlanner
double MyopicPlanner::run(){
    this->resetState();
    this->storeGuessFuture();
    double sum_rev = 0;
    int num_run = 0, sum_thd = 0;
    for(int e = 0; e < this->num_experiment_; e++){
        // std::cout << "experiment " << e+1 << " start-------------------------------------------\n";
        this->exper_id_ = e + 1;
        double revenue = this->experiment();
        // std::cout << "DEBUG: experiment " << e+1 << "　結束-------------------------------------------\n";
        sum_rev += revenue;
        this->revenues_.push_back(revenue);
        this->avg_revs_[e + 1] = sum_rev / (e + 1);
        // std::cout << "DEBUG: Experiment " << e+1 << " | rev: " << revenue 
        //           << " | avg_rev: " << this->avg_revs_.at(e+1) << "\n";
        num_run = e;
        if (e >= 1){
            // std::cout << "結果： avg現在： " << this->avg_revs_.at(e + 1)
            //           << "結果： avg前一個： " << this->avg_revs_.at(e) << "\n";
            double dif = abs(this->avg_revs_.at(e + 1) - this->avg_revs_.at(e));
            dif = dif / this->avg_revs_.at(e);
            
            if (dif < this->threshold_){
                sum_thd += 1;
            } else {
                sum_thd = 0;
            }
            // std::cout << "exp: " << e+1 << " | rev: " << revenue 
            //           << " | avg_rev: " << avg_revs_.at(e+1)
            //           << " | dif: " << dif 
            //           << " | sum_thd: " << sum_thd << "\n"; 
            // std::cout << "e: " << e << " | 有 e+1 in avg_revs: " 
            //           << (this->avg_revs_.find(e+1) != this->avg_revs_.end()) << "\n";
        }
        if(sum_thd > this->early_stop_){
            // std::cout << "提早結束囉！\n";
            break;
        }
        this->resetState();
    }
    // std::cout << "要 return ㄌ，num_run = " << num_run << "\n";
    return this->avg_revs_.at(num_run + 1);
}

void MyopicPlanner::debug(){
    this->run(1);
    std::cout << "order: " << this->order_record_ << "\n";
    std::cout << "accept: " << this->acceptable_record_ << "\n";
    std::cout << "decision: " << this->decision_record_<< "\n";
    std::cout << "RA: " << this->rev_acc_record_ << "\n";
    std::cout << "RR: " << this->rev_rej_record_ << "\n";
    std::cout << "RO: " << this->rev_order_record_ << "\n";
    std::cout << "RI: " << this->rev_ind_record_ << "\n";
}

void MyopicPlanner::clearRecords(){
    this->revenues_.clear();
    this->avg_revs_.clear();
    this->time_record_.clear();
    this->acceptable_record_.clear();
    this->decision_record_.clear();
    this->rev_acc_record_.clear();
    this->rev_rej_record_.clear();
    this->rev_order_record_.clear();
    this->rev_ind_record_.clear();
    this->sold_out_record_.clear();
}

void MyopicPlanner::testNC(const std::string& scenario){
    // 先猜 order 和 demand，存起來
    this->num_experiment_ = 50;
    this->random_first_ = true;
    for(int e = 1; e <= this->num_experiment_; e++){
        for(int t = this->dat_->scale.booking_period; t >= 1; t--){
            this->exper_id_ = e;
            this->random_orders_[{e, t}] = this->randomOrder(t);
            this->random_demands_[{e, t}] = this->randomDemand(t);
        }
    }

    // models and modes
    std::vector<MyopicModelType> models;
    std::vector<MyopicMode> modes;
    models.push_back(MyopicModelType::ip);
    models.push_back(MyopicModelType::sp);
    modes.push_back(MyopicMode::naive);
    modes.push_back(MyopicMode::nc);
    std::map<MyopicModelType, std::string> model_name;
    std::map<MyopicMode, std::string> mode_name;
    model_name[MyopicModelType::ip] = "MD";
    model_name[MyopicModelType::sp] = "MS";
    mode_name[MyopicMode::naive] = "naive";
    mode_name[MyopicMode::nc] = "nc";

    std::string prefix = "debug/NC/";
    createFolder(prefix);
    std::string folder = prefix + scenario + "/";
    createFolder(folder);
    this->threshold_ = 0;
    for(auto& mt: models){
        for(auto& m: modes){
            std::cout << "Running " + model_name[mt] + " " + mode_name[m] + "...\n";
            this->model_type_ = mt;
            this->mode_ = m;
            this->run(this->num_experiment_, int(1000));
            std::string path = folder + model_name[mt] + "_" + mode_name[m] + ".csv";
            this->storeRevs(path);
            this->clearRecords();
            // auto avg_rev = myopic.getAvgRevs();
        }
    }
    
}

void MyopicPlanner::testBSBeta(const std::string& scenario){
    // 先猜 order 和 demand，存起來
    this->num_experiment_ = 1000;
    this->random_first_ = true;
    for(int e = 1; e <= this->num_experiment_; e++){
        for(int t = this->dat_->scale.booking_period; t >= 1; t--){
            this->exper_id_ = e;
            this->random_orders_[{e, t}] = this->randomOrder(t);
            this->random_demands_[{e, t}] = this->randomDemand(t);
        }
    }

    // models and modes
    std::vector<MyopicModelType> models;
    std::vector<MyopicMode> modes;
    models.push_back(MyopicModelType::ip);
    models.push_back(MyopicModelType::sp);
    modes.push_back(MyopicMode::naive);
    modes.push_back(MyopicMode::bs);
    std::map<MyopicModelType, std::string> model_name;
    std::map<MyopicMode, std::string> mode_name;
    model_name[MyopicModelType::ip] = "MD";
    model_name[MyopicModelType::sp] = "MS";
    mode_name[MyopicMode::naive] = "naive";
    mode_name[MyopicMode::bs] = "bs";

    std::string prefix = "debug/BS/";
    createFolder(prefix);
    std::string folder = prefix + scenario + "_poisson/";
    createFolder(folder);
    this->threshold_ = 0;
    for(auto& mt: models){
        for(auto& m: modes){
            if(m == MyopicMode::bs){
                for(int b = 10; b >= 0; b--){
                    double beta = 0.02 * b;
                    std::cout << "Running " + model_name[mt] + " " + mode_name[m] + " | beta: " + std::to_string(beta) + "...\n";
                    this->model_type_ = mt;
                    this->mode_ = m;
                    this->baseline_beta_ = beta;
                    this->run(this->num_experiment_, int(1000));
                    std::string path = folder + model_name[mt] + "_" + mode_name[m] + "_b" + std::to_string(2*b) + "_Near0N1000.csv";
                    this->storeRevs(path);
                    this->clearRecords();
                }
            } else {
                std::cout << "Running " + model_name[mt] + " " + mode_name[m] + "...\n";
                this->model_type_ = mt;
                this->mode_ = m;
                this->run(this->num_experiment_, int(1000));
                std::string path = folder + model_name[mt] + "_" + mode_name[m] + "_Near0N1000.csv";
                this->storeRevs(path);
                this->clearRecords();
            }
        }
    }
}

void MyopicPlanner::testSmartGamma(const std::string& scenario){
    // 先猜 order 和 demand，存起來
    this->random_first_ = true;
    for(int e = 1; e <= this->num_experiment_; e++){
        for(int t = this->dat_->scale.booking_period; t >= 1; t--){
            this->exper_id_ = e;
            this->random_orders_[{e, t}] = this->randomOrder(t);
            this->random_demands_[{e, t}] = this->randomDemand(t);
        }
    }

    // models and modes
    std::vector<MyopicModelType> models;
    std::vector<MyopicMode> modes;
    models.push_back(MyopicModelType::ip);
    models.push_back(MyopicModelType::sp);
    modes.push_back(MyopicMode::naive);
    modes.push_back(MyopicMode::smart);
    std::map<MyopicModelType, std::string> model_name;
    std::map<MyopicMode, std::string> mode_name;
    model_name[MyopicModelType::ip] = "MD";
    model_name[MyopicModelType::sp] = "MS";
    mode_name[MyopicMode::naive] = "naive";
    mode_name[MyopicMode::smart] = "smart";

    std::string folder = "debug/smart_gamma/M3_" + scenario + "/";
    createFolder(folder);
    this->threshold_ = 0;
    for(auto& mt: models){
        for(auto& m: modes){
            if(m == MyopicMode::smart){
                for(int g = 10; g >= 0; g--){
                    double gamma = 0.1 * g;
                    std::cout << "Running " + model_name[mt] + " " + mode_name[m] + " | gamma: " + std::to_string(gamma) + "...\n";
                    this->model_type_ = mt;
                    this->mode_ = m;
                    this->smart_gamma_ = gamma;
                    this->run(50, int(1000));
                    std::string path = folder + model_name[mt] + "_" + mode_name[m] + "_g" + std::to_string(g) + ".csv";
                    this->storeRevs(path);
                    this->clearRecords();
                }
            } else {
                std::cout << "Running " + model_name[mt] + " " + mode_name[m] + "...\n";
                this->model_type_ = mt;
                this->mode_ = m;
                this->run(50, int(1000));
                std::string path = folder + model_name[mt] + "_" + mode_name[m] + ".csv";
                this->storeRevs(path);
                this->clearRecords();
            }
        }
    }
}

void MyopicPlanner::test(const std::string& scenario){
    // 先猜 order 和 demand，存起來
    this->random_first_ = true;
    for(int e = 1; e <= this->num_experiment_; e++){
        for(int t = this->dat_->scale.booking_period; t >= 1; t--){
            this->exper_id_ = e;
            this->random_orders_[{e, t}] = this->randomOrder(t);
            this->random_demands_[{e, t}] = this->randomDemand(t);
        }
    }

    // models and modes
    std::vector<MyopicModelType> models;
    std::vector<MyopicMode> modes;
    models.push_back(MyopicModelType::ip);
    models.push_back(MyopicModelType::sp);
    modes.push_back(MyopicMode::naive);
    modes.push_back(MyopicMode::smart);
    modes.push_back(MyopicMode::reserved);
    std::map<MyopicModelType, std::string> model_name;
    std::map<MyopicMode, std::string> mode_name;
    model_name[MyopicModelType::ip] = "MD";
    model_name[MyopicModelType::sp] = "MS";
    mode_name[MyopicMode::naive] = "naive";
    mode_name[MyopicMode::smart] = "smart";
    mode_name[MyopicMode::reserved] = "reserved";

    std::string folder = "debug/compare/M3_" + scenario + "/";
    createFolder(folder);
    this->threshold_ = 0;
    for(auto& mt: models){
        for(auto& m: modes){
            std::cout << "Running " + model_name[mt] + " " + mode_name[m] + "...\n";
            this->model_type_ = mt;
            this->mode_ = m;
            this->run(50, int(1000));
            std::string path = folder + model_name[mt] + "_" + mode_name[m] + ".csv";
            this->storeRevs(path);
            this->revenues_.clear();
            this->avg_revs_.clear();
            this->time_record_.clear();
            this->acceptable_record_.clear();
            this->decision_record_.clear();
            this->rev_acc_record_.clear();
            this->rev_rej_record_.clear();
            this->rev_order_record_.clear();
            this->rev_ind_record_.clear();
            this->sold_out_record_.clear();
            // auto avg_rev = myopic.getAvgRevs();
        }
    }
    
}

double MyopicPlanner::run(const int num){
    this->setNumExpt(num);
    double avg_rev = this->run();
    return avg_rev;
}

double MyopicPlanner::run(const int num, const int num_sample){
    this->setNumExpt(num);
    this->setNumSample(num_sample);
    double avg_rev = this->run();
    return avg_rev;
}

double MyopicPlanner::run(const double threshold){
    this->setThreshold(threshold);
    double avg_rev = this->run();
    return avg_rev;
}
double MyopicPlanner::run(const int num, const double threshold){
    this->setNumExpt(num);
    this->setThreshold(threshold);
    double avg_rev = this->run();
    return avg_rev;
}

double MyopicPlanner::run(const int num, const int early_stop, const double threshold){
    this->setNumExpt(num);
    this->setThreshold(threshold);
    this->early_stop_ = early_stop;
    double avg_rev = this->run();
    return avg_rev;
}

double MyopicPlanner::run(const int num, const int num_sample,
                          const int early_stop, const double threshold){
    this->setNumExpt(num);
    this->setNumSample(num_sample);
    this->setThreshold(threshold);
    this->early_stop_ = early_stop;
    double avg_rev = this->run();
    return avg_rev;
}

MyopicPlanner::~MyopicPlanner(){
    this->dat_ = nullptr;
}

int NCR(int n, int r){
        if(r == 0) return 1;
        if(r > n/2) return NCR(n, n-r);

        long res = 1; 
        for (int k = 1; k <= r; ++k)
        {
            res *= n - k + 1;
            res /= k;
        }
        return res;
    }

// For debug
void MyopicPlanner::testGuessDemand(){
    this->num_exper_SP_ = 10000;
    // 先猜 demand
    this->storeGuessFuture();
    // frequency: freq[t][s, i][c] = 3
    std::map<int, std::map<data::tuple2d, std::map<int, int> > > freq;
    for(int t = this->dat_->scale.booking_period; t >= 1; t--){
        std::map<data::tuple2d, std::map<int, int> > demand;
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                std::map<int, int> cap;
                for(int c = 0; c <= this->dat_->capacity.at(i); c++){
                    cap[c] = 0;
                }
                demand[{s, i}] = cap;
            }
        }
        freq[t] = demand;
    }
    // 存 freq
    for(int t = this->dat_->scale.booking_period; t >= 1; t--){
        for(int w = 0; w < this->num_exper_SP_; w++){
            State& demand = this->guess_demand_ms_.at(t)[w];
            for(int s = 1; s <= State::num_service_period; s++){
                for(int i = 1; i <= State::num_room_type; i++){
                    int num = demand.rooms.at({s, i});
                    if(num > this->dat_->capacity.at(i)){
                        num = this->dat_->capacity.at(i);
                    }
                    freq[t][{s, i}][num] ++;
                }
            }
        }
    }
    // 試 print:
    std::ofstream out;
    for(int t = this->dat_->scale.booking_period; t >= 1; t--){
        std::string fold = "debug/S5I3C25m20N10000/";
        std::string path = fold + "freq10000_t" + std::to_string(t) + ".csv";
        out.open(path);
        std::string head = ""; 
        for(int s = 1; s <= State::num_service_period; s++){
            for(int i = 1; i <= State::num_room_type; i++){
                head += "{" + std::to_string(s) + "_" + std::to_string(i) + "}";
                if(s < State::num_service_period || i < State::num_room_type){
                    head += ",";
                } else{
                    head += "\n";
                }
            }
        }
        out << head;
        for(int c = 0; c <= this->dat_->capacity.at(1); c++){
            std::string msg = "";
            for(int s = 1; s <= State::num_service_period; s++){
                for(int i = 1; i <= State::num_room_type; i++){
                    msg += std::to_string(freq[t][{s, i}][c]);
                    if(s < State::num_service_period || i < State::num_room_type){
                        msg += ",";
                    } else{
                        msg += "\n";
                    }
                }
            }
            out << msg;
        }
        
        out.close();
    }    
}

void MyopicPlanner::printAvgRevs(){ 
    std::cout << "Print average revenues:\n";
    for(auto const& [key, value]: this->avg_revs_){
        std::cout << "First " << key << " experiments: " << value << "\n";
    }
}

void MyopicPlanner::storeRevs(std::string path){
    std::ofstream out;
    out.open(path);
    std::string head = "Experiment,revenue,first_k_avg_rev,time,sold_out";
    for(int t = this->dat_->scale.booking_period; t >= 1; t--){
        head += ",order_t" + std::to_string(t)
                + ",acceptable_t" + std::to_string(t)
                + ",decision_t" + std::to_string(t)
                + ",rev_acc_t" + std::to_string(t)
                + ",rev_rej_t" + std::to_string(t)
                + ",rev_order_t" + std::to_string(t)
                + ",rev_ind_t" + std::to_string(t);
    }
    head += "\n";
    out << head;
    
    for(auto const& m : this->avg_revs_){
        int e = m.first;
        std::string msg = std::to_string(e) + "," 
                        + std::to_string(this->revenues_[e-1]) + ","
                        + std::to_string(this->avg_revs_.at(e)) + ","
                        + std::to_string(this->time_record_[e]) + ","
                        + std::to_string(this->sold_out_record_.at(e));

        for(int t = this->dat_->scale.booking_period; t >= 1; t--){
            msg += "," + std::to_string(this->order_record_[{e, t}])
                    + "," + std::to_string(this->acceptable_record_[{e, t}])
                    + "," + std::to_string(this->decision_record_[{e, t}])
                    + "," + std::to_string(this->rev_acc_record_[{e, t}])
                    + "," + std::to_string(this->rev_rej_record_[{e, t}])
                    + "," + std::to_string(this->rev_order_record_[{e, t}])
                    + "," + std::to_string(this->rev_ind_record_[{e, t}]);
        }
        msg += "\n";
        out << msg;
    }
    out.close();
}

std::map<int, double> MyopicPlanner::getAvgRevs(){
    return this->avg_revs_;
}

// firstKAverage return the average of first k elements in an array
double firstKAverage(const std::vector<double> arr, const int num){
    double sum = 0;
    int add_num = std::min(num, int(arr.size()));
    for(int i = 0; i < add_num; i++){
        sum += arr[i];
    }
    return sum / add_num;
}

// getExpValNB return the expected value of negative binomial
// distribution, given parameters k and p. 
double getExpValNB(const int k, const double p){
    return (k * p) / (1 - p);
}

// getExpValGeo return the expected value of geometric distribution, given parameter p. 
double getExpValGeo(const double p){
    return 1/p;
}

} // namespace planner

std::ostream& operator<<(std::ostream& os, const planner::State& state){
    for(int s = 1; s <= planner::State::num_service_period; s++){
        for(int i = 1; i <= planner::State::num_room_type; i++){
            os << "[(" << s << "," << i << ")=" << state.rooms.at({s,i}) << "]";
        }
    }
    os << " | prob: " << state.prob;
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::vector<planner::State>& state_list){
    int num = 1;
    os << "[\n" ;
    for(auto& state: state_list){
        os << num << "." << state << "\n";
        num++;
    }
    os << "]\n";
    return os;
}

planner::State operator+(const planner::State& state1, const planner::State& state2){
    planner::State state;
    for(int s = 1; s <= planner::State::num_service_period; s++){
        for(int i = 1; i <= planner::State::num_room_type; i++){
            state.rooms[{s,i}] = state1.rooms.at({s,i}) + state2.rooms.at({s,i});
        }
    }
    return state;
}

// return max(state1 - state2, 0)
planner::State operator-(const planner::State& state1, const planner::State& state2){
    planner::State state;
    for(int s = 1; s <= planner::State::num_service_period; s++){
        for(int i = 1; i <= planner::State::num_room_type; i++){
            state.rooms[{s,i}] = std::max(state1.rooms.at({s,i}) - state2.rooms.at({s,i}), 0);
        }
    }
    return state;
}

// forceMinus return state1 - state2, allow negative number
planner::State forceMinus(const planner::State& state1, const planner::State& state2){
    planner::State state;
    for(int s = 1; s <= planner::State::num_service_period; s++){
        for(int i = 1; i <= planner::State::num_room_type; i++){
            state.rooms[{s,i}] = state1.rooms.at({s,i}) - state2.rooms.at({s,i});
        }
    }
    return state;
}
