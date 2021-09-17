#include <fstream>
#include <iostream>
#include <map>
#include <time.h>
#include <chrono>
#include <random>
#include "experimentor.hpp"
#include "data.hpp"

#undef DEBUG
// #define DEBUG
namespace experimentor {

MDUpgrade::MDUpgrade(){
    this->num_day_ = 0;
    this->num_room_ = 0;
    this->period_ = 0;
    this->revenue_ = 0;
}

void MDUpgrade::ReadData(const std::string& path){
    std::ifstream input(path);
    // Read sets
    input >> this->num_day_ >> this->num_room_ >> this->period_;
    for(int i = 1; i <= this->num_room_; i++){
        this->upgrade_out_[i] = {};
        this->upgrade_in_[i] = {};
        for(int j = i + 1; j <= this->num_room_; j++){
            this->upgrade_out_[i].insert(j);
        }
        for(int j = 1; j <= i - 1; j++){
            this->upgrade_in_[i].insert(j);
        }
    }
    // Read upgrade_fees
    for(int i = 1; i <= this->num_room_; i++){
        if(!this->upgrade_out_[i].empty()){
            for(int j: this->upgrade_out_[i]){
                input >> this->upgrade_fees_[{i, j}];
            }
        }
    }
    // Read prices
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            input >> this->prices_[{s, i}];
        }
    }
    // Read capacity
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            input >> this->capacity_[{s, i}];
        }
    }
    
    // Read orders
    for(int i = 1; i <= this->num_room_; i++){
        input >> this->orders_[i];
    }
    // Read demands
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            input >> this->demands_[{s, i}];
        }
    }
    
    input.close();
}

double MDUpgrade::Run(){
    this->Reset();
    // Find min available room of each type among all days
    std::map<int, int> min_aval;
    for(int i = 1; i <= this->num_room_; i++){
        int min_room = this->capacity_.at({1, i});
        for(int s = 2; s <= this->num_day_; s++){
            if(this->capacity_.at({s, i}) < min_room){
                min_room = this->capacity_.at({s, i});
            }
        }
        min_aval[i] = min_room;
    }
    // Initialize
    std::map<int, int> num_upgrade_in;
    std::map<int, int> num_upgrade_out;
    for(int i = 1; i <= this->num_room_; i++){
        for(int j: this->upgrade_out_[i]){
            this->upgrade_[{i, j}] = 0;
        }
        num_upgrade_in[i] = 0;
        num_upgrade_out[i] = 0;
    }
    double revenue = 0;
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            int quantity = std::min(this->demands_.at({s, i}), 
                           this->capacity_.at({s, i}) - this->orders_.at(i));
            revenue += this->prices_.at({s, i}) * quantity;
            this->sold_[{s, i}] = quantity;
        }
    }
#ifdef DEBUG
    std::cout << "\nDEBUG: min_aval" << min_aval << "\n";
#endif

    // First we have to upgrade rooms that upgrade_LB > 0
    double total_margin = 0;
    std::map<data::tuple2d, int> alpha, beta;
    for(int i = this->num_room_; i >= 1; i--){
        int upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                                  - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
        while(upgrade_LB > 0){
            // Find margin of upgrade x rooms to type j
            double max_margin = -INT_MAX;
            int opt_to = 0, opt_num = 0;
            for(int j: this->upgrade_out_.at(i)){
                int upgrade_space = min_aval.at(j) - this->orders_.at(j) 
                                    - num_upgrade_in.at(j) + num_upgrade_out.at(j);
                int upgrade_UB = std::min(this->orders_.at(i) - num_upgrade_out.at(i), upgrade_space);
#ifdef DEBUG
                std::cout << "DEBUG: (i, j): (" << i << "," << j << ") |LB: " << upgrade_LB << " |UB: " <<  upgrade_UB << "\n";
#endif
                // Compute parameters in the formulation of max u(x_ij)
                for(int s = 1; s <= this->num_day_; s++){
                    // Compute alpha
                    int adj_order_j = this->orders_.at(j) - num_upgrade_out.at(j) + num_upgrade_in.at(j);
                    alpha[{s, j}] = std::max(this->capacity_.at({s, j}) - adj_order_j - this->demands_.at({s,j}), 0);
                    // Compute beta
                    int adj_order_i = this->orders_.at(i) - num_upgrade_out.at(i) + num_upgrade_in.at(i);
                    beta[{s, i}] = std::max(this->capacity_.at({s, i}) - adj_order_i, this->demands_.at({s, i})) 
                                  - (this->capacity_.at({s, i}) - adj_order_i);
                }
                for(int x = 1; x <= upgrade_UB; x++){
                    double margin = 0;
                    for(int s = 1; s <= this->num_day_; s++){
                        double margin_x = this->upgrade_fees_.at({i, j}) * x;
                        double margin_alpha = this->prices_.at({s, j}) 
                                              * (x - std::min(alpha.at({s,j}), x));
                        double margin_beta = this->prices_.at({s, i}) 
                                              * std::min(beta.at({s,i}), x);
                        margin += margin_x - margin_alpha + margin_beta;
                    }
                    if(margin > max_margin){
                        max_margin = margin;
                        opt_to = j;
                        opt_num = x;
                    }
#ifdef DEBUG
                    std::cout << "DEBUG: x: " << x << " |margin:" << margin << "\n"; 
#endif
                }
            }
            total_margin += max_margin;
            this->upgrade_[{i, opt_to}] += opt_num;
            num_upgrade_in[opt_to] += opt_num;
            num_upgrade_out[i] += opt_num;
            upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                            - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
#ifdef DEBUG
            std::cout << "DEBUG: max_margin: " << max_margin 
                      << " |total_margin: " << total_margin
                      << " |from: " << i
                      << " |to: " << opt_to
                      << " |num: " << opt_num << "\n";
#endif  
        }
    }
#ifdef DEBUG
    std::cout << "\nDEBUG: Must Upgrade END!\n";
    std::cout << "DEBUG: total_margin: " << total_margin << "\n";
    std::cout << "DEBUG: num_upgrade_out: " << num_upgrade_out << "\n";
    std::cout << "DEBUG: num_upgrade_in: " << num_upgrade_in << "\n";
    std::cout << "DEBUG: upgrade: " << this->upgrade_ << "\n\n";
