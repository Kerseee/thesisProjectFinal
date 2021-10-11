#include <iostream>
#include <stdio.h>
#include <fstream>
#include <map>
#include <tuple>
#include <vector>
#include "data.hpp"
#include "caseData.hpp"

namespace data{

// tokenize() tokenize the input str by delim and store results into vec.
void tokenize(std::string const &str, const char delim,
              std::vector<std::string> &vec){
    size_t start;
    size_t end = 0;
 
    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        vec.push_back(str.substr(start, end - start));
    }
}

void fileOpenCheck(std::ifstream& input, const std::string& path){
    if(!input.is_open()){
        std::cout << "Error: Cannot open file " << path << "!\n";
        exit(1);
    }
}

// readCsv() read data from path given number of rows and columns,
// and then store things into container.
void readCsv(const std::string path, std::map<int, double>& container){

    std::ifstream input(path);
    std::string line;

    // Check if file is open
    fileOpenCheck(input, path);

    bool readTitle = true;
    while(std::getline(input, line, '\n')){
        // Drop the title
        if(readTitle){
            readTitle = false;
            continue;
        }

        // Tokenize
        std::vector<std::string> vec;
        tokenize(line, ',', vec);

        // Put tokenized data into map
        int key = std::stoi(vec[0]);
        double value = std::stod(vec[1]);
        container[key] = value;
    }
    input.close();
}

// readCsv() read data from path given number of rows and columns,
// and then store things into container.
void readCsv(const std::string path, 
             std::map<int, std::map<int, double> >& container){
    std::ifstream input(path);
    std::string line;

    // Check if file is open
    fileOpenCheck(input, path);

    bool readTitle = true;
    std::vector<int> columns;
    while(std::getline(input, line, '\n')){
        // Tokenize
        std::vector<std::string> vec;
        tokenize(line, ',', vec);

        // Store titles of columns
        if(readTitle){
            // for title of indexs
            columns.push_back(0);
            for(int i = 1; i < vec.size(); i++){
                columns.push_back(std::stoi(vec[i]));
            }
            readTitle = false;
            continue;
        }

        // Put tokenized data into map
        int row_id = std::stoi(vec[0]);
        std::map<int, double> row_container;
        for(int i = 1; i < vec.size(); i++){
            int col_id = columns[i];
            row_container[col_id] = std::stod(vec[i]);
        }
        container[row_id] = row_container;
    }
    input.close();
}

// readCsv() read data from path given number of rows and columns,
// and then store things into container.
void readCsv(const std::string path, std::map<tuple2d, double>& container){
    std::ifstream input(path);
    std::string line;

    // Check if file is open
    fileOpenCheck(input, path);

    bool readTitle = true;
    std::vector<int> columns;
    while(std::getline(input, line, '\n')){
        // Tokenize
        std::vector<std::string> vec;
        tokenize(line, ',', vec);

        // Store titles of columns
        if(readTitle){
            // for title of indexs
            columns.push_back(0);
            for(int i = 1; i < vec.size(); i++){
                columns.push_back(std::stoi(vec[i]));
            }
            readTitle = false;
            continue;
        }

        // Put tokenized data into map
        int row_id = std::stoi(vec[0]);
        for(int i = 1; i < vec.size(); i++){
            int col_id = columns[i];
            container[{row_id, col_id}] = std::stod(vec[i]);
        }
    }
    input.close();
}

CaseScale::CaseScale() {
    this->room_type = 0;
    this->service_period = 0;
    this->booking_period = 0;
    this->before = 0;
    this->booking_day = 0;
    this->periods_per_day = 0;
}

CaseScale::CaseScale(const CaseScale& scale){
    operator=(scale);
}

CaseScale& CaseScale::operator=(const CaseScale& scale){
    this->room_type = scale.room_type;
    this->booking_period = scale.booking_period;
    this->service_period = scale.service_period;
    this->before = scale.before;
    this->booking_day = scale.booking_day;
    this->periods_per_day = scale.periods_per_day;
    return *this;
}

// refreshBookingVars() refresh the variables about booking stage
void CaseScale::refreshBookingVars(){
    this->booking_period = (this->booking_day + 1) * this->periods_per_day;
}

// Check if given day is in service stage
bool CaseScale::isServicePeriod(int day) const{
    return day >= 1 && day <= this->service_period;
}

// Check if given day is in booking stage
bool CaseScale::isBookingDay(const int day) const{
    return day >= 0 && day <= this->booking_day;
}

// Check if given day is in booking stage
bool CaseScale::isBookingPeriod(const int period) const{
    return period >= 1 && period <= this->booking_period;
}

