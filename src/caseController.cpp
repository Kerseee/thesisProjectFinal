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

// setNumExperGen set the number of experiments for generating evenets
void MyopicExpersController::setNumExperiments(const int num){
    this->num_experiments = num;
}

// setSampleSive set the sample size for stochastic planners
void MyopicExpersController::setSampleSize(const int num){
    this->sample_size = num;
}

// setAlphas set the alpha for running adjusted planners
void MyopicExpersController::setAlphas(const std::vector<double>& alphas){
    this->alphas = alphas;
}

// setInputFolder set the input folder of this controller
void MyopicExpersController::setInputFolder(std::string folder){
    this->input_folder = folder;
}

// setOutputFolder set the output folder of this controller
void MyopicExpersController::setOutputFolder(std::string folder){
    this->output_folder = folder;
    planner::createRelativeFolder(folder);
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
bool MyopicExpersController::hasValidNumExperiments(){
    if(this->num_experiments <= 0){
        std::cout << "Number of experiments are not set." 
            << "Please call setNumExperiments() first!\n";
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
ExperimentorResult MyopicExpersController::runPlannerND(){
    DeterExperimentor planner(this->data);
    planner.addOrders(this->orders);
    planner.addIndDemands(this->demands);
    planner.addEstIndDemands(this->exp_demands);
    return planner.run();
}

// runPlannerNS create a StochExperimentor, run one experiment, 
// and then return result.
ExperimentorResult MyopicExpersController::runPlannerNS(){
    StochExperimentor planner(this->data);
    planner.addOrders(this->orders);
    planner.addIndDemands(this->demands);
    planner.addEstIndDemands(this->est_demands);
    return planner.run();
}

// runPlannerAD create a ADExperimentor, run one experiment, 
// and then return result.
ExperimentorResult MyopicExpersController::runPlannerAD(const double alpha){
    ADExperimentor planner(this->data, alpha);
    planner.addOrders(this->orders);
    planner.addIndDemands(this->demands);
    planner.addEstIndDemands(this->exp_demands);
    return planner.run();
}

// runPlannerAS create a ASExperimentor, run one experiment, 
// and then return result.
ExperimentorResult MyopicExpersController::runPlannerAS(const double alpha){
    ASExperimentor planner(this->data, alpha);
    planner.addOrders(this->orders);
    planner.addIndDemands(this->demands);
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

// generateOrders recreate the orders and replace current this->orders
void MyopicExpersController::generateOrders(){
    if(!this->checkHasData()) return;
    this->orders.clear();
    OrderGenerator gen(this->data);
    this->orders = gen.generate();
}

// generateDemands recreate the demands and replace current this->demands
void MyopicExpersController::generateDemands(){
    if(!this->checkHasData()) return;
    this->demands.clear();
    IndDemandGenerator gen(this->data);
    this->demands = gen.generate();
}

// generateExpDemands generate expected demands for deterministic planners
void MyopicExpersController::generateExpDemands(){
    if(!this->checkHasData()) return;
    this->exp_demands.clear();
    ExpectedDemandGenerator gen(this->data);
    this->exp_demands = gen.generate();
}

// generateEstDemands generate expected demands for stochastic planners
void MyopicExpersController::generateEstDemands(){
    if(!this->checkHasData() || !this->hasValidSampleSize()) return;
    this->est_demands.clear();
    EstimatedDemandGenerator gen(this->data);
    this->est_demands = gen.generate(this->sample_size);
}

// runPlanner run all planners and store results
void MyopicExpersController::runPlanners(const int exper_id){
    if(!this->hasValidAlphas()) return;

    std::cout << "Run experiment " << exper_id << ": " << std::flush;

    // Generate orders and demand for this experiment
    this->generateOrders();
    this->generateDemands();

    // Run ND
    std::cout << "ND " << std::flush;
    ExperimentorResult result_ND = this->runPlannerND();
    this->appendResultToFile(exper_id, "ND", result_ND);

    // Run NS
    std::cout << "NS " << std::flush;
    ExperimentorResult result_NS = this->runPlannerNS();
    this->appendResultToFile(exper_id, "NS", result_NS);

    // Run AD
    std::cout << "AD " << std::flush;
    for(const auto& alpha: this->alphas){
        ExperimentorResult result_AD = this->runPlannerAD(alpha);
        std::string name_AD = "AD_" + std::to_string(alpha);
        this->appendResultToFile(exper_id, name_AD, result_AD);
    }

    // Run AS
    std::cout << "AS " << std::flush;
    for(const auto& alpha: this->alphas){
        ExperimentorResult result_AS = this->runPlannerAS(alpha);
        std::string name_AS = "AS_" + std::to_string(alpha);
        this->appendResultToFile(exper_id, name_AS, result_AS);
    }

    std::cout << "done!" << std::endl;

}

void MyopicExpersController::setResultFileNames(){
    std::string prefix = this->output_folder;
    std::string suffix = "_result.csv";

    auto getFileNames = [&](std::string name) -> std::string {
        return prefix + name + suffix;
    };

    this->result_file_names["ND"] = getFileNames("ND");
    this->result_file_names["NS"] = getFileNames("NS");
    for(const auto& alpha: this->alphas){
        std::string name_AD = "AD_" + std::to_string(alpha);
        std::string name_AS = "AS_" + std::to_string(alpha); 
        this->result_file_names[name_AD] = getFileNames(name_AD);
        this->result_file_names[name_AS] = getFileNames(name_AS);
    }
}

// prepareResultsFiles write the headers of each planner into result files
void MyopicExpersController::prepareResultsFiles(){
    
    // Set the result file names
    this->setResultFileNames();
    
    // Write header
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

    // Create result files
    for(const auto& file: this->result_file_names){
        std::ofstream out;
        out.open(file.second);
        out << head;
        out.close();
    }
}

// appendResultToFile append a line of result to the result file of planner
void MyopicExpersController::appendResultToFile(
    const int exper_id, 
    const std::string& planner_name, 
    const ExperimentorResult& result
){
    // Return if no file name exist
    auto it_file = this->result_file_names.find(planner_name);
    if(it_file == this->result_file_names.end()){
        std::cout << "Cannot find the file name of planner " << planner_name
            << " !\n";
        return;
    }
    
    // Store result
    std::ofstream out(it_file->second, std::ofstream::app);

    // Write the result into result file
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

    out.close();
}

// runAll go through processes including reading data, generating events,
// running planners, and store results.
void MyopicExpersController::runAll(){ 
    
    // Read all data
    std::cout << "Reading data in " << this->input_folder << " ...\n";
    this->readDataFolder(this->input_folder);

    // Generate estimated and expected demands for planners
    std::cout << "\nGenerating expected demands ...\n";
    this->generateExpDemands();

    std::cout << "\nGenerating estimated demands ...\n";
    this->generateEstDemands();

    // Prepare the result files
    std::cout << "\nPreparing result files ...\n";
    this->prepareResultsFiles();

    // Run planners
    std::cout << "\nStart planning! \n";
    for(int e = 1; e <= this->num_experiments; e++){
        this->runPlanners(e);
    }
}
}
