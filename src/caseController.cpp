#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <fstream>
#include <vector>
#include "data.hpp"
#include "planner.hpp"
#include "caseData.hpp"
#include "caseStructures.hpp"
#include "caseGenerator.hpp"
#include "casePlanner.hpp"
#include "caseController.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace planner {

void createRelativeFolder(std::string folder){
#ifdef __APPLE__
    system(("mkdir " + folder).c_str());
#elif _WIN32
    CreateDirectory((folder).c_str(), NULL);
#endif
}

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

// runAll go through processes including reading data, generating events,
// and running planners. If you want to store results after calling runAll,
// please call storeAllResults(folder) and input the result-folder path.
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

// storeResult store the result of given planner type
void MyopicExpersController::storeResult(
    const std::string& planner_type, const std::string& path
){
    // Check if result of planner exist
    auto it = this->results.find(planner_type);
    if(it == this->results.end()){
        std::cout << "Type " << planner_type 
            << " does not exist in results!\n";
        return;
    }
    // Write header of result file
    std::ofstream out;
    out.open(path);
    std::string head = "Experiment,revenue,time,sold_out";
    for(int t = this->data.scale.booking_period; t >= 1; t--){
        head += ",acceptable_t" + std::to_string(t)
                + ",decision_t" + std::to_string(t)
                + ",rev_acc_t" + std::to_string(t)
                + ",rev_rej_t" + std::to_string(t)
                + ",rev_order_t" + std::to_string(t)
                + ",rev_ind_t" + std::to_string(t);
    }
    head += "\n";
    out << head;

    // Write the result into result file
    for(auto& exper: it->second){
        int exper_id = exper.first;
        auto& result = exper.second;
        std::string msg = std::to_string(exper_id) + "," 
                        + std::to_string(result.revenue) + ","
                        + std::to_string(result.runtime) + ","
                        + std::to_string(result.stop_period);

        // Write results in all booking periods
        for(int t = this->data.scale.booking_period; t >= 1; t--){
            bool acceptable = false, accepted = false;
            double rev_accept = 0, rev_reject = 0;
            double rev_order = 0, rev_ind_demand = 0;
            
            // Fetch results about decision
            auto it_decision = result.decisions.find(t);
            if(it_decision != result.decisions.end()){
                auto& decision = it_decision->second;
                acceptable = decision.acceptable;
                accepted = decision.accepted;
                rev_accept = decision.exp_acc_togo;
                rev_reject = decision.exp_rej_togo;
            }
            
            // Fetch results about revenues of order and demand
            auto it_order_rev = result.order_revenues.find(t);
            if(it_order_rev != result.order_revenues.end()){
                rev_order = it_order_rev->second;
            }
            auto it_order_demand = result.ind_demand_revenues.find(t);
            if(it_order_demand != result.ind_demand_revenues.end()){
                rev_ind_demand = it_order_demand->second;
            }
            
            // Write to file
            msg += "," + std::to_string(acceptable)
                    + "," + std::to_string(accepted)
                    + "," + std::to_string(rev_accept)
                    + "," + std::to_string(rev_reject)
                    + "," + std::to_string(rev_order)
                    + "," + std::to_string(rev_ind_demand);
        }
        msg += "\n";
        out << msg;
    }
    out.close();
}

// storeResults store all results into folder
// Please make sure there is "/" for mac or "\" for windows after folder
void MyopicExpersController::storeAllResults(const std::string& folder){
    createRelativeFolder(folder);
    this->storeResult("ND", folder + "ND_result.csv");
    this->storeResult("NS", folder + "NS_result.csv");
    this->storeResult("AD", folder + "AD_result.csv");
    this->storeResult("AS", folder + "AS_result.csv");
}

}
