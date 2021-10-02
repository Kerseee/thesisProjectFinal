#ifndef caseData_h
#define caseData_h

#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>
#include "data.hpp"

namespace data {

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
    // price_ind[{room_type, service_day}] = price
    std::map<tuple2d, double> price_ind;

    /* Methods */
    // Read the metadata of this group of data. Called by readAllData()
    void readMetaData(const std::string& path);
    // Read all inputs in the folders.
    void readAllData(const std::string& folder);
    // Print all the data
    void printAll();
};

// Order stores information of an order from a travel agency.
struct Order {
    int id;
    double price;
    std::set<int> request_days;
    std::map<int, int> request_rooms;
    std::map<tuple2d, double> upgrade_price;
};

// RandomOrderGenerator generate random orders from travel agencies.
class RandomOrderGenerator {

};

// RandomIndDemandGenerator generate random demand from individual customers.
class RandomIndDemandGenerator {

};

// Experimentor execute an experiment based on given data and random demands.
class Experimentor {

};


class MyopicCasePlanner {

};



}

#endif /* caseData_h */