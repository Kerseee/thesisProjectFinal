#ifndef experimentor_h
#define experimentor_h

#include <stdio.h>
#include <map>
#include <string>
#include <set>
#include <fstream>
#include <vector>
#include "data.hpp"

namespace experimentor {

// MDUpgrade find the best upgrade plan.
// Example:
//    MDupgrade md;
//    md.ReadData(data_path);
//    double rev = md.Run();
//    md.storeResult(output_path);
class MDUpgrade {
protected:
    // Sets
    int num_day_;
    int num_room_;
    int period_;
    std::map<int, std::set<int> > upgrade_out_;
    std::map<int, std::set<int> > upgrade_in_;
    // Parameters
    std::map<data::tuple2d, double> upgrade_fees_;
    std::map<data::tuple2d, double> prices_;
    std::map<data::tuple2d, int> capacity_;
    std::map<data::tuple2d, int> demands_;
    std::map<int, int> orders_;
    // Decision variables
    std::map<data::tuple2d, int> upgrade_;
    std::map<data::tuple2d, int> sold_;
    double revenue_;

    // Reset the data
    void Reset();
public:
    MDUpgrade();
    void ReadData(const std::string& path);
    double Run();
    void StoreResult(const std::string& path_upgrade, 
                     const std::string& path_sold);
    void PrintData();
    void PrintResult();
};

class MSUpgrade: public MDUpgrade{
protected:
    // Sets
    int num_scenarios_;

    // Parameters
    std::map<data::tuple2d, std::map<int, double> > prob_;
    std::map<data::tuple2d, std::map<int, double> > cdf_;
    // Scenario_demand
    std::vector<std::map<data::tuple2d, int> > s_demands_;
    int num_exper_;
    // Guess demand
    void GuessDemand();
public:
    void ReadData(const std::string& path);
    double Run();
    // void StoreResult(const std::string& path_upgrade, 
                    //  const std::string& path_sold);
    void PrintData();
    void StoreResult(const std::string& path_upgrade);
    void test();
};

class MSUpgradeSmart: public MSUpgrade{
private:
    // Scenario_prices
    std::vector<std::map<data::tuple2d, double> > s_prices_;
public:
    void ReadData(const std::string& path);
    void PrintData();
    double Run();
};



void PerformExpers();
void PerformExpersMS();
void PerformExpersMSSmart();
void Debug(int day, int room, int cap, int p, int t, int file);
void DebugMS(int day, int room, int cap, int p, int t, int file);

}

#endif /* experimentor_h */
