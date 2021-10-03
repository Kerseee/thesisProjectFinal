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

void CaseData::readMetaData(const std::string& path){
    std::ifstream input(path);
    std::string item;
    // Check if file is open
    fileOpenCheck(input, path);
    input >> item >> this->scale.before
          >> item >> this->scale.room_type
          >> item >> this->scale.booking_day
          >> item >> this->scale.booking_period
          >> item >> this->scale.service_period;
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

    // Read probs of price for individual customers
    readCsv(folder + "p_si.csv", this->price_ind);
}
void CaseData::printAll(){
    std::cout << "Input folder: " << this->folder << "\n"
        << "Scale ----------------------------------------------------\n"
        << this->scale << "\n"
        << "before ----------------------------------------------------\n"
        << this->prob_before << "\n"
        << "night ----------------------------------------------------\n"
        << this->prob_night << "\n";
    for(int i = 1; i <= this->scale.room_type; i++){
        std::cout << "request of room " << i << ":\n"
            << this->prob_request[i] << "\n";
    }
    for(int i = 1; i <= this->scale.room_type; i++){
        std::cout << "demand of ind " << i << ":\n"
            << this->prob_ind_demand[i] << "\n";
    }
    std::cout << "Discount ----------------------------------------------------\n"
        << this->prob_discount << "\n"
        << "price ----------------------------------------------------\n"
        << this->price_ind << "\n";
}

}