#endif
    // Then find other upgrade plans
    bool has_margin = true;
    while(has_margin){
        int from = 0, to = 0, num_upgrade = 0;
        double max_margin = 0;
        std::map<data::tuple2d, int> alpha, beta;
        // Find the best pair of upgrade in this run.
        for(int i = 1; i <= this->num_room_; i++){
            int upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                                      - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
            // Otherwise, find the optimal upgrade plan from i to j in this run
            for(int j: this->upgrade_out_.at(i)){
                int upgrade_space = min_aval.at(j) - this->orders_.at(j) 
                                    - num_upgrade_in.at(j) + num_upgrade_out.at(j);
                int upgrade_UB = std::min(this->orders_.at(i) - num_upgrade_out.at(i), upgrade_space);
#ifdef DEBUG
                std::cout << "DEBUG: (i, j): (" << i << "," << j << ") |LB: " << upgrade_LB << " |UB: " <<  upgrade_UB << "\n";
#endif
                // If there is no capacity for room i to upgrade, then skip.
                if(upgrade_LB > upgrade_UB || upgrade_UB == 0){
                    continue;
                }
                // Compute parameters in the formulation of max u(x_ij)
                for(int s = 1; s <= this->num_day_; s++){
                    // Compute alpha
                    int adj_order_j = this->orders_.at({j}) - num_upgrade_out.at(j) + num_upgrade_in.at(j);
                    alpha[{s, j}] = std::max(this->capacity_.at({s, j}) - adj_order_j - this->demands_.at({s,j}), 0);
                    // Compute beta
                    int adj_order_i = this->orders_.at({i}) - num_upgrade_out.at(i) + num_upgrade_in.at(i);
                    beta[{s, i}] = std::max(this->capacity_.at({s, i}) - adj_order_i, this->demands_.at({s, i})) 
                                  - (this->capacity_.at({s, i}) - adj_order_i);
                }
                // Find the optimal upgraded number of rooms from i to j.
                double max_margin_ij = 0;
                int opt_num_upgrade_ij = 0;
                double upgrade_fee = this->upgrade_fees_.at({i, j});
                // Find opt. sol. with loop, since the margin function is the sum of 
                // piecewise linear concave functions.
                for(int x = upgrade_LB; x <= upgrade_UB; x++){
                    double margin = 0;
                    for(int s = 1; s <= this->num_day_; s++){
                        double margin_x = upgrade_fee * x;
                        double margin_alpha = this->prices_.at({s, j}) 
                                              * (x - std::min(alpha.at({s,j}), x));
                        double margin_beta = this->prices_.at({s, i}) 
                                              * std::min(beta.at({s,i}), x);
                        margin += margin_x - margin_alpha + margin_beta;
                    }
                    if(margin > max_margin_ij){
                        max_margin_ij = margin;
                        opt_num_upgrade_ij = x;
                    }
#ifdef DEBUG
                    std::cout << "DEBUG: x: " << x << " |margin:" << margin << "\n"; 
#endif
                }            

                // Check if the margin from upgrading from i to j is the max
                // among all upgrade pairs currently.
                if(max_margin_ij > max_margin){
                    max_margin = max_margin_ij;
                    from = i;
                    to = j;
                    num_upgrade = opt_num_upgrade_ij; 
                }
            }
        }
        // if(from != 0 && to != 0 && num_upgrade != 0){
        if(max_margin > 0){
            // Store the optimal upgrade plan in this run
            total_margin += max_margin;
            this->upgrade_[{from, to}] += num_upgrade;
            num_upgrade_in[to] += num_upgrade;
            num_upgrade_out[from] += num_upgrade;
#ifdef DEBUG
            std::cout << "DEBUG: max_margin: " << max_margin 
                      << " |total_margin: " << total_margin
                      << " |from: " << from
                      << " |to: " << to
                      << " |num: " << num_upgrade << "\n";
#endif
        } else{
            has_margin = false;
        }
    }
    // Store result
    this->revenue_ = revenue + total_margin;
    // Correct the number of rooms sold to individuals.
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            int aval = this->capacity_.at({s, i}) - this->orders_.at(i)
                       + num_upgrade_out.at(i) - num_upgrade_in.at(i);
            this->sold_[{s, i}] = std::min(this->demands_.at({s, i}), aval);
            if(aval < 0){
                return -INT_MAX;
            }
        }
    }
    return this->revenue_;
}

void MDUpgrade::Reset(){
    this->revenue_ = 0;
    for(int i = 1; i <= this->num_room_; i++){
        for(int j: this->upgrade_out_.at(i)){
            this->upgrade_[{i, j}] = 0;
        }
        for(int s = 1; s <= this->num_day_; s++){
            this->sold_[{s, i}] = 0;
        }
    }
}

void MDUpgrade::StoreResult(const std::string& path_upgrade, const std::string& path_sold){
    // Write sold
    std::ofstream out(path_upgrade);
    std::string head = "Upgrade_from\\to";
    for(int i = 1; i <= this->num_room_; i++){
        head += "," + std::to_string(i);
    }
    out << head << "\n";
    for(int i = 1; i <= this->num_room_; i++){
        std::string msg = std::to_string(i);
        for(int j = 1; j <= this->num_room_; j++){
            int num_up = 0;
            if(this->upgrade_out_.at(i).count(j)){
                num_up = this->upgrade_.at({i, j});
            }
            msg += "," + std::to_string(num_up);
        }
        out << msg << "\n";
    }
    out.close();
    // Write upgrade
    out.open(path_sold, std::ofstream::out);
    head = "Day\\room";
    for(int i = 1; i <= this->num_room_; i++){
        head += "," + std::to_string(i);
    }
    out << head << "\n";
    for(int s = 1; s <= this->num_day_; s++){
        std::string msg = std::to_string(s);
        for(int i = 1; i <= this->num_room_; i++){
            msg += "," + std::to_string(this->sold_.at({s, i}));
        }
        out << msg << "\n";
    }
    out.close();
}

