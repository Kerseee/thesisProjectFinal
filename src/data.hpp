//
//  data.hpp
//  thesisDP
//
//  Created by 黃楷翔 on 2021/3/7.
//  Copyright © 2021年 黃楷翔. All rights reserved.
//

#ifndef data_h
#define data_h

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>



namespace data {
    
typedef float price;
typedef std::tuple<int, int> tuple2d;
typedef std::tuple<int, int, int> tuple3d;
typedef std::tuple<int, int, int, int> tuple4d;


//Scale stores scales of this problem.
struct Scale {
    int room_type;      //number of room type(n)
    int order_type;     //k
    int booking_period; //tau
    int service_period; //delta, equals to service_day
    int before;
    int booking_day;
    Scale();
    Scale(const int num_room_type, const int num_order_type,
          const int num_booking_period, const int num_service_period,
          const int before, const int booking_day);
    Scale(const Scale& scale);
    Scale& operator=(const Scale& scale);
};


// readCsv() read data from path given number of rows and columns,
// and then store things into container.
void readCsv(const std::string path, std::map<int, double>& container);

// readCsv() read data from path given number of rows and columns,
// and then store things into container.
void readCsv(const std::string path, std::map<tuple2d, double>& container);

// tokenize() tokenize the input str by delim and store results into vec.
void tokenize(std::string const &str, const char delim,
              std::vector<std::string> &vec);

void fileOpenCheck(std::ifstream& input, const std::string& path);

// CaseData store all data in case study.
struct CaseData {
    /* Variables */
    std::string folder;
    Scale scale;
    // Keys and values of following maps are shown after the statements.
    std::map<int, double> prob_before;      // prob_before[day] = prob
    std::map<tuple2d, double> prob_night;   // prob_night[{before, day}] = prob
    // prob_request and prob_ind_demand:
    // prob_[room_type][{before, day}] = prob
    std::map<int, std::map<tuple2d, double> > prob_request; 
    std::map<int, std::map<tuple2d, double> > prob_ind_demand;
    // prob_discount[{before, discount}] = prob
    std::map<tuple2d, double> prob_discount;
    // price_ind[{room_typeservice_day}] = price
    std::map<tuple2d, double> price_ind;

    /* Methods */
    // Read all inputs in the folders.
    void readMetaData(const std::string& path);
    void readAllData(const std::string& folder);
    void printAll();
};



//ProbDemand stores probability of demands.
struct ProbDemand {
    std::map<tuple2d, double> prob_order;           //Pr(t, k)
    std::map<tuple4d, double> prob_ind;    //Pr(t, s, i, c_i)
    void readDemandProb(std::string path);
    ProbDemand();
    ProbDemand(const ProbDemand& prob_demand);
    ProbDemand& operator=(const ProbDemand& prob_demand);
};

//Data stores all deterministic sets and parameters.
struct Params {
    Scale scale;
    std::map<int, std::set<int> > reqest_periods;   // S_k
    std::map<int, std::set<int> > upgrade_to;       // I_i
    std::map<int, std::set<int> > upgrade_from;     // I_i'
    std::map<tuple2d, price> price_cus;             // p_si
    std::map<int, price> price_order;               // p_k
    std::map<tuple3d, price> upgrade_fee;           // p_kii'
    std::map<tuple3d, int> demand_order;            // d_ksi
    std::map<tuple2d, int> req_rooms;    // demand rooms of order k, i
    std::map<int, int> checkin;                     // s^B_k
    std::map<int, int> checkout;                    // s^E_k
    std::map<int, int> capacity;                    // c_i
    ProbDemand demand;
    
    Params();
    Params(std::string param_path, std::string prob_path);
    Params(const Params& param);
    Params& operator=(const Params& param);
    void readParams(std::string path);
    void print();
};

} // namespace data

std::ostream& operator<<(std::ostream& os, const data::Scale& scale);
std::ostream& operator<<(std::ostream& os, const data::tuple2d& tup);
std::ostream& operator<<(std::ostream& os, const data::tuple3d& tup);
std::ostream& operator<<(std::ostream& os, const data::tuple4d& tup);
std::ostream& operator<<(std::ostream& os, const std::set<int>& s);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple2d, int>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple3d, int>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple4d, int>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple2d, data::price>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple3d, data::price>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple4d, data::price>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple2d, double>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple3d, double>& m);
std::ostream& operator<<(std::ostream& os, const std::map<data::tuple4d, double>& m);
std::ostream& operator<<(std::ostream& os, const std::map<int, int>& m);
std::ostream& operator<<(std::ostream& os, const std::map<int, double>& m);
std::ostream& operator<<(std::ostream& os, const std::map<int, data::price>& m);
std::ostream& operator<<(std::ostream& os, const std::map<int, std::set<int> >& m);

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& vec);

#endif /* data_h */
