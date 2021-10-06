//
//  main.cpp
//  thesisDP
//
//  Created by 黃楷翔 on 2021/3/7.
//  Copyright © 2021年 黃楷翔. All rights reserved.
//

#include <iostream>
#include <string>
#include <map>
#include <tuple>
#include <time.h>
#include <chrono>
#include <random>
#include "src/data.hpp"
#include "src/planner.hpp"
#include "src/experimentor.hpp"
#include "src/caseData.hpp"
#include "src/caseGenerator.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

const string PARAM_PREFIX = "input/params";
const string PROB_PREFIX = "input/prob";
const string DIST_PREFIX = "input/distParams";
const string OUTPUT_PREFIX = "result";
const string DECISION_PREFIX = "result/decision";
const string REVENUE_PREFIX = "result/revenue";
const string WINDOWS_PREFIX = "C:/Users/racs1/Desktop/thesisProject/";

void callDP(bool store=true);
void callMyopic(planner::MyopicModelType model, planner::MyopicMode mode, int num_exper=1000, bool store=true);
void createFolder(string path);
void testMyopic();
void testMyopicSmart();
void debug();
void testMyopicNC();
void testMyopicBS();

int main(int argc, const char * argv[]) {
    debug();
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
    planner::State::setScale(data.scale.service_period, data.scale.room_type);
    
    // Test functions
    cout << "\n Test planner::EstimatedDemandGenerator::generateEstDemand\n";
    planner::EstimatedDemandGenerator gen(data);
    cout << "Input period and seed: \n";
    int period = 0, seed = 0;
    cin >> period >> seed;
    cout << gen.generateEstDemand(period, seed);

    cout << "\nTest End!\n";
}