void MDUpgrade::PrintData(){
    std::cout << "Set -------------------------:\n";
    std::cout << "S: " << this->num_day_ << " | I: " 
              << this->num_room_ << " | t: " << this->period_ <<  "\n";
    std::cout << "I_out: " << this->upgrade_out_ << "\n";
    std::cout << "I_in: " << this->upgrade_in_ << "\n";
    std::cout << "Parameters -------------------------:\n";
    std::cout << "Upgrade_fees: " << this->upgrade_fees_ << "\n";
    std::cout << "Prices: " << this->prices_ << "\n";
    std::cout << "Capacity: " << this->capacity_ << "\n";
    std::cout << "Demands: " << this->demands_ << "\n";
    std::cout << "Orders: " << this->orders_ << "\n";
}

void MDUpgrade::PrintResult(){
    std::cout << "Results -------------------------:\n";
    std::cout << "Upgrade: " << this->upgrade_ << "\n";
    std::cout << "Sold " << this->sold_ << "\n";
    std::cout << "Revenue " << this->revenue_ << "\n";
}

void Debug(int day, int room, int cap, int p, int t, int file){
    // Scenarios
    std::string input_folder_prefix = "input/MDexperiment/";
    std::string out_folder_prefix = "result/MDexperiment/Myopic/";
    std::string scenario = "S" + std::to_string(day)
                           + "I" + std::to_string(room)
                           + "C" + std::to_string(cap)
                           + "p" + std::to_string(p)
                           + "t" + std::to_string(t);
    // Input
    std::string folder = input_folder_prefix 
                            + scenario + "/";
    std::string path = folder + std::to_string(file) + ".txt";
    std::cout << "Scenario: " << scenario << "_" 
                << std::to_string(file) 
                << "---------------------------------------\n";
    // Run myopic
    MDUpgrade md;
    md.ReadData(path);
    md.PrintData();
    double rev = md.Run();
    md.PrintResult();
}

void PerformExpers(){
    // Scenarios
    std::string input_folder_prefix = "input/MDexperiment/";
    std::string out_folder_prefix = "result/MDexperiment/Myopic/";
    system(("mkdir -p "+ out_folder_prefix).c_str());
    std::string out_rev_file = out_folder_prefix + "revenue.csv";
    int file_num = 20;
    // const int days [] = {2};
    // const int rooms [] = {2};
    // const int caps [] = {5};
    // const int p_NB [] = {25};
    const int days [] = {2, 7, 14};
    const int rooms [] = {2, 3, 4, 5};
    const int caps [] = {5, 25, 100};
    const int p_NB [] = {25, 50, 75};
    const int periods [] = {2, 5, 10, 15};

    std::ofstream out_rev(out_rev_file);
    std::string head = 
        "Num_day,Num_room_type,Min_capacity,prob_NB,period,file,revenue,time\n";
    out_rev << head;
    for(int day: days){
        for(int room: rooms){
            for(int cap: caps){
                for(int p: p_NB){
                    for(int t: periods){
                        std::string scenario = "S" + std::to_string(day)
                                            + "I" + std::to_string(room)
                                            + "C" + std::to_string(cap)
                                            + "p" + std::to_string(p)
                                            + "t" + std::to_string(t);
                        for(int i = 1; i <= file_num; i++){
                            // Input
                            std::string folder = input_folder_prefix 
                                                + scenario + "/";
                            std::string path = folder + std::to_string(i) + ".txt";
                            std::cout << "Scenario: " << scenario << "_" 
                                    << std::to_string(i) 
                                    << "---------------------------------------\n";
                            // Run myopic
                            MDUpgrade md;
                            md.ReadData(path);
                            time_t run_start;
                            time(&run_start);
                            // md.PrintData();
                            double rev = md.Run();
                            time_t time_now;
                            time(&time_now);
                            // md.PrintResult();
                            // Store revenue
                            std::string msg = std::to_string(day) + ","
                                            + std::to_string(room) + ","
                                            + std::to_string(cap) + ","
                                            + std::to_string(p) + ","
                                            + std::to_string(t) + ","
                                            + std::to_string(i) + ","
                                            + std::to_string(int(rev)) + ","
                                            + std::to_string(difftime(time_now, run_start)) + "\n";
                            out_rev << msg;
                            // Store result
                            std::string out_folder = out_folder_prefix
                                                    + scenario + "/";
                            system(("mkdir -p "+ out_folder).c_str());
                            std::string path_up = out_folder + "upgrade_"
                                                + std::to_string(i) + ".csv";
                            std::string path_sold = out_folder + "sold_"
                                                + std::to_string(i) + ".csv";
                            md.StoreResult(path_up, path_sold);
                        }
                    }
                }
            }
        }
    }
}

void MSUpgrade::test(){
    this->GuessDemand();
}

// GuessDemand roll dies up to num_exper_ times.
// Each time random a series of demands
void MSUpgrade::GuessDemand(){
    for(int w = 0; w < this->num_exper_; w++){
        // Initialize
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine generator (seed);
        std::uniform_real_distribution<double> distribution (0.0, 1.0);
        // Guess demand
        std::map<data::tuple2d, int> demand;
        for(int s = 1; s <= this->num_day_; s++){
            for(int i = 1; i <= this->num_room_; i++){
                double prob = distribution(generator);
                int num = 0;
                for(int c = 0; c <= this->capacity_.at({s, i}); c++){
                    if(prob <= this->cdf_.at({s, i}).at(c)){
                        num = c;
                        break;
                    }
                }
                demand[{s, i}] = num;
            }
        }
        // std::cout << "DEBUG: Scenario " << w << ": " << demand << "\n";
        this->s_demands_.push_back(demand);
    }
}

