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

// setNumExperPlan set the number of experiments for running planners
void MyopicExpersController::setNumExperPlan(const int num){
    this->num_exper_plan = num;
}

// setNumExperGen set the number of experiments for generating evenets
void MyopicExpersController::setNumExperGen(const int num){
    this->num_exper_gen = num;
}

// setSampleSive set the sample size for stochastic planners
void MyopicExpersController::setSampleSize(const int num){
    this->sample_size = num;
}

// setAlphas set the alpha for running adjusted planners
void MyopicExpersController::setAlphas(const std::vector<double>& alphas){
    this->alphas = alphas;
}

bool MyopicExpersController::hasValidSampleSize(){
    if(this->sample_size <= 0){
        std::cout << "Sample size for stochastic planners are not set! " 
            << "Please call setSampleSize() first!\n";
        return false;
    }
    return true;
}

bool MyopicExpersController::hasValidAlphas(){
    if(this->alphas.empty()){
        std::cout << "There is no alpha for adjusted planners! " 
            << "Please call setAlphas() first!\n";
        return false;
    }
    return true;
}
bool MyopicExpersController::hasValidNumExperPlan(){
    if(this->num_exper_plan <= 0){
        std::cout << "Number of experiments for running planners are not set." 
            << "Please call setNumExperPlan() first!\n";
        return false;
    }
    return true;
}
bool MyopicExpersController::hasValidNumExperGen(){
    if(this->num_exper_gen <= 0){
        std::cout << "Number of experiments for gerenrating events are not set." 
            << "Please call setNumExperGen() first!\n";
        return false;
    }
    return true;
}

// setAlphas set the alpha for running adjusted planners
void MyopicExpersController::setAlphas(
    const double from, const double to, const double step_size
){
    if(from > to || step_size == 0 ||
       from > 1 || from < 0 ||
       to > 1 || to < 0)
    {
        std::cout << "Error: setAlphas error in controller.\n";
        return;
    }
    this->alphas.clear();
    if(from == to){
        this->alphas.push_back(from);
        return;
    }
    
    double current_alpha = from;
    while(current_alpha < to){
        this->alphas.push_back(current_alpha);
        current_alpha += step_size;
    }
    this->alphas.push_back(to);
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

// runPlannerNS create a StochExperimentor, run one experiment, 
// and then return result.
ExperimentorResult MyopicExpersController::runPlannerNS(
    const int exper_id
){
    StochExperimentor planner(this->data);
    planner.addOrders(this->orders.at(exper_id));
    planner.addIndDemands(this->demands.at(exper_id));
    planner.addEstIndDemands(this->est_demands);
    return planner.run();
}

// runPlannerAD create a ADExperimentor, run one experiment, 
// and then return result.
ExperimentorResult MyopicExpersController::runPlannerAD(
    const int exper_id, const double alpha
){
    ADExperimentor planner(this->data, alpha);
    planner.addOrders(this->orders.at(exper_id));
    planner.addIndDemands(this->demands.at(exper_id));
    planner.addEstIndDemands(this->exp_demands);
    return planner.run();
}

// runPlannerAS create a ASExperimentor, run one experiment, 
// and then return result.
ExperimentorResult MyopicExpersController::runPlannerAS(
    const int exper_id, const double alpha
){
    ASExperimentor planner(this->data, alpha);
    planner.addOrders(this->orders.at(exper_id));
    planner.addIndDemands(this->demands.at(exper_id));
    planner.addEstIndDemands(this->est_demands);
    return planner.run();
}

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
void MyopicExpersController::generateEvents(){

    if(!this->hasValidNumExperGen() || !this->hasValidSampleSize()){
        return;
    }

    // Create generators
    std::cout << "Create generators ...\n";
    OrderGenerator gen_order(this->data);
    IndDemandGenerator gen_demand(this->data);
    ExpectedDemandGenerator gen_exp_demand(this->data);
    EstimatedDemandGenerator gen_est_demand(this->data);

    // Generate orders and demands
    std::cout << "Generate need data ...\n";
    this->orders = gen_order.generate(this->num_exper_gen);
    this->demands = gen_demand.generate(this->num_exper_gen);
    this->exp_demands = gen_exp_demand.generate();
    this->est_demands = gen_est_demand.generate(sample_size);
}


// runPlanner run the planner num_exper times
void MyopicExpersController::runPlanner(const std::string& planner_type){
    if(!this->hasValidNumExperPlan()) return;
    
    
    if(planner_type == "ND"){
        std::map<int, ExperimentorResult> planner_result;
        for(int e = 1; e <= this->num_exper_plan; e++){
            planner_result[e] = this->runPlannerND(e);
        }
        this->results[planner_type] = planner_result;
    } 
    else if(planner_type == "NS"){
        std::map<int, ExperimentorResult> planner_result;
        for(int e = 1; e <= this->num_exper_plan; e++){
            planner_result[e] = this->runPlannerNS(e);
        }
        this->results[planner_type] = planner_result;
    }
    else if(planner_type == "AD"){
        for(const auto& alpha: this->alphas){
            std::map<int, ExperimentorResult> planner_result;
            for(int e = 1; e <= this->num_exper_plan; e++){
                planner_result[e] = this->runPlannerAD(e, alpha);
            }
            std::string name = planner_type + "_" + std::to_string(alpha);
            this->results[name] = planner_result;
        }
        
    }
    else if(planner_type == "AS"){
        for(const auto& alpha: this->alphas){
            std::map<int, ExperimentorResult> planner_result;
            for(int e = 1; e <= this->num_exper_plan; e++){
                planner_result[e] = this->runPlannerAS(e, alpha);
            }
            std::string name = planner_type + "_" + std::to_string(alpha);
            this->results[name] = planner_result;
        }
    } 
    else{
        std::cout << "Planner " << planner_type << "no exist in controller!\n";
        return;
    }
    
}

// runAll go through processes including reading data, generating events,
// and running planners. If you want to store results after calling runAll,
// please call storeAllResults(folder) and input the result-folder path.
void MyopicExpersController::runAll(const std::string& data_folder){ 
    
    // Read all data
    std::cout << "Reading data in " << data_folder << " ...\n";
    this->readDataFolder(data_folder);

    // Generate events
    std::cout << "GenerateEvents ...\n";
    this->generateEvents();
    
    // Create planners
    std::cout << "Run Planners ...\n";
    this->runPlanner("ND");
    this->runPlanner("NS");
    this->runPlanner("AD");
    this->runPlanner("AS");
    std::cout << "Finish planning! \n";
}

void MyopicExpersController::debug(){

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
    std::cout << "Store all...\n";
    for(const auto& result: this->results){
        std::string name = result.first;
        std::string path = folder + name + "_result.csv";
        this->storeResult(name, path);
    }
    std::cout << "Finish storing!\n";
}

}
