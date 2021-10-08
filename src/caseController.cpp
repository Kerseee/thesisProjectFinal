#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"
#include "caseStructures.hpp"
#include "caseGenerator.hpp"
#include "casePlanner.hpp"
#include "caseController.hpp"

namespace planner {

// ======================================================================
// ----------------------- MyopicExpersController -----------------------
// ======================================================================

MyopicExpersController::MyopicExpersController(){
    this->has_data = false;
}

// runPlannerND create a DeterExperimentor, run one experiment, 
// and then return result.
ExperimentorResult MyopicExpersController::runPlannerND(
    const int exper_id
){
    DeterExperimentor planner(this->data);
    planner.addOrders(this->orders.at(exper_id));
    planner.addIndDemands(this->demands.at(exper_id));
    planner.addEstIndDemands(this->exp_demands);
    return planner.run();
}

// // runPlannerNS create a StochExperimentor, run one experiment, 
// // and then return result.
// ExperimentorResult MyopicExpersController::runPlannerNS(
//     const int exper_id
// ){
//     StochExperimentor planner(this->data);
//     planner.addOrders(this->orders.at(exper_id));
//     planner.addIndDemands(this->demands.at(exper_id));
//     planner.addEstIndDemands(this->est_demands);
//     return planner.run();
// }

// // runPlannerAD create a ADExperimentor, run one experiment, 
// // and then return result.
// ExperimentorResult MyopicExpersController::runPlannerAD(
//     const int exper_id
// ){
//     ADExperimentor planner(this->data);
//     planner.addOrders(this->orders.at(exper_id));
//     planner.addIndDemands(this->demands.at(exper_id));
//     planner.addEstIndDemands(this->exp_demands);
//     return planner.run();
// }

// // runPlannerAS create a ASExperimentor, run one experiment, 
// // and then return result.
// ExperimentorResult MyopicExpersController::runPlannerAS(
//     const int exper_id
// ){
//     ASExperimentor planner(this->data);
//     planner.addOrders(this->orders.at(exper_id));
//     planner.addIndDemands(this->demands.at(exper_id));
//     planner.addEstIndDemands(this->est_demands);
//     return planner.run();
// }

// checkHasData check if readDataFolder has been called and cout alert
// message
bool MyopicExpersController::checkHasData(){
    if(!this->has_data){
        std::cout << "Please call readDataFolder first!\n";
        return false;
    }
    return true;
}

// readDataFolder read all the data in given folder and store data
// into this controller
void MyopicExpersController::readDataFolder(const std::string& folder){
    this->data.readAllData(folder);
    this->has_data = true;
}

// generateEvents generates and stores all type of generators, 
// including orders, demands, exp_demands, est_demands
void MyopicExpersController::generateEvents(
    const int num_exper, const int sample_size
){
    // Create generators
    OrderGenerator gen_order(this->data);
    IndDemandGenerator gen_demand(this->data);
    ExpectedDemandGenerator gen_exp_demand(this->data);
    EstimatedDemandGenerator gen_est_demand(this->data);

    // Generate orders and demands
    this->orders = gen_order.generate(num_exper);
    this->demands = gen_demand.generate(num_exper);
    this->exp_demands = gen_exp_demand.generate();
    this->est_demands = gen_est_demand.generate(sample_size);
}


// runPlanner run the planner num_exper times
void MyopicExpersController::runPlanner(
    const std::string& planner_type, const int num_exper
){
    std::map<int, ExperimentorResult> planner_result;
    if(planner_type == "ND"){
        for(int e = 1; e <= num_exper; e++){
            planner_result[e] = this->runPlannerND(e);
        }
    } 
    // else if(planner_type == "NS"){
    //     for(int e = 1; e <= num_exper; e++){
    //         planner_result[e] = this->runPlannerNS(e);
    //     }
    // } else if(planner_type == "AD"){
    //     for(int e = 1; e <= num_exper; e++){
    //         planner_result[e] = this->runPlannerAD(e);
    //     }
    // } else if(planner_type == "AS"){
    //     for(int e = 1; e <= num_exper; e++){
    //         planner_result[e] = this->runPlannerAS(e);
    //     }
    // } 
    else{
        std::cout << "Planner " << planner_type << "no exist in controller!\n";
        return;
    }
    this->results[planner_type] = planner_result;
}

// runAll go through all the process
void MyopicExpersController::runAll(
    const std::string& data_folder, const int num_exper, const int sample_size
){
    // Read all data
    this->readDataFolder(data_folder);

    // Generate events
    this->generateEvents(num_exper, sample_size);
    
    // Create planners
    this->runPlanner("ND", num_exper);
    // this->runPlanner("NS", num_exper);
    // this->runPlanner("AD", num_exper);
    // this->runPlanner("AS", num_exper);
}

// storeResults store all results into folder
void MyopicExpersController::storeResults(const std::string& folder){
    
}

}