double MSUpgrade::Run(){
    this->Reset();
    // Find min available room of each type among all days
    std::map<int, int> min_aval;
    for(int i = 1; i <= this->num_room_; i++){
        int min_room = this->capacity_.at({1, i});
        for(int s = 2; s <= this->num_day_; s++){
            if(this->capacity_.at({s, i}) < min_room){
                min_room = this->capacity_.at({s, i});
            }
        }
        min_aval[i] = min_room;
    }
    // Initialize
    std::map<int, int> num_upgrade_in;
    std::map<int, int> num_upgrade_out;
    for(int i = 1; i <= this->num_room_; i++){
        for(int j: this->upgrade_out_[i]){
            this->upgrade_[{i, j}] = 0;
        }
        num_upgrade_in[i] = 0;
        num_upgrade_out[i] = 0;
    }
    double revenue = 0;
    for(int w = 0; w < this->num_scenarios_; w++){
        for(int s = 1; s <= this->num_day_; s++){
            for(int i = 1; i <= this->num_room_; i++){
                int quantity = std::min(this->s_demands_[w].at({s, i}), 
                            this->capacity_.at({s, i}) - this->orders_.at(i));
                revenue += this->prices_.at({s, i}) * quantity;
            }
        }
    }
    revenue = revenue / this->num_scenarios_;

#ifdef DEBUG
    std::cout << "\nDEBUG: min_aval" << min_aval << "\n";
#endif

    // First we have to upgrade rooms that upgrade_LB > 0
    double total_margin = 0;
    for(int i = this->num_room_; i >= 1; i--){
        int upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                                  - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
        while(upgrade_LB > 0){
            // Find margin of upgrade x rooms to type j
            double max_margin = -INT_MAX;
            int opt_to = 0, opt_num = 0;
            for(int j: this->upgrade_out_.at(i)){
                int upgrade_space = min_aval.at(j) - this->orders_.at(j) 
                                    - num_upgrade_in.at(j) + num_upgrade_out.at(j);
                int upgrade_UB = std::min(this->orders_.at(i) - num_upgrade_out.at(i), upgrade_space);
#ifdef DEBUG
                std::cout << "DEBUG: (i, j): (" << i << "," << j << ") |LB: " << upgrade_LB << " |UB: " <<  upgrade_UB << "\n";
#endif
                // Compute parameters in the formulation of max u(x_ij)
                std::map<data::tuple3d, int>  alpha, beta;
                for(int w = 0; w < this->num_scenarios_; w++){
                    for(int s = 1; s <= this->num_day_; s++){
                        // Compute alpha
                        int adj_order_j = this->orders_.at(j) - num_upgrade_out.at(j) + num_upgrade_in.at(j);
                        alpha[{w, s, j}] = std::max(this->capacity_.at({s, j}) - adj_order_j - this->s_demands_[w].at({s, j}), 0);
                        // Compute beta
                        int adj_order_i = this->orders_.at(i) - num_upgrade_out.at(i) + num_upgrade_in.at(i);
                        beta[{w, s, i}] = std::max(this->capacity_.at({s, i}) - adj_order_i, this->s_demands_[w].at({s, i})) 
                                    - (this->capacity_.at({s, i}) - adj_order_i);
                    }
                }
                
                for(int x = 1; x <= upgrade_UB; x++){
                    double exp_margin = 0;
                    for(int w = 0; w < this->num_scenarios_; w++){
                        double margin = 0;
                        for(int s = 1; s <= this->num_day_; s++){
                            double margin_x = this->upgrade_fees_.at({i, j}) * x;
                            double margin_alpha = this->prices_.at({s, j}) 
                                                * (x - std::min(alpha.at({w, s,j}), x));
                            double margin_beta = this->prices_.at({s, i}) 
                                                * std::min(beta.at({w, s,i}), x);
                            margin += margin_x - margin_alpha + margin_beta;
                        }
                        exp_margin += margin;
                    }
                    exp_margin = exp_margin / this->num_scenarios_;

                    if(exp_margin > max_margin){
                        max_margin = exp_margin;
                        opt_to = j;
                        opt_num = x;
                    }
#ifdef DEBUG
                    std::cout << "DEBUG: x: " << x << " |margin:" << exp_margin << "\n"; 
#endif
                }
            }
            total_margin += max_margin;
            this->upgrade_[{i, opt_to}] += opt_num;
            num_upgrade_in[opt_to] += opt_num;
            num_upgrade_out[i] += opt_num;
            upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                            - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
#ifdef DEBUG
            std::cout << "DEBUG: max_margin: " << max_margin 
                      << " |total_margin: " << total_margin
                      << " |from: " << i
                      << " |to: " << opt_to
                      << " |num: " << opt_num << "\n";
#endif  
        }
    }
#ifdef DEBUG
    std::cout << "\nDEBUG: Must Upgrade END!\n";
    std::cout << "DEBUG: total_margin: " << total_margin << "\n";
    std::cout << "DEBUG: num_upgrade_out: " << num_upgrade_out << "\n";
    std::cout << "DEBUG: num_upgrade_in: " << num_upgrade_in << "\n";
    std::cout << "DEBUG: upgrade: " << this->upgrade_ << "\n\n";
#endif
    // Then find other upgrade plans
    bool has_margin = true;
    while(has_margin){
        int from = 0, to = 0, num_upgrade = 0;
        double max_margin = 0;
        // std::map<data::tuple2d, int> alpha, beta;
        // Find the best pair of upgrade in this run.
        for(int i = 1; i <= this->num_room_; i++){
            int upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                                      - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
            // Otherwise, find the optimal upgrade plan from i to j in this run
            for(int j: this->upgrade_out_.at(i)){
                int upgrade_space = min_aval.at(j) - this->orders_.at(j) 
                                    - num_upgrade_in.at(j) + num_upgrade_out.at(j);
                int upgrade_UB = std::min(this->orders_.at(i) - num_upgrade_out.at(i), upgrade_space);
#ifdef DEBUG
                std::cout << "DEBUG: (i, j): (" << i << "," << j << ") |LB: " << upgrade_LB << " |UB: " <<  upgrade_UB << "\n";
#endif
                // If there is no capacity for room i to upgrade, then skip.
                if(upgrade_LB > upgrade_UB || upgrade_UB == 0){
                    continue;
                }
                // Compute parameters in the formulation of max u(x_ij)
                std::map<data::tuple3d, int>  alpha, beta;
                for(int w = 0; w < this->num_scenarios_; w++){
                    for(int s = 1; s <= this->num_day_; s++){
                        // Compute alpha
                        int adj_order_j = this->orders_.at(j) - num_upgrade_out.at(j) + num_upgrade_in.at(j);
                        alpha[{w, s, j}] = std::max(this->capacity_.at({s, j}) - adj_order_j - this->s_demands_[w].at({s, j}), 0);
                        // Compute beta
                        int adj_order_i = this->orders_.at(i) - num_upgrade_out.at(i) + num_upgrade_in.at(i);
                        beta[{w, s, i}] = std::max(this->capacity_.at({s, i}) - adj_order_i, this->s_demands_[w].at({s, i})) 
                                    - (this->capacity_.at({s, i}) - adj_order_i);
                    }
                }
                // Find the optimal upgraded number of rooms from i to j.
                double max_margin_ij = 0;
                int opt_num_upgrade_ij = 0;
                double upgrade_fee = this->upgrade_fees_.at({i, j});
                // Find opt. sol. with loop, since the margin function is the sum of 
                // piecewise linear concave functions.
                for(int x = upgrade_LB; x <= upgrade_UB; x++){
                    double exp_margin = 0;
                    for(int w = 0; w < this->num_scenarios_; w++){
                        double margin = 0;
                        for(int s = 1; s <= this->num_day_; s++){
                            double margin_x = this->upgrade_fees_.at({i, j}) * x;
                            double margin_alpha = this->prices_.at({s, j}) 
                                                * (x - std::min(alpha.at({w, s,j}), x));
                            double margin_beta = this->prices_.at({s, i}) 
                                                * std::min(beta.at({w, s,i}), x);
                            margin += margin_x - margin_alpha + margin_beta;
                        }
                        exp_margin += margin;
                    }
                    exp_margin = exp_margin / this->num_scenarios_;
                    
                    if(exp_margin > max_margin_ij){
                        max_margin_ij = exp_margin;
                        opt_num_upgrade_ij = x;
                    }
#ifdef DEBUG
                    std::cout << "DEBUG: x: " << x << " |margin:" << exp_margin << "\n"; 
#endif
                }            

                // Check if the margin from upgrading from i to j is the max
                // among all upgrade pairs currently.
                if(max_margin_ij > max_margin){
                    max_margin = max_margin_ij;
                    from = i;
                    to = j;
                    num_upgrade = opt_num_upgrade_ij; 
                }
            }
        }
        // if(from != 0 && to != 0 && num_upgrade != 0){
        if(max_margin > 0){
            // Store the optimal upgrade plan in this run
            total_margin += max_margin;
            this->upgrade_[{from, to}] += num_upgrade;
            num_upgrade_in[to] += num_upgrade;
            num_upgrade_out[from] += num_upgrade;
#ifdef DEBUG
            std::cout << "DEBUG: max_margin: " << max_margin 
                      << " |total_margin: " << total_margin
                      << " |from: " << from
                      << " |to: " << to
                      << " |num: " << num_upgrade << "\n";
#endif
        } else{
            has_margin = false;
        }
    }
    // Store result
    this->revenue_ = revenue + total_margin;

    return this->revenue_;
}

