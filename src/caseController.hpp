#ifndef caseController_hpp
#define caseController_hpp

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
#include "../third_party/json-develop/single_include/nlohmann/json.hpp"

namespace planner {

void createRelativeFolder(std::string folder);

class MyopicExpersController{
private:
    /* Variables */
    bool has_data;
    data::CaseData data;
    int num_experiments;
    int sample_size; // sample size for stochastic planners
    std::vector<double> alphas; // parameter alpha for adjusted planners
    std::string input_folder;
    std::string output_folder;

    // orders store the orders generated by OrderGenerator
    std::map<int, Order> orders;

    // demands store the orders generated by IndDemandGenerator
    std::map<int, std::map<data::tuple2d, int> > demands;

    // exp_demands store the orders generated by ExpectedDemandGenerator
    std::map<int, std::map<data::tuple2d, int> > exp_demands;

    // est_demands store the orders generated by EstimatedDemandGenerator
    std::map<int, std::vector<std::map<data::tuple2d, int> > > est_demands;
    
    // results store the results generated from planners, 
    // results[planner_type][experiment_id] = result
    std::map<std::string, std::map<int, ExperimentorResult> > results;

    // result_file_names store names of result files for each planners
    std::map<std::string, std::string> result_file_names;

    /* Private methods */
    
    // runPlannerND create a DeterExperimentor, run one experiment, 
    // and then return result.
    ExperimentorResult runPlannerND();
    
    // runPlannerNS create a StochExperimentor, run one experiment, 
    // and then return result.
    ExperimentorResult runPlannerNS();
    
    // runPlannerAD create a ADExperimentor, run one experiment, 
    // and then return result.
    ExperimentorResult runPlannerAD(const double alpha);
    
    // runPlannerAS create a ASExperimentor, run one experiment, 
    // and then return result.
    ExperimentorResult runPlannerAS(const double alpha);

    // Check the valid of parameter for this controller
    bool hasValidSampleSize();
    bool hasValidAlphas();
    bool hasValidNumExperiments();

    // runPlanner run all planners and store results
    void runPlanners(const int exper_id);

    // prepareResultsFiles write the headers of each planner into result files
    void prepareResultsFiles();

    void setResultFileNames();

    // appendResultToFile append a line of result to the result file of planner
    void appendResultToFile(const int exper_id, 
        const std::string& planner_name, 
        const ExperimentorResult& result);

public:
    MyopicExpersController();

    // setNumExperGen set the number of experiments for generating evenets
    void setNumExperiments(const int num);

    // setSampleSive set the sample size for stochastic planners
    void setSampleSize(const int num);

    // setAlphas set the alpha for running adjusted planners
    void setAlphas(const std::vector<double>& alphas);

    // setAlphas set the alpha for running adjusted planners
    void setAlphas(const double from, const double to, const double step_size);

    // setInputFolder set the input folder of this controller
    void setInputFolder(std::string folder);

    // setOutputFolder set the output folder of this controller
    void setOutputFolder(std::string folder);

    // checkHasData check if readDataFolder has been called and cout alert
    // message
    bool checkHasData();

    // readDataFolder read all the data in given folder and store data
    // into this controller
    void readDataFolder(const std::string& folder);

    // generateOrders recreate the orders and replace current this->orders
    void generateOrders();

    // generateDemands recreate the demands and replace current this->demands
    void generateDemands();

    // generateExpDemands generate expected demands for deterministic planners
    void generateExpDemands();

    // generateEstDemands generate expected demands for stochastic planners
    void generateEstDemands();

    // storeOrdersToJson store the orders in this controller into json file
    void storeOrdersToJson(const std::string& path);

    // runAll go through processes including reading data, generating events,
    // running planners, and store results.
    void runAll();
 

    void debug();
};

// strToTuple2d transfer "1_2" to {1, 2}
data::tuple2d strToTuple2d(const std::string& str);

nlohmann::json orderToJson(const Order& order);

}

#endif /*caseController_hpp*/