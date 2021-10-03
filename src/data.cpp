//
//  data.cpp
//  thesisDP
//
//  Created by 黃楷翔 on 2021/3/8.
//  Copyright © 2021年 黃楷翔. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include <fstream>
#include <map>
#include <tuple>
#include <vector>
#include "data.hpp"

namespace data{

Scale::Scale(){
    this->room_type = 0;
    this->order_type = 0;
    this->booking_period = 0;
    this->service_period = 0;
}
Scale::Scale(const int num_room_type, const int num_order_type,
             const int num_booking_period, const int num_service_period){
    this->room_type = num_room_type;
    this->order_type = num_order_type;
    this->booking_period = num_booking_period;
    this->service_period = num_service_period;
}
    
Scale::Scale(const Scale& scale){
    operator=(scale);
}
Scale& Scale::operator=(const Scale& scale){
    this->room_type = scale.room_type;
    this->order_type = scale.order_type;
    this->booking_period = scale.booking_period;
    this->service_period = scale.service_period;
    return *this;
}

ProbDemand::ProbDemand(){
    ;
}
ProbDemand::ProbDemand(const ProbDemand& prob_demand){
    operator=(prob_demand);
}
ProbDemand& ProbDemand::operator=(const ProbDemand& prob_demand){
    this->prob_ind = prob_demand.prob_ind;
    this->prob_order = prob_demand.prob_order;
    return *this;
}
    
// Input probability data into demand.
void ProbDemand::readDemandProb(std::string path){
    std::ifstream input(path);
    
    // Read probability of order k
    int num_order = 0, num_booking_period;
    input >> num_order >> num_booking_period;
    for(int t = 1; t <= num_booking_period; t++){
        double sum_prob_order = 0;
        for(int k = 1; k <= num_order; k++){
            input >> this->prob_order[{t,k}];
            sum_prob_order += this->prob_order.at({t,k});
        }
        this->prob_order[{t,0}] = 1 - sum_prob_order;
    }
    
    // Read probabilities of demands of individual customers
    int num_service_period, num_room_type = 0;
    input >> num_service_period >> num_room_type;
    std::map<int, int> capacity;
    for(int i = 1; i <= num_room_type; i++){
        input >> capacity[i];
    }
    for(int t = 1; t <= num_booking_period; t++){
        for(int s = 1; s <= num_service_period; s++){
            int booking_period = 0, service_period = 0;
            input >> booking_period >> service_period;
            for(int i = 1; i <= num_room_type; i++){
                for(int c = 0; c <= capacity[i]; c++){
                    input >> this->prob_ind[{booking_period, service_period, i, c}];
                }
            }
        }
    }
    
    input.close();
}
    
// Input data to the structure 'Data' from a file.
void Params::readParams(std::string path){
    std::ifstream input(path);
    input >> this->scale.room_type >> this->scale.order_type
          >> this->scale.booking_period >> this->scale.service_period;
    
    // Read requested periods of order type k
    for(int k = 1; k <= this->scale.order_type; k++){
        input >> this->checkin[k] >> this->checkout[k];
        this->reqest_periods[k] = {};
        for(int s = this->checkin[k]; s <= this->checkout[k]; s++){
            this->reqest_periods[k].insert(s);
        }
    }
    
    // Read upgrade set of i
    for(int i = 1; i <= this->scale.room_type; i++){
        this->upgrade_from[i] = {};
    }
    for(int i = 1; i <= this->scale.room_type; i++){
        int num_upgrade = 0;
        input >> num_upgrade;
        this->upgrade_to[i] = {};
        for(int j = 1; j <= num_upgrade; j++){
            int upgrade_index = 0;
            input >> upgrade_index;
            this->upgrade_to[i].insert(upgrade_index);
            this->upgrade_from[upgrade_index].insert(i);
        }
    }
    
    // Read price of order type k
    for(int k = 1; k <= this->scale.order_type; k++){
        input >> this->price_order[k];
    }
    
    // Read upgrade fee
    for(int k = 1; k <= this->scale.order_type; k++){
        for(int i = 1; i <= this->scale.room_type; i++){
            int org_room = 0;
            input >> org_room;
            for(int upgrade_room: this->upgrade_to[org_room]){
                input >> this->upgrade_fee[{k, org_room, upgrade_room}];
            }
        }
    }
    
    // Read requesed rooms of order k
    for(int k = 1; k <= this->scale.order_type; k++){
        for(int i = 1; i <= this->scale.room_type; i++){
            int num_rooms = 0;
            input >> num_rooms;
            this->req_rooms[{k, i}] = num_rooms;
            for(int s: this->reqest_periods[k]){
                this->demand_order[{k, s, i}] = num_rooms;
            }
        }
    }
    
    // Read capacity of room type i
    for(int i = 1; i <= this->scale.room_type; i++){
        input >> this->capacity[i];
    }
    
    // Read price of individual demand
    for(int s = 1; s <= this->scale.service_period; s++){
        for(int i = 1; i <= this->scale.room_type; i++){
            input >> this->price_cus[{s, i}];
        }
    }
    input.close();
}

Params::Params(){
    ;
}
    
// Read all parameters and stochastic demand through data file and demand probability file.
Params::Params(std::string param_path, std::string prob_path){
    this->readParams(param_path);
    this->demand.readDemandProb(prob_path);
}
Params::Params(const Params& param){
    operator=(param);
}
Params& Params::operator=(const Params& param){
    this->scale = param.scale;
    this->demand = param.demand;
    this->reqest_periods = param.reqest_periods;
    this->upgrade_to = param.upgrade_to;
    this->price_cus = param.price_cus;
    this->price_order = param.price_order;
    this->upgrade_fee = param.upgrade_fee;
    this->demand_order = param.demand_order;
    this->checkin = param.checkin;
    this->checkout = param.checkout;
    this->capacity = param.capacity;
    return *this;
}

void Params::print(){
    ;
}

}// namespace data