void MSUpgrade::ReadData(const std::string& path){
    std::ifstream input(path);
    // Read sets
    input >> this->num_day_ >> this->num_room_ >> this->period_ >> this->num_scenarios_;
    for(int i = 1; i <= this->num_room_; i++){
        this->upgrade_out_[i] = {};
        this->upgrade_in_[i] = {};
        for(int j = i + 1; j <= this->num_room_; j++){
            this->upgrade_out_[i].insert(j);
        }
        for(int j = 1; j <= i - 1; j++){
            this->upgrade_in_[i].insert(j);
        }
    }
    // Read upgrade_fees
    for(int i = 1; i <= this->num_room_; i++){
        if(!this->upgrade_out_[i].empty()){
            for(int j: this->upgrade_out_[i]){
                input >> this->upgrade_fees_[{i, j}];
            }
        }
    }
    // Read prices
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            input >> this->prices_[{s, i}];
        }
    }
    // Read capacity
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            input >> this->capacity_[{s, i}];
        }
    }
    
    // Read orders
    for(int i = 1; i <= this->num_room_; i++){
        input >> this->orders_[i];
    }

    // Read demands
    for(int w = 0; w < this->num_scenarios_; w++){
        std::map<data::tuple2d, int> demand;
        for(int s = 1; s <= this->num_day_; s++){
            for(int i = 1; i <= this->num_room_; i++){
                input >> demand[{s, i}];
            }
        }
        this->s_demands_.push_back(demand);
    }
    input.close();
}

void MSUpgrade::PrintData(){
    std::cout << "Set -------------------------:\n";
    std::cout << "S: " << this->num_day_ << " | I: " 
              << this->num_room_ << " | t: " << this->period_ <<  "\n";
    std::cout << "I_out: " << this->upgrade_out_ << "\n";
    std::cout << "I_in: " << this->upgrade_in_ << "\n";
    std::cout << "Parameters -------------------------:\n";
    std::cout << "Upgrade_fees: " << this->upgrade_fees_ << "\n";
    std::cout << "Prices: " << this->prices_ << "\n";
    std::cout << "Capacity: " << this->capacity_ << "\n";
    std::cout << "Orders: " << this->orders_ << "\n";
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            std::cout << "Prob (" << s << "," << i << "): " << this->prob_.at({s, i}) << "\n";
        }
    }
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            std::cout << "cdf (" << s << "," << i << "): " << this->cdf_.at({s, i}) << "\n";
        }
    }
    
}

void MSUpgrade::StoreResult(const std::string& path_upgrade){
    std::ofstream out(path_upgrade);
    std::string head = "Upgrade_from\\to";
    for(int i = 1; i <= this->num_room_; i++){
        head += "," + std::to_string(i);
    }
    out << head << "\n";
    for(int i = 1; i <= this->num_room_; i++){
        std::string msg = std::to_string(i);
        for(int j = 1; j <= this->num_room_; j++){
            int num_up = 0;
            if(this->upgrade_out_.at(i).count(j)){
                num_up = this->upgrade_.at({i, j});
            }
            msg += "," + std::to_string(num_up);
        }
        out << msg << "\n";
    }
    out.close();
}

