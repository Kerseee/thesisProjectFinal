//
//  main.cpp
//  thesisDP
//
//  Created by 黃楷翔 on 2021/3/7.
//  Copyright © 2021年 黃楷翔. All rights reserved.
//

#include <iostream>
#include <string>
// #include <map>
// #include <tuple>
#include <time.h>
// #include <chrono>
// #include <random>
// #include "src/data.hpp"
// #include "src/planner.hpp"
#include "src/caseController.hpp"
// #include "src/experimentor.hpp"
// #include "src/caseData.hpp"
// #include "src/caseGenerator.hpp"
// #include "src/casePlanner.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

// const string PARAM_PREFIX = "input/params";
// const string PROB_PREFIX = "input/prob";
// const string DIST_PREFIX = "input/distParams";
// const string OUTPUT_PREFIX = "result";
// const string DECISION_PREFIX = "result/decision";
// const string REVENUE_PREFIX = "result/revenue";
// const string WINDOWS_PREFIX = "C:/Users/racs1/Desktop/thesisProject/";

void debug();
void numericalExperiment();

int main(int argc, const char * argv[]) {
    
    // Create controller
    planner::MyopicExpersController controller;
    
    // Input the path of input folder
    std::string input_folder;
    cout << "Input the path of INPUT folder, and make sure "
        << "there is \\ on windows or / on mac after the folder path:\n";
    cin >> input_folder;

    // Input the path of output folder
    std::string output_folder;
    cout << "\nInput the path of OUTPUT folder, and make sure "
        << "there is \\ on windows or / on mac after the folder path:\n";
    cin >> output_folder;
    planner::createRelativeFolder(output_folder);

    // Set parameters:
    int num_exper_gen = 0, num_exper_plan = 0, sample_size = 0;
    
    cout << "\nInput the number of experiments for generating random events: ";
    cin >> num_exper_gen;
    controller.setNumExperGen(num_exper_gen);
    
    cout << "\nInput the number of sample size for stochastic planners: ";
    cin >> sample_size;
    controller.setSampleSize(sample_size);

    cout << "\nInput the number of experiments for running planners: ";
    cin >> num_exper_plan;
    controller.setNumExperPlan(num_exper_plan);

    double from = 0, to = 1, step_size = 0.1;
    cout << "\nInput 3 double for generating alphas for adjusted planners \n"
        << "format: from to step_size\n" 
        << "e.g. 0 1 0.1 meaning that alphas = [0, 0.1, 0.2, ..., 0.9, 1]\n "
        << "(all double shoud be inside of [0, 1]):\n";
    cin >> from >> to >> step_size;
    controller.setAlphas(from, to, step_size);
    cout << "\nSetting success! Now start running...\n";

    controller.runAll(input_folder);
    controller.storeAllResults(output_folder);
    // nlohmann::json j;
    // j["pi"] = 3.41;
    // cout << j;

    return 0;
}

void debug(){
    // Input folders
    string folder = "";
    cout << "Input folder name \n (Please add \"\\\" on windows or \"/\" on mac after the folder):\n";
    cin >> folder;
    
    // Build CaseData
    cout << "\nReading data...\n";
    data::CaseData data;
    data.readAllData(folder);
    cout << data.scale;

    // Set State
    planner::DeterExperimentor myopicND(data);
    myopicND.run();

    cout << "\nTest End!\n";
}

void numericalExperiment(){
    string scenarios[] = {"S5I3C25m5b70r2d25K10T10", "S5I3C25m10b70r2d25K10T10", "S5I3C25m15b70r2d25K10T10", "S5I3C25m20b70r2d25K10T10",
                          "S5I3C25m10b50r2d25K10T10", "S5I3C25m10b90r2d25K10T10",
                          "S5I3C25m10b70r1d25K10T10", "S5I3C25m10b70r3d25K10T10",
                          "S5I3C25m10b70r2d10K10T10", "S5I3C25m10b70r2d40K10T10"};
    for(auto& s: scenarios){
        string param_path = "input/params/" + s + "_poisson/1.txt";
        string prob_path = "input/prob/" + s + "_poisson/T10_1.txt";
        data::Params params(param_path, prob_path);

        planner::MyopicPlanner myopic(params, planner::MyopicMode::bs, planner::MyopicModelType::ip);
        time_t start, end;
        time(&start);
        myopic.testBSBeta(s);
        time(&end);
        std::cout << "Time: " << difftime(end, start) << "\n";
    }
}