std::ostream& operator<<(std::ostream& os, const data::Scale& scale){
    os << "room_type: " << scale.room_type << "\n"
       << "booking_period: " << scale.booking_period << "\n"
       << "service_period: " << scale.service_period << "\n"
       << "order_type: " << scale.order_type;
    return os;
}

std::ostream& operator<<(std::ostream& os, const data::tuple2d& tup){
    os << "(" << std::get<0>(tup) << "," << std::get<1>(tup) << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const data::tuple3d& tup){
    os << "(" << std::get<0>(tup) << "," << std::get<1>(tup) << "," << std::get<2>(tup) << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const data::tuple4d& tup){
    os << "(" << std::get<0>(tup) << "," << std::get<1>(tup) << ","
       << std::get<2>(tup) << "," << std::get<3>(tup) << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::map<data::tuple4d, data::price>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::map<data::tuple2d, int>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple3d, int>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple4d, int>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple2d, data::price>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple3d, data::price>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<int, int>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<int, data::price>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::set<int>& s){
    os << "{";
    for(auto& e: s){
        os << e << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<int, std::set<int>>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple2d, double>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple3d, double>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple4d, double>& m){
    os << "{";
    for(auto& s: m){
        os << s.first << ":" << s.second << ",";
    }
    os << "}";
    return os;
}
std::ostream& operator<<(std::ostream& os, const std::map<int, double>& m){
    os << "{";
    for(auto& e: m){
        os << "[" << e.first << ":" << e.second << "]";
    }
    os << "}";
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::map<int, std::map<int, double> >& m){
    for(auto& e: m){
        os << "{" << e.first << ":" << e.second << "}\n";
    }
    return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec){
    for(auto& e: vec){
        os << "[" << e << "]";
    }
    return os;
}
template std::ostream& operator<<(
    std::ostream& os, const std::vector<std::string>& vec);