void DebugMS(int day, int room, int cap, int p, int t, int file){
    // Scenarios
    std::string input_folder_prefix = "input/MSexperiment/";
    // std::string out_folder_prefix = "result/MSexperiment/Myopic/";
    std::string scenario = "S" + std::to_string(day)
                           + "I" + std::to_string(room)
                           + "C" + std::to_string(cap)
                           + "p" + std::to_string(p)
                           + "t" + std::to_string(t);
    // Input
    std::string folder = input_folder_prefix 
                            + scenario + "/";
    std::string path = folder + std::to_string(file) + ".txt";
    std::cout << "Scenario: " << scenario << "_" 
                << std::to_string(file) 
                << "---------------------------------------\n";
    // Run myopic
    MSUpgrade ms;
    ms.ReadData(path);
    ms.PrintData();
    ms.test();
    double rev = ms.Run();
    ms.PrintResult();
}

void PerformExpersMS(){
    // Scenarios
    std::string input_folder_prefix = "input/MSexperiment/";
    std::string out_folder_prefix = "result/MSexperiment/Myopic/";
    system(("mkdir -p "+ out_folder_prefix).c_str());
    std::string out_rev_file = out_folder_prefix + "revenue.csv";
    int file_num = 20;
    // const int days [] = {2};
    // const int rooms [] = {2};
    // const int caps [] = {5};
    // const int p_NB [] = {25};
    const int days [] = {2, 7, 14};
    const int rooms [] = {2, 3, 4, 5};
    const int caps [] = {5, 25, 100};
    const int p_NB [] = {25, 50, 75};
    const int periods [] = {2, 5, 10, 15};

    std::ofstream out_rev(out_rev_file);
    std::string head = 
        "Num_day,Num_room_type,Min_capacity,prob_NB,period,file,revenue,time\n";
    out_rev << head;
    for(int day: days){
        for(int room: rooms){
            for(int cap: caps){
                for(int p: p_NB){
                    for(int t: periods){
                        std::string scenario = "S" + std::to_string(day)
                                            + "I" + std::to_string(room)
                                            + "C" + std::to_string(cap)
                                            + "p" + std::to_string(p)
                                            + "t" + std::to_string(t);
                        for(int i = 1; i <= file_num; i++){
                            // Input
                            std::string folder = input_folder_prefix 
                                                + scenario + "/";
                            std::string path = folder + std::to_string(i) + ".txt";
                            std::cout << "Scenario: " << scenario << "_" 
                                    << std::to_string(i) 
                                    << "---------------------------------------\n";
                            // Run myopic
                            MSUpgrade ms;
                            ms.ReadData(path);
                            time_t run_start;
                            time(&run_start);
                            // md.PrintData();
                            double rev = ms.Run();
                            time_t time_now;
                            time(&time_now);
                            // md.PrintResult();
                            // Store revenue
                            std::string msg = std::to_string(day) + ","
                                            + std::to_string(room) + ","
                                            + std::to_string(cap) + ","
                                            + std::to_string(p) + ","
                                            + std::to_string(t) + ","
                                            + std::to_string(i) + ","
                                            + std::to_string(rev) + ","
                                            + std::to_string(difftime(time_now, run_start)) + "\n";
                            out_rev << msg;
                            // Store result
                            std::string out_folder = out_folder_prefix
                                                    + scenario + "/";
                            system(("mkdir -p "+ out_folder).c_str());
                            std::string path_up = out_folder + "upgrade_"
                                                + std::to_string(i) + ".csv";
                            ms.StoreResult(path_up);
                        }
                    }
                }
            }
        }
    }
}

void MSUpgradeSmart::ReadData(const std::string& path){
    std::ifstream input(path);
    // Read sets
    input >> this->num_day_ >> this->num_room_ >> this->period_ >> this->num_scenarios_;
    for(int i = 1; i <= this->num_room_; i++){
        this->upgrade_out_[i] = {};
        this->upgrade_in_[i] = {};
        for(int j = i + 1; j <= this->num_room_; j++){
            this->upgrade_out_[i].insert(j);
        }
        for(int j = 1; j <= i - 1; j++){
            this->upgrade_in_[i].insert(j);
        }
    }
    // Read upgrade_fees
    for(int i = 1; i <= this->num_room_; i++){
        if(!this->upgrade_out_[i].empty()){
            for(int j: this->upgrade_out_[i]){
                input >> this->upgrade_fees_[{i, j}];
            }
        }
    }
    // Read capacity
    for(int s = 1; s <= this->num_day_; s++){
        for(int i = 1; i <= this->num_room_; i++){
            input >> this->capacity_[{s, i}];
        }
    }
    
    // Read orders
    for(int i = 1; i <= this->num_room_; i++){
        input >> this->orders_[i];
    }

    // Read prices
    for(int w = 0; w < this->num_scenarios_; w++){
        std::map<data::tuple2d, double> prices;
        for(int s = 1; s <= this->num_day_; s++){
            for(int i = 1; i <= this->num_room_; i++){
                input >> prices[{s, i}];
            }
        }
        this->s_prices_.push_back(prices);
    }

    // Read demands
    for(int w = 0; w < this->num_scenarios_; w++){
        std::map<data::tuple2d, int> demand;
        for(int s = 1; s <= this->num_day_; s++){
            for(int i = 1; i <= this->num_room_; i++){
                input >> demand[{s, i}];
            }
        }
        this->s_demands_.push_back(demand);
    }
    input.close();
}

