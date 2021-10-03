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
void readCsv(const std::string path, 
             std::map<int, std::map<int, double> >& container);

// readCsv() read data from path given number of rows and columns,
// and then store things into container.
void readCsv(const std::string path, std::map<tuple2d, double>& container);

// tokenize() tokenize the input str by delim and store results into vec.
void tokenize(std::string const &str, const char delim,
              std::vector<std::string> &vec);

void fileOpenCheck(std::ifstream& input, const std::string& path);

// Todo: 10/3 map 全改一維
// CaseData store all data in case study.
struct CaseData {
    /* Variables */
    std::string folder;
    Scale scale;

    // Keys and values of following maps are shown after the statements.
    std::map<int, double> prob_before;      // prob_before[day] = prob
    // prob_night[before][day] = prob
    std::map<int, std::map<int, double> > prob_night;

    // prob_request and prob_ind_demand:
    // prob_[room_type][before][day] = prob
    std::map<int, std::map<int, std::map<int, double> > > prob_request; 
    std::map<int, std::map<int, std::map<int, double> > > prob_ind_demand;
    // prob_discount[before][discount] = prob
    std::map<int, std::map<int, double> > prob_discount;
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

}

#endif /* caseData_h */