// getBookingDay return the booking day given booking period
int CaseScale::getBookingDay(const int booking_period) const {
    if(!this->isBookingPeriod(booking_period)) return -1;
    int booking_day = (booking_period - 1) / this->periods_per_day;
    return booking_day;
}
// getServicePeriod return the serice period given booking day and before.
int CaseScale::getServicePeriod(
    const int booking_day, const int before) const 
{
    if(!this->isBookingDay(booking_day) || before < 0) return -1;
    return before - booking_day + 1;
}

// getBefore return the before given booking day and service period.
int CaseScale::getBefore(
    const int booking_day, const int service_period) const
{
    if(!this->isBookingDay(booking_day) || 
       !this->isBookingPeriod(service_period)
    ){
        return -1;
    }
    return booking_day + service_period - 1;
}

void CaseData::readMetaData(const std::string& path){
    std::ifstream input(path);
    std::string item;
    // Check if file is open
    fileOpenCheck(input, path);
    // Read meta data
    input >> item >> this->scale.room_type
          >> item >> this->scale.before
          >> item >> this->scale.booking_day
          >> item >> this->scale.periods_per_day
          >> item >> this->scale.service_period
          >> item;
    // Refresh variables about booking stages
    this->scale.refreshBookingVars();
    // Read capacities
    for(int i = 1; i <= scale.room_type; i++){
        input >> this->capacity[i];
    }
}

// Refresh variables about upgrade information. Called by readAllData()
void CaseData::refreshUpgradeInfo(){
    for(int i = 1; i <= this->scale.room_type; i++){
        for(int j = i + 1; j <= this->scale.room_type; j++){
            this->upgrade_upper[i].insert(j);
            this->upgrade_lower[j].insert(i);
            this->upgrade_pairs.insert({i, j});
        }
    }
}

// Read all inputs in the folders.
void CaseData::readAllData(const std::string& folder){
    // Read metadata
    this->folder = folder;
    this->readMetaData(folder + "metadata.txt");
    
    const std::map<int, std::string> ROOM_NAMES = {
        {1, "exquisite"},
        {2, "superior"},
        {3, "deluxe"},
    };

    // Read probs of before and night
    readCsv(folder + "before_prob.csv", this->prob_before);
    readCsv(folder + "night_prob.csv", this->prob_night);
    
    // Read probs of demand
    for(int r = 1; r <= this->scale.room_type; r++){
        readCsv(folder + ROOM_NAMES.at(r) + "_request_prob.csv", 
                this->prob_request[r]);
        readCsv(folder + ROOM_NAMES.at(r) + "_demand_ind_prob.csv", 
                this->prob_ind_demand[r]);
    }
    
    // Read probs of discount
    readCsv(folder + "discount_prob.csv", this->prob_discount);

    // Read price for individual customers
    readCsv(folder + "p_si.csv", this->price_ind);

    // Refresh the upgrade information
    this->refreshUpgradeInfo();
}

void CaseData::printAll() const {
    std::cout << "Input folder: " << this->folder << "\n"
        << "Scale ----------------------------------------------------\n"
        << this->scale << "\n"
        << "before ----------------------------------------------------\n"
        << this->prob_before << "\n"
        << "night ----------------------------------------------------\n"
        << this->prob_night << "\n";
    for(auto& request: this->prob_request){
        std::cout << "request of room " << request.first << ":\n"
            << request.second << "\n";
    }
    for(auto& demand: this->prob_ind_demand){
        std::cout << "demand of ind " << demand.first << ":\n"
            << demand.second << "\n";
    }
    std::cout << "Discount ----------------------------------------------------\n"
        << this->prob_discount << "\n"
        << "price ----------------------------------------------------\n"
        << this->price_ind << "\n"
        << "capacity ----------------------------------------------------\n"
        << this->capacity << "\n";
    
    std::cout << "upgrade to upper ----------------------------------------------------\n"
        << this->upgrade_upper << "\n"
        << "upgrade from lower ----------------------------------------------------\n"
        << this->upgrade_lower << "\n"
        << "upgrade pairs ----------------------------------------------------\n"
        << this->upgrade_pairs << "\n";
}

}

std::ostream& operator<<(std::ostream& os, const data::CaseScale& scale){
    os << "room_type: " << scale.room_type << "\n"
       << "before: " << scale.before << "\n"
       << "periods_per_day: " << scale.periods_per_day << "\n"
       << "booking_day: " << scale.booking_day << "\n"
       << "booking_period: " << scale.booking_period << "\n"
       << "service_period: " << scale.service_period << "\n"; 
    return os;
}