double MSUpgradeSmart::Run(){
    this->Reset();
    // Find min available room of each type among all days
    std::map<int, int> min_aval;
    for(int i = 1; i <= this->num_room_; i++){
        int min_room = this->capacity_.at({1, i});
        for(int s = 2; s <= this->num_day_; s++){
            if(this->capacity_.at({s, i}) < min_room){
                min_room = this->capacity_.at({s, i});
            }
        }
        min_aval[i] = min_room;
    }
    // Initialize
    std::map<int, int> num_upgrade_in;
    std::map<int, int> num_upgrade_out;
    for(int i = 1; i <= this->num_room_; i++){
        for(int j: this->upgrade_out_[i]){
            this->upgrade_[{i, j}] = 0;
        }
        num_upgrade_in[i] = 0;
        num_upgrade_out[i] = 0;
    }
    double revenue = 0;
    for(int w = 0; w < this->num_scenarios_; w++){
        for(int s = 1; s <= this->num_day_; s++){
            for(int i = 1; i <= this->num_room_; i++){
                int quantity = std::min(this->s_demands_[w].at({s, i}), 
                            this->capacity_.at({s, i}) - this->orders_.at(i));
                revenue += this->s_prices_[w].at({s, i}) * quantity;
            }
        }
    }
    revenue = revenue / this->num_scenarios_;

#ifdef DEBUG
    std::cout << "\nDEBUG: min_aval" << min_aval << "\n";
#endif

    // First we have to upgrade rooms that upgrade_LB > 0
    double total_margin = 0;
    for(int i = this->num_room_; i >= 1; i--){
        int upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                                  - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
        while(upgrade_LB > 0){
            // Find margin of upgrade x rooms to type j
            double max_margin = -INT_MAX;
            int opt_to = 0, opt_num = 0;
            for(int j: this->upgrade_out_.at(i)){
                int upgrade_space = min_aval.at(j) - this->orders_.at(j) 
                                    - num_upgrade_in.at(j) + num_upgrade_out.at(j);
                int upgrade_UB = std::min(this->orders_.at(i) - num_upgrade_out.at(i), upgrade_space);
#ifdef DEBUG
                std::cout << "DEBUG: (i, j): (" << i << "," << j << ") |LB: " << upgrade_LB << " |UB: " <<  upgrade_UB << "\n";
#endif
                // Compute parameters in the formulation of max u(x_ij)
                std::map<data::tuple3d, int>  alpha, beta;
                for(int w = 0; w < this->num_scenarios_; w++){
                    for(int s = 1; s <= this->num_day_; s++){
                        // Compute alpha
                        int adj_order_j = this->orders_.at(j) - num_upgrade_out.at(j) + num_upgrade_in.at(j);
                        alpha[{w, s, j}] = std::max(this->capacity_.at({s, j}) - adj_order_j - this->s_demands_[w].at({s, j}), 0);
                        // Compute beta
                        int adj_order_i = this->orders_.at(i) - num_upgrade_out.at(i) + num_upgrade_in.at(i);
                        beta[{w, s, i}] = std::max(this->capacity_.at({s, i}) - adj_order_i, this->s_demands_[w].at({s, i})) 
                                    - (this->capacity_.at({s, i}) - adj_order_i);
                    }
                }
                
                for(int x = 1; x <= upgrade_UB; x++){
                    double exp_margin = 0;
                    for(int w = 0; w < this->num_scenarios_; w++){
                        double margin = 0;
                        for(int s = 1; s <= this->num_day_; s++){
                            double margin_x = this->upgrade_fees_.at({i, j}) * x;
                            double margin_alpha = this->s_prices_[w].at({s, j}) 
                                                * (x - std::min(alpha.at({w, s,j}), x));
                            double margin_beta = this->s_prices_[w].at({s, i}) 
                                                * std::min(beta.at({w, s,i}), x);
                            margin += margin_x - margin_alpha + margin_beta;
                        }
                        exp_margin += margin;
                    }
                    exp_margin = exp_margin / this->num_scenarios_;

                    if(exp_margin > max_margin){
                        max_margin = exp_margin;
                        opt_to = j;
                        opt_num = x;
                    }
#ifdef DEBUG
                    std::cout << "DEBUG: x: " << x << " |margin:" << exp_margin << "\n"; 
#endif
                }
            }
            total_margin += max_margin;
            this->upgrade_[{i, opt_to}] += opt_num;
            num_upgrade_in[opt_to] += opt_num;
            num_upgrade_out[i] += opt_num;
            upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                            - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
#ifdef DEBUG
            std::cout << "DEBUG: max_margin: " << max_margin 
                      << " |total_margin: " << total_margin
                      << " |from: " << i
                      << " |to: " << opt_to
                      << " |num: " << opt_num << "\n";
#endif  
        }
    }
#ifdef DEBUG
    std::cout << "\nDEBUG: Must Upgrade END!\n";
    std::cout << "DEBUG: total_margin: " << total_margin << "\n";
    std::cout << "DEBUG: num_upgrade_out: " << num_upgrade_out << "\n";
    std::cout << "DEBUG: num_upgrade_in: " << num_upgrade_in << "\n";
    std::cout << "DEBUG: upgrade: " << this->upgrade_ << "\n\n";
#endif
    // Then find other upgrade plans
    bool has_margin = true;
    while(has_margin){
        int from = 0, to = 0, num_upgrade = 0;
        double max_margin = 0;
        // std::map<data::tuple2d, int> alpha, beta;
        // Find the best pair of upgrade in this run.
        for(int i = 1; i <= this->num_room_; i++){
            int upgrade_LB = std::max(this->orders_.at(i) - min_aval.at(i)
                                      - num_upgrade_out.at(i) + num_upgrade_in.at(i), 0);
            // Otherwise, find the optimal upgrade plan from i to j in this run
            for(int j: this->upgrade_out_.at(i)){
                int upgrade_space = min_aval.at(j) - this->orders_.at(j) 
                                    - num_upgrade_in.at(j) + num_upgrade_out.at(j);
                int upgrade_UB = std::min(this->orders_.at(i) - num_upgrade_out.at(i), upgrade_space);
#ifdef DEBUG
                std::cout << "DEBUG: (i, j): (" << i << "," << j << ") |LB: " << upgrade_LB << " |UB: " <<  upgrade_UB << "\n";
#endif
                // If there is no capacity for room i to upgrade, then skip.
                if(upgrade_LB > upgrade_UB || upgrade_UB == 0){
                    continue;
                }
                // Compute parameters in the formulation of max u(x_ij)
                std::map<data::tuple3d, int>  alpha, beta;
                for(int w = 0; w < this->num_scenarios_; w++){
                    for(int s = 1; s <= this->num_day_; s++){
                        // Compute alpha
                        int adj_order_j = this->orders_.at(j) - num_upgrade_out.at(j) + num_upgrade_in.at(j);
                        alpha[{w, s, j}] = std::max(this->capacity_.at({s, j}) - adj_order_j - this->s_demands_[w].at({s, j}), 0);
                        // Compute beta
                        int adj_order_i = this->orders_.at(i) - num_upgrade_out.at(i) + num_upgrade_in.at(i);
                        beta[{w, s, i}] = std::max(this->capacity_.at({s, i}) - adj_order_i, this->s_demands_[w].at({s, i})) 
                                    - (this->capacity_.at({s, i}) - adj_order_i);
                    }
                }
                // Find the optimal upgraded number of rooms from i to j.
                double max_margin_ij = 0;
                int opt_num_upgrade_ij = 0;
                double upgrade_fee = this->upgrade_fees_.at({i, j});
                // Find opt. sol. with loop, since the margin function is the sum of 
                // piecewise linear concave functions.
                for(int x = upgrade_LB; x <= upgrade_UB; x++){
                    double exp_margin = 0;
                    for(int w = 0; w < this->num_scenarios_; w++){
                        double margin = 0;
                        for(int s = 1; s <= this->num_day_; s++){
                            double margin_x = this->upgrade_fees_.at({i, j}) * x;
                            double margin_alpha = this->s_prices_[w].at({s, j}) 
                                                * (x - std::min(alpha.at({w, s,j}), x));
                            double margin_beta = this->s_prices_[w].at({s, i}) 
                                                * std::min(beta.at({w, s,i}), x);
                            margin += margin_x - margin_alpha + margin_beta;
                        }
                        exp_margin += margin;
                    }
                    exp_margin = exp_margin / this->num_scenarios_;
                    
                    if(exp_margin > max_margin_ij){
                        max_margin_ij = exp_margin;
                        opt_num_upgrade_ij = x;
                    }
#ifdef DEBUG
                    std::cout << "DEBUG: x: " << x << " |margin:" << exp_margin << "\n"; 
#endif
                }            

                // Check if the margin from upgrading from i to j is the max
                // among all upgrade pairs currently.
                if(max_margin_ij > max_margin){
                    max_margin = max_margin_ij;
                    from = i;
                    to = j;
                    num_upgrade = opt_num_upgrade_ij; 
                }
            }
        }
        // if(from != 0 && to != 0 && num_upgrade != 0){
        if(max_margin > 0){
            // Store the optimal upgrade plan in this run
            total_margin += max_margin;
            this->upgrade_[{from, to}] += num_upgrade;
            num_upgrade_in[to] += num_upgrade;
            num_upgrade_out[from] += num_upgrade;
#ifdef DEBUG
            std::cout << "DEBUG: max_margin: " << max_margin 
                      << " |total_margin: " << total_margin
                      << " |from: " << from
                      << " |to: " << to
                      << " |num: " << num_upgrade << "\n";
#endif
        } else{
            has_margin = false;
        }
    }
    // Store result
    this->revenue_ = revenue + total_margin;

    return this->revenue_;
}