void testMyopicBS(){
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

void testMyopicNC(){
    string scenarios[] = {"S5I3C25m10b70r2d25K10T10", "S5I3C25m10b50r2d25K10T10", "S5I3C25m10b90r2d25K10T10",
                           "S5I3C25m5b70r2d25K10T10", "S5I3C25m20b70r2d25K10T10"};
    for(auto& s: scenarios){
        string param_path = "debug/data/params/" + s + "/1.txt";
        string prob_path = "debug/data/prob/" + s + "/T10_1.txt";
        data::Params params(param_path, prob_path);

        planner::MyopicPlanner myopic(params, planner::MyopicMode::naive, planner::MyopicModelType::ip);
        time_t start, end;
        time(&start);
        myopic.testNC(s);
        time(&end);
        std::cout << "Time: " << difftime(end, start) << "\n";
    }
}

void testMyopicSmart(){
    string scenarios[] = {"S5I3C25m10b70r2d25K10T10", "S5I3C25m10b50r2d25K10T10", "S5I3C25m10b90r2d25K10T10",
                           "S5I3C25m5b70r2d25K10T10", "S5I3C25m20b70r2d25K10T10"};
    for(auto& s: scenarios){
        string param_path = "debug/data/params/" + s + "/1.txt";
        string prob_path = "debug/data/prob/" + s + "/T10_1.txt";
        data::Params params(param_path, prob_path);

        planner::MyopicPlanner myopic(params, planner::MyopicMode::smart, planner::MyopicModelType::ip);
        time_t start, end;
        time(&start);
        myopic.testSmartGamma(s);
        time(&end);
        std::cout << "Time: " << difftime(end, start) << "\n";
    }

}

void testMyopic(){
    // Read file
    string scenarios[] = {"S5I3C25m10b70r2d25K10T10", "S5I3C25m10b50r2d25K10T10", "S5I3C25m10b90r2d25K10T10",
                           "S5I3C25m5b70r2d25K10T10", "S5I3C25m20b70r2d25K10T10"};
    
    for(auto& s: scenarios){
        string param_path = "debug/data/params/" + s + "/1.txt";
        string prob_path = "debug/data/prob/" + s + "/T10_1.txt";
        data::Params params(param_path, prob_path);

        planner::MyopicPlanner myopic(params, planner::MyopicMode::naive, planner::MyopicModelType::ip);
        time_t start, end;
        time(&start);
        myopic.test(s);
        time(&end);
        std::cout << "Time: " << difftime(end, start) << "\n";
    }
    



// #ifdef __APPLE__
//     std::cout << "OS: Mac os \n";
// #elif _WIN32
//     std::cout << "OS: windows \n";
// #endif

    // time_t start;
    // time(&start);
    // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    // std::uniform_real_distribution<double> distribution (0.0, 1.0);
    // std::default_random_engine generator1 (seed);
    // for(int i = 1; i <= 100; i++){
    //     seed = std::chrono::system_clock::now().time_since_epoch().count();
    //     std::default_random_engine generator2 (seed);
    //     double prob1 = distribution(generator1);
    //     double prob2 = distribution(generator2);
    //     cout << i << ", " << prob1 << ", " << prob2  << "\n";
    // }
    
    // std::default_random_engine generator3 (seed);
    // for(int i = 1; i <= 100; i++){
    //     // unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    //     std::default_random_engine generator2 (seed);
    //     double prob1 = distribution(generator3);
    //     double prob2 = distribution(generator2);
    //     cout << i << ", " << prob1 << ", " << prob2  << "\n";
    // }
    // time_t end;
    // time(&end);
    // cout << "time: " << difftime(end, start);
    
}


void createFolder(string folder){
#ifdef __APPLE__
    system(("mkdir -p "+ folder).c_str());
#elif _WIN32
    CreateDirectory((WINDOWS_PREFIX + folder).c_str(), NULL);
#endif
}

void callDP(bool store){
    // Scenarios
    std::string param_prefix = "input/params/";
    std::string prob_prefix = "input/prob/";
    std::string out_prefix = "result/DP/";
    createFolder(out_prefix);
    std::string out_rev_file = out_prefix + "speed.csv";
    int file_num = 1;
    // const int K = 5, T = 5;
    // const int days [] = {2};
    // const int rooms [] = {2};
    // const int caps [] = {2};
    // const int m_NB [] = {5, 10, 15, 20};
    // const int bargs [] = {50, 70, 90};
    // const int no_ords [] = {1, 2, 3};
    // const int reqs [] = {25, 50, 75};
    const int K = 5, T = 5;
    const int days [] = {2, 3, 4, 5};
    // const int rooms [] = {2};
    // const int caps [] = {2};
    const int m_NB [] = {5};
    const int bargs [] = {50};
    const int no_ords [] = {1};
    const int reqs [] = {25};

    std::ofstream out(out_rev_file, std::ofstream::out|std::ofstream::app);
    std::string head = "Num_day,Num_room_type,Min_capacity,param_NB,param_barg,param_no_order,param_req,Num_period,Num_order,file,exp_rev,time\n";
    out << head;
    out.close();

    for(int s: days){
        set<int> rooms;
        if(s == 2){
            rooms.insert({2, 3, 4, 5});
        } else {
            rooms.insert(2);
        }
        for(int i: rooms){
            set<int> caps;
            if(i == 2 && s == 2){
                caps.insert({2, 3, 4, 5});
            } else {
                caps.insert(2);
            }
            for(int c: caps){
                for(int m: m_NB){
                    for(int b: bargs){
                        for(int r: no_ords){
                            for(int d: reqs){
                                string folder = "S" + to_string(s) +
                                                "I" + to_string(i) +
                                                "C" + to_string(c) +
                                                "m" + to_string(m) +
                                                "b" + to_string(b) +
                                                "r" + to_string(r) + 
                                                "d" + to_string(d) + "/";
                                string param_folder = param_prefix + folder;
                                string prob_folder = prob_prefix + folder;
                                for(int f = 1; f <= file_num; f++){
                                    cout << "-------- File " + folder + to_string(f) + " start! --------\n";
                                    
                                    // File name
                                    string file = to_string(f) + ".txt";
                                    string prob = "T" + to_string(T) + "_";
                                    string param_path = param_folder + file;
                                    string prob_path = prob_folder + prob + file;
                                    
                                    // Read file
                                    data::Params params(param_path, prob_path);
                                    
                                    // Initial
                                    planner::DPPlanner DP(params);
                                    
                                    // Run
                                    time_t start_time;
                                    time(&start_time);
                                    double revenue = DP.run();
                                    time_t now_time;
                                    time(&now_time);
                                    double compute_time = difftime(now_time, start_time);

                                    std::ofstream out(out_rev_file, std::ofstream::out|std::ofstream::app);
                                    // Store time
                                    string msg = to_string(s) + ","
                                                + to_string(i) + ","
                                                + to_string(c) + ","
                                                + to_string(m) + ","
                                                + to_string(b) + ","
                                                + to_string(r) + ","
                                                + to_string(d) + ","
                                                + to_string(T) + ","
                                                + to_string(K) + ","
                                                + to_string(f) + ","
                                                + to_string(revenue) + ","
                                                + to_string(compute_time) + "\n";
                                    out << msg;
                                    out.close();

                                    if(store){
                                        // Create folder
                                        string out_folder = out_prefix + folder;
                                        createFolder(out_folder);
                                        string out_file_folder = out_folder + "f" + to_string(f) + "/";
                                        createFolder(out_file_folder);
                                        string rev_folder = out_file_folder + "revenue/";
                                        createFolder(rev_folder);
                                        string dec_folder = out_file_folder + "decision/";
                                        createFolder(dec_folder);

                                        // Store
                                        DP.storeResult(planner::DPPlanner::revenue, rev_folder);
                                        DP.storeResult(planner::DPPlanner::decision, dec_folder);
                                    }
                                    
                                    cout << "total time: " << compute_time << "\n";

                                }
                            }
                        }
                    }
                }
            }
        }
    }    
}

void callMyopic(planner::MyopicModelType model, planner::MyopicMode mode, int num_exper, bool store){
    // Scenarios
    std::string param_prefix = "debug/data/params/";
    std::string prob_prefix = "debug/data/prob/";
    std::string out_prefix = "debug/Myopic/";
    createFolder(out_prefix);

    std::string mt = "DP", m = "naive";
    if(model == planner::MyopicModelType::sp){
        mt = "SP";
    }
    if(mode == planner::MyopicMode::smart){
        m = "smart";
    }

    out_prefix = "debug/Myopic/" + mt + "_" + m + "/";
    createFolder(out_prefix);
    std::string out_rev_file = out_prefix + "S5I3C25K10T10debug.csv";
    int file_num = 1;

    const int K = 10, T = 10;
    const int days [] = {5};
    const int rooms [] = {3};
    const int caps [] = {25};
    const int m_NB [] = {5, 10, 15, 20};
    // const int bargs [] = {50, 70, 90};
    // const int no_ords [] = {1, 2, 3};
    // const int reqs [] = {25, 50, 75};

    // const int K = 5, T = 5;
    // const int days [] = {2, 3, 4, 5};
    // const int rooms [] = {2};
    // const int caps [] = {2};
    // const int m_NB [] = {5};
    // const int bargs [] = {50};
    // const int no_ords [] = {1};
    // const int reqs [] = {25};
    int early = 50, round = 1;
    double thd = 0.01;
    std::ofstream out(out_rev_file, std::ofstream::out|std::ofstream::app);
    std::string head = "Num_day,Num_room_type,Min_capacity,param_NB,param_barg,param_no_order,param_req,Num_period,Num_order,early,thd,file,round,num_exper,actual_exper,exp_rev,time\n";
    out << head;
    out.close();

    for(int s: days){
        for(int i: rooms){
            for(int c: caps){
                for(int m: m_NB){
                    set<int> bargs;
                    if(m == 10){
                        bargs.insert({50, 70, 90});
                    } else {
                        bargs.insert(70);
                    }
                    for(int b: bargs){
                        set<int> no_ords;
                        if(b == 70 && m == 10){
                            no_ords.insert({1, 2, 3});
                        } else {
                            no_ords.insert(2);
                        }
                        for(int r: no_ords){
                            set<int> reqs;
                            if(r == 2 && b == 70 && m == 10){
                                reqs.insert({10, 25, 40});
                            } else {
                                reqs.insert(25);
                            }
                            for(int d: reqs){
                                string folder = "S" + to_string(s) +
                                                "I" + to_string(i) +
                                                "C" + to_string(c) +
                                                "m" + to_string(m) +
                                                "b" + to_string(b) +
                                                "r" + to_string(r) + 
                                                "d" + to_string(d) +
                                                "K" + to_string(K) +
                                                "T" + to_string(T) + "/";
                                string param_folder = param_prefix + folder;
                                string prob_folder = prob_prefix + folder;
                                for(int f = 1; f <= file_num; f++){
                                    cout << "-------- File " + folder + to_string(f) + " start! --------\n";
                                    // File name
                                    string file = to_string(f) + ".txt";
                                    string prob = "T" + to_string(T) + "_";
                                    string param_path = param_folder + file;
                                    string prob_path = prob_folder + prob + file;
                                    
                                    // Read file
                                    data::Params params(param_path, prob_path);
                                    for(int rd = 1; rd <= round; rd++){
                                        cout << "-------- round " + to_string(rd) + " start! --------\n";
                                        // Initial
                                        planner::MyopicPlanner myopic(params, mode, model);
                                        
                                        // Run
                                        time_t start_time;
                                        time(&start_time);
                                        double revenue = myopic.run(num_exper, 1000, early, thd);
                                        auto avg_rev = myopic.getAvgRevs();
                                        time_t now_time;
                                        time(&now_time);
                                        double compute_time = difftime(now_time, start_time);
                                        
                                        
                                        std::ofstream out(out_rev_file, std::ofstream::out|std::ofstream::app);
                                        // Store time
                                        string msg = to_string(s) + ","
                                                    + to_string(i) + ","
                                                    + to_string(c) + ","
                                                    + to_string(m) + ","
                                                    + to_string(b) + ","
                                                    + to_string(r) + ","
                                                    + to_string(d) + ","
                                                    + to_string(T) + ","
                                                    + to_string(K) + ","
                                                    + to_string(early) + ","
                                                    + to_string(thd).substr(0, 4) + ","
                                                    + to_string(f) + ","
                                                    + to_string(rd) + ","
                                                    + to_string(num_exper) + ","
                                                    + to_string(avg_rev.size()) + ","
                                                    + to_string(revenue) + ","
                                                    + to_string(compute_time) + "\n";
                                        out << msg;
                                        out.close();
                                        // Store detail
                                        if(store) {
                                            string out_folder = out_prefix + folder;
                                            createFolder(out_folder);
                                            string out_file = out_folder + "debug_" + to_string(f) + "_r" + to_string(rd) + ".csv";
                                            myopic.storeRevs(out_file);
                                        }
                                        cout << "total time: " << compute_time << "\n";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
