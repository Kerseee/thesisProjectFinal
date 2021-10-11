#ifndef caseData_hpp
#define caseData_hpp

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

struct CaseScale {
    int room_type;
    int service_period;
    int booking_period;
    int before;
    int booking_day;
    int periods_per_day;

    CaseScale();
    CaseScale(const CaseScale& scale);
    CaseScale& operator=(const CaseScale& scale);
    
    // refreshBookingVars() refresh the variables about booking stage
    void refreshBookingVars();

    // Check if given day is in service stage
    bool isServicePeriod(const int day) const;
    // Check if given day is in booking stage
    bool isBookingDay(const int day) const;
    // Check if given day is in booking stage
    bool isBookingPeriod(const int period) const;

    // getBookingDay return the booking day given booking period
    // Return -1 booking_period is not valid
    int getBookingDay(const int booking_period) const;
    // getServicePeriod return the serice period given booking day and before.
    // Return -1 if booking_day or before is not valid
    int getServicePeriod(const int booking_day, const int before) const;
    // getBefore return the before given booking day and service period.
    // Return -1 if booking_day or service_period is not valid
    int getBefore(const int booking_day, const int service_period) const;
};


// CaseData store all data in case study.
struct CaseData {
    /* Variables */
    std::string folder;
    CaseScale scale;

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
    
    // price_ind[{service_day, room_type}] = price
    std::map<tuple2d, double> price_ind;
    std::map<int, int> capacity;

    // upgrade info
    std::map<int, std::set<int> > upgrade_upper;
    std::map<int, std::set<int> > upgrade_lower;
    std::set<tuple2d> upgrade_pairs;

    /* Methods */
    // Read the metadata of this group of data. Called by readAllData()
    void readMetaData(const std::string& path);
    // Refresh variables about upgrade information. Called by readAllData()
    void refreshUpgradeInfo();
    // Read all inputs in the folders.
    void readAllData(const std::string& folder);
    // Print all the data
    void printAll() const;
};

}

std::ostream& operator<<(std::ostream& os, const data::CaseScale& scale);

#endif /* caseData_hpp */