void MSUpgradeSmart::PrintData(){
    std::cout << "Set -------------------------:\n";
    std::cout << "S: " << this->num_day_ << " | I: " 
              << this->num_room_ << " | t: " << this->period_ <<  "\n";
    std::cout << "I_out: " << this->upgrade_out_ << "\n";
    std::cout << "I_in: " << this->upgrade_in_ << "\n";
    std::cout << "Parameters -------------------------:\n";
    std::cout << "Upgrade_fees: " << this->upgrade_fees_ << "\n";
    std::cout << "Capacity: " << this->capacity_ << "\n";
    std::cout << "Orders: " << this->orders_ << "\n";
    for(int w = 0; w < this->num_scenarios_; w++){
        std::cout << "Scenario " << w+1 << " | price: " << this->s_prices_[w]
                  << " | demands: " << this->s_demands_[w] << "\n";
    }   
}

void PerformExpersMSSmart(){
    // Scenarios
    std::string input_folder_prefix = "input/MSexperiment_smart/";
    std::string out_folder_prefix = "result/MSexperiment_smart/Myopic/";
    system(("mkdir -p "+ out_folder_prefix).c_str());
    std::string out_rev_file = out_folder_prefix + "revenue.csv";
    int file_num = 20;
    // const int days [] = {2};
    // const int rooms [] = {2};
    // const int caps [] = {5};
    // const int p_NB [] = {50};
    // const int periods [] = {2};
    const int days [] = {2, 7, 14};
    const int rooms [] = {2, 3, 4, 5};
    const int caps [] = {5, 25, 100};
    const int p_NB [] = {25, 50, 75};
    const int periods [] = {2, 5, 10, 15};

    std::ofstream out_rev(out_rev_file);
    std::string head = 
        "Num_day,Num_room_type,Min_capacity,prob_NB,period,file,revenue,time\n";
    out_rev << head;
    for(int day: days){
        for(int room: rooms){
            for(int cap: caps){
                for(int p: p_NB){
                    for(int t: periods){
                        std::string scenario = "S" + std::to_string(day)
                                            + "I" + std::to_string(room)
                                            + "C" + std::to_string(cap)
                                            + "p" + std::to_string(p)
                                            + "t" + std::to_string(t);
                        for(int i = 1; i <= file_num; i++){
                            // Input
                            std::string folder = input_folder_prefix 
                                                + scenario + "/";
                            std::string path = folder + std::to_string(i) + ".txt";
                            std::cout << "Scenario: " << scenario << "_" 
                                    << std::to_string(i) 
                                    << "---------------------------------------\n";
                            // Run myopic
                            MSUpgradeSmart ms;
                            ms.ReadData(path);
                            time_t run_start;
                            time(&run_start);
                            // ms.PrintData();
                            double rev = ms.Run();
                            time_t time_now;
                            time(&time_now);
                            // md.PrintResult();
                            // Store revenue
                            std::string msg = std::to_string(day) + ","
                                            + std::to_string(room) + ","
                                            + std::to_string(cap) + ","
                                            + std::to_string(p) + ","
                                            + std::to_string(t) + ","
                                            + std::to_string(i) + ","
                                            + std::to_string(rev) + ","
                                            + std::to_string(difftime(time_now, run_start)) + "\n";
                            out_rev << msg;
                            // Store result
                            std::string out_folder = out_folder_prefix
                                                    + scenario + "/";
                            system(("mkdir -p "+ out_folder).c_str());
                            std::string path_up = out_folder + "upgrade_"
                                                + std::to_string(i) + ".csv";
                            ms.StoreResult(path_up);
                        }
                    }
                }
            }
        }
    }
}

}