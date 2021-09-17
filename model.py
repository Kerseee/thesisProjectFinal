import random
import gurobipy as gp
import numpy as np
import os
import time
from scipy.stats import nbinom, poisson


def sort_dict(d):
    return {k:d[k] for k in sorted(d)}

def geometric(p,k):
    return pow((1 - p), k) * p

def poisson_pmf(mu, k):
    return poisson.pmf(k, mu)

class Order:
    def __init__(self, S, I, num, day_mode):
        self.request_day = ()
        self.order = {i:0 for i in range(1, I+1)}
        
        # Decide day
        if day_mode == 1:
            self.request_day = (1, 1)
        elif day_mode == 2:
            self.request_day = (1, S)
        elif day_mode == 3:
            self.request_day = (int(S/2), S)
        elif day_mode == 4:
            self.request_day = (1, int(round(S/2)))
        else:
            check_day = [random.randint(1, S) for i in range(2)]
            checkin, checkout = min(check_day), max(check_day)
            self.request_day = (checkin, checkout)

        # Decide order
        rooms = list(range(1, I+1))
        params = np.array([1/i for i in rooms])
        probs = params / sum(params)
        rand = np.random.choice(rooms, num, p=probs)
        for i in rand:
            self.order[i] += 1

class Data:
    basic_price = {1:1000, 2:1800, 3:2700, 4:3600, 5:4300}
    basic_prob = 0.4
    prob_dif = 0.025
    def __init__(self, S, I, C, K, T, m, b, r, d):
        """Create a data for DP and Myopic.

        Arguments:
        S -- number of service periods
        I -- number of rooms
        C -- capacity of each rooms
        K -- number of order type
        T -- number of booking periods
        m -- param of negative binomial distribution
        b -- discount rate for order prices
        r -- no order rate
        d -- param of total requested rooms in an order
        """
        self.S = S
        self.I = I
        self.C = C
        self.T = T
        self.K = K
        self.m = m
        self.b = b
        self.r = r
        self.d = d

        # Parameters
        self.upgrade_fees = {}
        self.upgrade_list = {}
        self.prices = {}    # prices of demand from individuals
        self.capacity = {}
        self.orders = {}
        self.request_day = {}
        self.price_order = {}

        # Probs
        self.prob_order = {}
        self.prob_ind = {}
    
    def create(self):
        rooms = range(1, self.I + 1)
        days = range(1, self.S + 1)
        periods = range(1, self.T + 1)
        agencies = range(1, self.K + 1)

        # Upgrade_list
        self.upgrade_list = {i:list(range(i+1, self.I+1)) for i in rooms}

        # capacity:
        self.capacity = {i:self.C for i in rooms}

        # prices
        for i in rooms:
            for s in days:
                noise = random.uniform(0.9, 1.1)
                self.prices[s, i] = int(round(Data.basic_price[i] * noise))
        
        # orders
        total_num = max(int(round(sum(self.capacity.values()) * self.d)), 2)

        orders = {k:Order(self.S, self.I, 
                          int(round(total_num * random.uniform(0.8, 1.2))), k) 
                  for k in agencies}
        self.request_day = {k:orders[k].request_day for k in orders}
        self.orders = {k:orders[k].order for k in orders}
        
        # price_order
        for k in agencies:
            checkin, checkout = self.request_day[k]
            price = sum([sum([self.orders[k][i] * self.prices[s, i] 
                        for i in self.orders[k]])
                        for s in range(checkin, checkout+1)])
            noise = random.uniform(0.9, 1.1)
            self.price_order[k] = int(round(price * self.b * noise))
            # print("DEBUG: k:{} | price: {} | price_order: {} | order: {}".format(k, price, self.price_order[k], self.orders[k]))

            
            
            # upgrade fee
            for i in self.upgrade_list:
                self.upgrade_fees[k, i] = {}
                for j in self.upgrade_list[i]:
                    noise = random.uniform(0.9, 1.1)
                    fee = (Data.basic_price[j] - Data.basic_price[i]) \
                          * self.b * noise
                    self.upgrade_fees[k, i][j] = int(round(fee))
        
        # prob_order
        self.prob_order = {k:(1-self.r)/self.K for k in agencies}

        # prob_ind
        for t in range(1, self.T + 1):
            self.prob_ind[t] = {}
            for s in days:
                self.prob_ind[t][s] = {}
                for i in rooms:
                    cap = self.capacity[i]
                    prob = self.m * (Data.basic_prob + (i - 1) * Data.prob_dif)
                    mu = (1 - prob)/prob
                    # self.prob_ind[t][s][i] = {c:geometric(prob, c) 
                    #                           for c in range(0, cap)}
                    self.prob_ind[t][s][i] = {c:poisson_pmf(mu, c) 
                                              for c in range(0, cap)}
                    self.prob_ind[t][s][i][cap] = 1 - sum(self.prob_ind[t][s][i].values())

                    

    def store_params(self, path):
        with open(path, 'w+') as file:
            # Write scale
            scale = [self.I, self.K, self.T, self.S]
            file.write(" ".join(map(str, scale)) + "\n")

            # Write requested day
            for k in range(1, self.K + 1):
                file.write(" ".join(map(str, self.request_day[k])) + "\n")
            
            # Write upgrade list
            for i in range(1, self.I + 1):
                file.write(str(len(self.upgrade_list[i])) + " ")
                file.write(" ".join(map(str, self.upgrade_list[i])) + "\n")
            
            # Write order price
            file.write(" ".join(map(str, self.price_order.values())) + "\n")

            # Write upgrade fee
            for k in range(1, self.K + 1):
                for i in range(1, self.I + 1):
                    file.write(str(i))
                    for j in range(i + 1, self.I + 1):
                        file.write(" " + str(self.upgrade_fees[k, i][j]))
                    file.write("\n")
            
            # Write demand in order
            for k in range(1, self.K + 1):
                file.write(" ".join(map(str, self.orders[k].values())) + "\n")
            
            # Write capacity
            file.write(" ".join(map(str, self.capacity.values())) + "\n")

            for s in range(1, self.S + 1):
                prices = [self.prices[s, i] for i in range(1, self.I + 1)]
                file.write(" ".join(map(str, prices)))
                file.write("\n")
        
        file.close()

    def store_prob(self, path, T):
        with open(path, 'w+') as file:
            # Write scale
            file.write(" ".join(map(str,[self.K, T])) + "\n")

            # Write prob order
            for t in range(1, T + 1):
                file.write(" ".join(map(str, self.prob_order.values())) + "\n")
            
            # Write S,I
            file.write(" ".join(map(str,[self.S, self.I])) + "\n")

            # write capacity
            file.write(" ".join(map(str, self.capacity.values())) + "\n")

            # Write prob ind
            for t in range(1, T + 1):
                for s in range(1, self.S + 1):
                    file.write(" ".join(map(str,[t, s])))
                    file.write("\n")
                    for i in range(1, self.I + 1):
                        file.write(" ".join(map(str, self.prob_ind[t][s][i].values())))
                        file.write("\n")
        
        file.close()

class UpgradeModelData:
    def __init__(self, S, I, C, p, t):
        """Create a data for upgrade model.

        Arguments:
        S -- number of service periods
        I -- number of rooms
        C -- basic capacity of each rooms
             (random from C to 2C)
        p -- param of negative binomial distribution
        """
        # Sets
        self.S = S
        self.I = I
        self.C = C
        self.p = p
        self.t = t

        # Parameters
        self.upgrade_fees = {}
        self.prices = {}    # prices of demand from individuals
        self.demands = {}   # demand from individuals
        self.capacity = {}
        self.orders = {}    # requested number of rooms from the travel agency
    
    def create(self):
        rooms = range(1, self.I + 1)
        days = range(1, self.S + 1)
        self.upgrade_out = {i:{j for j in range(i + 1, self.I + 1)} 
                            for i in rooms}
        self.upgrade_in = {i:{j for j in range(1, i)} for i in rooms}

        basic_price = random.randint(500, 1000)
        for i in rooms:
            rate = max(1, random.uniform(i - 0.5, i))
            for s in days:
                noise = random.uniform(0.9, 1.1)
                self.prices[s, i] = int(round(basic_price * rate * noise))
        
        for i in rooms:
            for j in self.upgrade_out[i]:
                rate = random.uniform(0.7, 0.95)
                self.upgrade_fees[i, j] = int(round((self.prices[1, j] 
                    - self.prices[1, i]) * rate))
        
        self.create_capacity()
        
        min_cap = {i:min([self.capacity[s, i] for s in days])
                   for i in rooms}
        for i in range(self.I, 0, -1):
            sum_cap_up = sum([min_cap[j] - self.orders[j] 
                            for j in self.upgrade_out[i]])
            max_aval = min_cap[i] + sum_cap_up
            self.orders[i] = random.randint(0, max_aval)
        
        self.create_demand()
    
    def create_capacity(self):
        rooms = range(1, self.I + 1)
        days = range(1, self.S + 1)
        self.capacity = {(s, i):random.randint(self.C, 2 * self.C) 
                         for s in days for i in rooms}
    
    def create_demand(self):
        rooms = range(1, self.I + 1)
        days = range(1, self.S + 1)
        self.demands = {(s, i):np.random.negative_binomial(self.t, self.p)
                        for s in days for i in rooms}

    def store_set(self, file):
        file.write(' '.join([str(self.S), str(self.I), str(self.t)]) + '\n')
    
    def store_demand(self, file):
        rooms = range(1, self.I + 1)
        days = range(1, self.S + 1)
        for s in days:
            demand = [str(self.demands[s, i]) for i in rooms]
            file.write(' '.join(demand) + '\n')
   
    def store(self, path):
        with open(path, 'w+') as file:
            # write sets
            self.store_set(file)
            rooms = range(1, self.I + 1)
            days = range(1, self.S + 1)

            # write upgrade fee
            for i in rooms:
                fees = [str(self.upgrade_fees[i, j]) for j in self.upgrade_out[i]]
                if(len(fees) > 0):
                    file.write(' '.join(fees) + '\n')
            
            # write price
            for s in days:
                prices = [str(self.prices[s, i]) for i in rooms]
                file.write(' '.join(prices) + '\n')
            
            # write capacity
            for s in days:
                capacity = [str(self.capacity[s, i]) for i in rooms]
                file.write(' '.join(capacity) + '\n')
            
            # write orders
            orders = [str(self.orders[i]) for i in rooms]
            file.write(' '.join(list(map(str, orders))) + '\n')
            
            # write demands
            self.store_demand(file)
        
        file.close()
       
class UpgradeModelDataSP(UpgradeModelData):
    def __init__(self, S, I, C, p, t, prob_thd=0.01, W=1000):
        super().__init__(S, I, C, p, t)
        # Add set: scenarios
        self.W = W
        # Add params: probabilities of scenarios
        self.probs = {}
        self.pmf = {}
        self.cdf = {}
        self.prob_thd = prob_thd
    
    def create_total_demand(self):
        rooms = range(1, self.I + 1)
        days = range(1, self.S + 1)

        for s in days:
            for i in rooms:
                cap = self.capacity[s, i]
                self.pmf[s, i] = {n:nbinom.pmf(n, self.t, self.p) for n in range(0, cap)}
                self.pmf[s, i][cap] = 1 - nbinom.cdf(cap - 1, self.t, self.p) \
                                        if cap > 0 else 1
                
        self.demands = {}
        num_rooms = {(s, i):0 for s in days for i in rooms}
        digit = [(s, i) for s in days for i in rooms]

        w = 1
        while(True):
            # Store scenario
            prob = 1
            self.demands[w] = {}
            for s in days:
                for i in rooms:
                    num = num_rooms[s, i]
                    self.demands[w][s, i] = num
                    prob *= self.pmf[s, i][num]
            self.probs[w] = prob

            # 找到可以動的那一位
            p = 0
            while(p < len(digit)):
                bit = digit[p]
                if num_rooms[bit] < self.capacity[bit]:
                    break
                p += 1
            
            # Stop condition: 每一位數（s, i）capacity 都滿了
            if p == len(digit):
                break

            # 可以動的那位加一，可動位數以下歸 0
            bit = digit[p]
            num_rooms[bit] += 1
            for i in range(0, p):
                bit = digit[i]
                num_rooms[bit] = 0
            w += 1
        
        self.W = w
        
    def create_demand(self):
        days = range(1, self.S + 1)
        rooms = range(1, self.I + 1)
        cap = 1000
        # Build adjusted pmf, cdf
        pmf = {n:nbinom.pmf(n, self.t, self.p) for n in range(0, cap)}
        trunc_pmf = {n:pmf[n] if pmf[n] >= self.prob_thd else 0 for n in pmf}
        sum_trunc_pmf = sum(trunc_pmf.values())
        norm_pmf = {n:trunc_pmf[n]/sum_trunc_pmf for n in trunc_pmf}
        # print("DEBUG: norm_pmf:", norm_pmf)
        adj_pmf = {}
        for s in days:
            for i in rooms:
                capacity = self.capacity[s, i]
                adj_pmf[s, i] = {n:norm_pmf[n] for n in range(0, capacity)}
                self.pmf[s, i] = adj_pmf[s, i].copy()
                self.pmf[s, i][capacity] = max(0, 1 - sum((adj_pmf[s, i].values())))
        print("DEBUG:", {(s, i):sum(self.pmf[s, i].values()) for s in days for i in rooms})
        # print("DEBUG: exp = {}".format(self.p * self.t / (1 - self.p)))

        # Sampling w scenarios
        for w in range(1, self.W + 1):
            self.demands[w] = {}
            for s in days:
                for i in rooms:
                    population = np.arange(0, self.capacity[s, i] + 1)
                    weights = list(self.pmf[s, i].values())
                    num = np.random.choice(population, p=weights)
                    self.demands[w][s, i] = num
    
    def store_total_demand(self, file):
        scenarios = range(1, self.W + 1)
        rooms = range(1, self.I + 1)
        days = range(1, self.S + 1)
        for w in scenarios:
            for s in days:
                demand = [str(self.demands[w][s, i]) for i in rooms]
                file.write(' '.join(demand) + '\n')
        
        prob = list(map(str, self.probs.values()))
        file.write(' '.join(prob))
    
    def store_set(self, file):
        file.write(' '.join([str(self.S), str(self.I), str(self.t), str(self.W)]) + '\n')

    # store_demand actually store pmf of demand
    def store_demand(self, file):
        days = range(1, self.S + 1)
        rooms = range(1, self.I + 1)
        for w in self.demands:
            for s in days:
                demands = [str(self.demands[w][s, i]) for i in rooms]
                file.write(' '.join(demands) + '\n')
                     
    def print_data(self):
        print("Set -------------------------:")
        print("S:{} | I:{} | C:{} | p:{} | t:{} | W:{}".format(self.S, self.I, self.C, self.p, self.t, self.W))
        print("\nParameters -------------------------:")
        print("p_U:{}".format(self.upgrade_fees))
        print("p:{}".format(self.prices))
        print("c:{}".format(self.capacity))
        print("order:{}".format(self.orders))
        print("pmf:{}".format(self.pmf))

class UpgradeModelDataSPSmart(UpgradeModelDataSP):
    def __init__(self, S, I, C, p, t, prob_thd=0.01, W=1000):
        super().__init__(S, I, C, p, t, prob_thd, W)
        # Add params:
        self.future_orders = {}
        self.order_prices = {}
        self.s_prices = {}
        self.days = range(1, self.S + 1)
        self.rooms = range(1, self.I + 1)
    
    def create(self):
        super().create()
        self.future_orders = {(s, i):random.randint(0, self.capacity[s, i])
                                for s in self.days for i in self.rooms}

        self.order_prices = {(s, i): random.uniform(0.7, 0.95) * self.prices[s, i]
                                for s in self.days for i in self.rooms}
        
        for w in range(1, self.W + 1):
            self.s_prices[w] = {}
            for s in self.days:
                for i in self.rooms:
                    total_demand = self.demands[w][s, i] + self.future_orders[s, i]
                    price = (self.prices[s, i] * self.demands[w][s, i] 
                             + self.order_prices[s, i] * self.future_orders[s, i]) \
                             / total_demand \
                             if total_demand > 0 else self.prices[s, i]
                    self.s_prices[w][s, i] = price
                    self.demands[w][s, i] = total_demand

    def store(self, path):
        with open(path, 'w+') as file:
            # write sets
            self.store_set(file)
            rooms = range(1, self.I + 1)
            days = range(1, self.S + 1)

            # write upgrade fee
            for i in rooms:
                fees = [str(self.upgrade_fees[i, j]) for j in self.upgrade_out[i]]
                if(len(fees) > 0):
                    file.write(' '.join(fees) + '\n')
            
            # write capacity
            for s in days:
                capacity = [str(self.capacity[s, i]) for i in rooms]
                file.write(' '.join(capacity) + '\n')
            
            # write orders
            orders = [str(self.orders[i]) for i in rooms]
            file.write(' '.join(list(map(str, orders))) + '\n')
            

            # write price
            for w in self.s_prices:
                for s in days:
                    prices = [str(self.s_prices[w][s, i]) for i in rooms]
                    file.write(' '.join(prices) + '\n')
            
            # write demands
            super().store_demand(file)
        
        file.close()

class MyopicModel:
    
    def __init__(self):
        # Sets
        self.days = {}
        self.rooms = {}
        self.period = 0
        self.upgrade_out = {}
        self.upgrade_in = {}

        # Parameters
        self.upgrade_fees = {}
        self.prices = {}    # prices of demand from individuals
        self.demands = {}   # demand from individuals
        self.capacity = {}
        self.orders = {}    # requested number of rooms from the travel agency

        # Decision Variables
        self.upgrade = {}
        self.sold = {}

        # objective value
        self.revenue = 0
        self.runtime = 0
    
    def run(self):
        try:      
            # Create a new model
            model = gp.Model("mip_myopic")
            model.Params.LogToConsole = 0

            # Create variables
            x = {}
            for i in self.rooms:
                for j in self.upgrade_out[i]:
                    x[i, j] = model.addVar(lb=0, vtype=gp.GRB.INTEGER, 
                                           name='x_%d_%d' % (i, j))
            
            z = {}
            for s in self.days:
                for i in self.rooms:
                    z[s, i] = model.addVar(lb=0, vtype=gp.GRB.INTEGER, 
                                           name='z_%d_%d' % (s, i))

            # Set objective
            model.setObjective((
                gp.quicksum(
                    gp.quicksum(x[i, j] * self.upgrade_fees[i, j] 
                        for j in self.upgrade_out[i])
                    for i in self.rooms
                    for s in self.days)
                + gp.quicksum(z[s, i] * self.prices[s, i]
                    for i in self.rooms 
                    for s in self.days)), gp.GRB.MAXIMIZE)          

            # Add constraint: Upgrade out
            model.addConstrs((
                gp.quicksum(x[i, j] for j in self.upgrade_out[i]) 
                <= self.orders[i] for i in self.rooms), "Upgrade out")

            # Add constraint: Demand capacity
            model.addConstrs((
                z[s, i] <= self.demands[s, i] 
                    for i in self.rooms 
                    for s in self.days), "Demand capacity")
            
            # Add constraint: Upgrade in
            model.addConstrs((
                gp.quicksum(x[k, i] for k in self.upgrade_in[i])
                - gp.quicksum(x[i, j] for j in self.upgrade_out[i]) + z[s, i] 
                <= self.capacity[s, i] - self.orders[i]
                    for i in self.rooms for s in self.days), "Upgrade in")
            
            # Optimize model
            model.optimize()
            # print(model.display())

            # for v in model.getVars():
            #     print('%s %g' % (v.varName, v.x))

            # print('Obj: %g' % model.objVal)

            # Store result
            for v in model.getVars():
                [name, v1, v2] = v.varName.split('_')
                if name == 'x':
                    upgrade_from = int(v1)
                    upgrade_to = int(v2)
                    self.upgrade[upgrade_from, upgrade_to] = v.x
                elif name == 'z':
                    day = int(v1)
                    room = int(v2)
                    self.sold[day, room] = v.x
            self.revenue = model.objVal

        except gp.GurobiError as e:
            print('Error code ' + str(e.errno) + ': ' + str(e))

        except AttributeError:
            print('Encountered an attribute error')
            exit()

        return self.revenue
    
    def read_data(self, path):
        with open(path, 'r') as file:
            # read S, I
            line = file.readline()
            scale = line.split(' ')
            S = int(scale[0])
            I = int(scale[1])
            self.period = int(scale[2])
            self.days = {s for s in range(1, S + 1)}
            self.rooms = {i for i in range(1, I + 1)}

            for i in self.rooms:
                self.upgrade_out[i] = set(range(i + 1, I + 1))
                self.upgrade_in[i] = set(range(1, i))

            # read upgrade fee
            for i in self.rooms:
                if self.upgrade_out[i]:
                    line = file.readline()
                    fees = list(map(int, line.split(' ')))
                    out = i + 1
                    for fee in fees:
                        self.upgrade_fees[i, out] = fee
                        out += 1
            
            # read prices
            for s in self.days:
                line = file.readline()
                prices = list(map(int, line.split(' ')))
                for i in self.rooms:
                    self.prices[s, i] = prices[i - 1]
            
            # read capacity:
            for s in self.days:
                line = file.readline()
                caps = list(map(int, line.split(' ')))
                for i in self.rooms:
                    self.capacity[s, i] = caps[i - 1]
            
            # read order
            line = file.readline()
            orders = list(map(int, line.split(' ')))
            self.orders = dict(zip(self.rooms, orders))
            
            # read demand
            for s in self.days:
                line = file.readline()
                demands = list(map(int, line.split(' ')))
                for i in self.rooms:
                    self.demands[s, i] = demands[i - 1]
                
    def print_data(self):
        print("Set -------------------------:")
        print("S:{}".format(self.days))
        print("I:{}".format(self.rooms))
        print("t:{}".format(self.period))
        print("I_out:{}".format(self.upgrade_out))
        print("I_in:{}".format(self.upgrade_in))
        print("\nParameters -------------------------:")
        print("p_U:{}".format(self.upgrade_fees))
        print("p:{}".format(self.prices))
        print("c:{}".format(self.capacity))
        print("D:{}".format(self.demands))
        print("order:{}".format(self.orders))

    def store_result(self, path_upgrade, path_sold):
        with open(path_upgrade, 'w+') as file:
            head = ','.join(['Upgrade_from\\to'] + list(map(str, self.rooms)))
            file.write(head + '\n')
            for i in self.rooms:
                upgrades = [str(int(self.upgrade[i, j])) \
                            if (i, j) in self.upgrade else '0' \
                            for j in self.rooms]
                file.write(','.join([str(i)] + upgrades) + '\n')
        file.close()

        with open(path_sold, 'w+') as file:
            head = ','.join(['Day\\room'] + list(map(str, self.rooms)))
            file.write(head + '\n')
            for s in self.days:
                sold = [str(int(self.sold[s, i])) for i in self.rooms]
                file.write(','.join([str(s)] + sold) + '\n')
        file.close()

# Myopic model in stochastic programming version
class MyopicModelSP(MyopicModel):
    
    def __init__(self, mode="IP"):
        super().__init__()
        # Add set: scenarios
        self.scenarios = {}
        # Add params: probability mass function
        self.pmf = {}
        # Add params: probabilities of scenarios
        self.probs = {}
        # Choose mode
        self.mode = mode
        if self.mode not in ["LP", "IP"]:
            print("Mode ERROR!!!")
            exit(1)

    def run(self):
        try:      
            # Create a new model
            model = gp.Model("mip_myopicSP")

            # Create variables
            x = {}
            for i in self.rooms:
                for j in self.upgrade_out[i]:
                    x[i, j] = model.addVar(lb=0, vtype=gp.GRB.INTEGER, 
                                           name='x_%d_%d' % (i, j))
            
            z = {}
            z_type = gp.GRB.INTEGER
            if self.mode == "LP":
                z_type = gp.GRB.CONTINUOUS
            
            print("Compute scenarios...")

            for w in self.scenarios:
                for s in self.days:
                    for i in self.rooms:
                        z[w, s, i] = model.addVar(lb=0, vtype=z_type, 
                                            name='z_%d_%d_%d' % (w, s, i))
            print("Gurobi start...")
            # Set objective
            model.setObjective(
                (gp.quicksum(
                    gp.quicksum(x[i, j] * self.upgrade_fees[i, j] 
                        for j in self.upgrade_out[i])
                    for i in self.rooms
                    for s in self.days)
                + gp.quicksum(
                    gp.quicksum(z[w, s, i] * self.prices[s, i]
                                  for i in self.rooms 
                                  for s in self.days)
                    for w in self.scenarios) / len(self.scenarios)
                ), gp.GRB.MAXIMIZE)    

            # Add constraint: Upgrade out
            model.addConstrs((
                gp.quicksum(x[i, j] for j in self.upgrade_out[i]) 
                <= self.orders[i] for i in self.rooms), "Upgrade out")

            # Add constraint: Demand capacity
            model.addConstrs((
                z[w, s, i] <= self.demands[w][s, i] 
                    for w in self.scenarios
                    for i in self.rooms 
                    for s in self.days), "Demand capacity")
            
            # Add constraint: Upgrade in
            model.addConstrs((
                gp.quicksum(x[k, i] for k in self.upgrade_in[i])
                    - gp.quicksum(x[i, j] for j in self.upgrade_out[i]) + z[w, s, i] 
                <= self.capacity[s, i] - self.orders[i]
                    for w in self.scenarios 
                    for i in self.rooms 
                    for s in self.days), "Upgrade in")
            
            # Optimize model
            model.optimize()
            # print(model.display())

            for v in model.getVars():
                result = v.varName.split('_')
                if result[0] == 'x':
                    print('%s %g' % (v.varName, v.x))

            print('Obj: %g' % model.objVal)

            # Store result
            for v in model.getVars():
                result = v.varName.split('_')
                if result[0] == 'x':
                    upgrade_from = int(result[1])
                    upgrade_to = int(result[2])
                    self.upgrade[upgrade_from, upgrade_to] = v.x

            self.revenue = model.objVal

        except gp.GurobiError as e:
            print('Error code ' + str(e.errno) + ': ' + str(e))

        except AttributeError:
            print('Encountered an attribute error')
            exit()

        return self.revenue

    def read_data(self, path):
        with open(path, 'r') as file:
            # read S, I
            line = file.readline()
            scale = line.split(' ')
            S = int(scale[0])
            I = int(scale[1])
            W = int(scale[3])
            self.period = int(scale[2])
            self.days = {s for s in range(1, S + 1)}
            self.rooms = {i for i in range(1, I + 1)}
            self.scenarios = {w for w in range(1, W + 1)}

            for i in self.rooms:
                self.upgrade_out[i] = set(range(i + 1, I + 1))
                self.upgrade_in[i] = set(range(1, i))

            # read upgrade fee
            for i in self.rooms:
                if self.upgrade_out[i]:
                    line = file.readline()
                    fees = list(map(int, line.split(' ')))
                    out = i + 1
                    for fee in fees:
                        self.upgrade_fees[i, out] = fee
                        out += 1
            
            # read prices
            for s in self.days:
                line = file.readline()
                prices = list(map(int, line.split(' ')))
                for i in self.rooms:
                    self.prices[s, i] = prices[i - 1]
            
            # read capacity:
            for s in self.days:
                line = file.readline()
                caps = list(map(int, line.split(' ')))
                for i in self.rooms:
                    self.capacity[s, i] = caps[i - 1]
            
            # read order
            line = file.readline()
            orders = list(map(int, line.split(' ')))
            self.orders = dict(zip(self.rooms, orders))
            
            # read demands
            for w in self.scenarios:
                self.demands[w] = {}
                for s in self.days:
                    line = file.readline()
                    room_num = list(map(float, line.split(' ')))
                    keys = list(self.rooms)
                    demand = dict(zip(keys, room_num))
                    for i in self.rooms:
                        self.demands[w][s, i] = demand[i]

    def compute_scenarios(self):
        num_rooms = {(s, i):0 for s in self.days for i in self.rooms}
        digit = [(s, i) for s in self.days for i in self.rooms]
        w = 1
        while(True):
            # Store scenario
            prob = 1
            self.demands[w] = {}
            for s in self.days:
                for i in self.rooms:
                    num = num_rooms[s, i]
                    self.demands[w][s, i] = num
                    prob *= self.pmf[s, i][num]
            self.probs[w] = prob

            # 找到可以動的那一位
            p = 0
            while(p < len(digit)):
                bit = digit[p]
                if num_rooms[bit] < self.capacity[bit]:
                    break
                p += 1
            
            # Stop condition: 每一位數（s, i）capacity 都滿了
            if p == len(digit):
                break

            # 可以動的那位加一，可動位數以下歸 0
            bit = digit[p]
            num_rooms[bit] += 1
            for i in range(0, p):
                bit = digit[i]
                num_rooms[bit] = 0
            w += 1
        
        self.W = w
        self.scenarios = set(range(1, w + 1))

    def print_data(self):
        print("Set -------------------------:")
        print("S:{}".format(self.days))
        print("I:{}".format(self.rooms))
        print("t:{}".format(self.period))
        print("I_out:{}".format(self.upgrade_out))
        print("I_in:{}".format(self.upgrade_in))
        print("\nParameters -------------------------:")
        print("p_U:{}".format(self.upgrade_fees))
        print("p:{}".format(self.prices))
        print("c:{}".format(self.capacity))
        print("order:{}".format(self.orders))
        print("demands:{}".format(self.demands))

    def store_result(self, path_upgrade):
        with open(path_upgrade, 'w+') as file:
            head = ','.join(['Upgrade_from\\to'] + list(map(str, self.rooms)))
            file.write(head + '\n')
            for i in self.rooms:
                upgrades = [str(int(self.upgrade[i, j])) \
                            if (i, j) in self.upgrade else '0' \
                            for j in self.rooms]
                file.write(','.join([str(i)] + upgrades) + '\n')
        file.close()

class MyopicModelSPSmart(MyopicModelSP):
    def __init__(self, mode="LP"):
        super().__init__(mode)
    
    def read_data(self, path):
        with open(path, 'r') as file:
            # read S, I
            line = file.readline()
            scale = line.split(' ')
            S = int(scale[0])
            I = int(scale[1])
            W = int(scale[3])
            self.period = int(scale[2])
            self.days = {s for s in range(1, S + 1)}
            self.rooms = {i for i in range(1, I + 1)}
            self.scenarios = {w for w in range(1, W + 1)}

            for i in self.rooms:
                self.upgrade_out[i] = set(range(i + 1, I + 1))
                self.upgrade_in[i] = set(range(1, i))

            # read upgrade fee
            for i in self.rooms:
                if self.upgrade_out[i]:
                    line = file.readline()
                    fees = list(map(int, line.split(' ')))
                    out = i + 1
                    for fee in fees:
                        self.upgrade_fees[i, out] = fee
                        out += 1
            
            # read capacity:
            for s in self.days:
                line = file.readline()
                caps = list(map(int, line.split(' ')))
                for i in self.rooms:
                    self.capacity[s, i] = caps[i - 1]
            
            # read order
            line = file.readline()
            orders = list(map(int, line.split(' ')))
            self.orders = dict(zip(self.rooms, orders))

            # read prices
            for w in self.scenarios:
                self.prices[w] = {}
                for s in self.days:
                    line = file.readline()
                    prices_line = list(map(float, line.split(' ')))
                    keys = list(self.rooms)
                    prices = dict(zip(keys, prices_line))
                    for i in self.rooms:
                        self.prices[w][s, i] = prices[i]
            
            # read demands
            for w in self.scenarios:
                self.demands[w] = {}
                for s in self.days:
                    line = file.readline()
                    room_num = list(map(float, line.split(' ')))
                    keys = list(self.rooms)
                    demand = dict(zip(keys, room_num))
                    for i in self.rooms:
                        self.demands[w][s, i] = demand[i]

    def run(self):
        try:      
            # Create a new model
            model = gp.Model("mip_myopicSP")

            # Create variables
            x = {}
            for i in self.rooms:
                for j in self.upgrade_out[i]:
                    x[i, j] = model.addVar(lb=0, vtype=gp.GRB.INTEGER, 
                                           name='x_%d_%d' % (i, j))
            
            z = {}
            z_type = gp.GRB.INTEGER
            if self.mode == "LP":
                z_type = gp.GRB.CONTINUOUS
            
            print("Compute scenarios...")

            for w in self.scenarios:
                for s in self.days:
                    for i in self.rooms:
                        z[w, s, i] = model.addVar(lb=0, vtype=z_type, 
                                            name='z_%d_%d_%d' % (w, s, i))
            print("Gurobi start...")
            # Set objective
            model.setObjective(
                (gp.quicksum(
                    gp.quicksum(x[i, j] * self.upgrade_fees[i, j] 
                        for j in self.upgrade_out[i])
                    for i in self.rooms
                    for s in self.days)
                + gp.quicksum(
                    gp.quicksum(z[w, s, i] * self.prices[w][s, i]
                                  for i in self.rooms 
                                  for s in self.days)
                    for w in self.scenarios) / len(self.scenarios)
                ), gp.GRB.MAXIMIZE)    

            # Add constraint: Upgrade out
            model.addConstrs((
                gp.quicksum(x[i, j] for j in self.upgrade_out[i]) 
                <= self.orders[i] for i in self.rooms), "Upgrade out")

            # Add constraint: Demand capacity
            model.addConstrs((
                z[w, s, i] <= self.demands[w][s, i] 
                    for w in self.scenarios
                    for i in self.rooms 
                    for s in self.days), "Demand capacity")
            
            # Add constraint: Upgrade in
            model.addConstrs((
                gp.quicksum(x[k, i] for k in self.upgrade_in[i])
                    - gp.quicksum(x[i, j] for j in self.upgrade_out[i]) + z[w, s, i] 
                <= self.capacity[s, i] - self.orders[i]
                    for w in self.scenarios 
                    for i in self.rooms 
                    for s in self.days), "Upgrade in")
            
            # Optimize model
            model.optimize()
            print(model.display())

            for v in model.getVars():
                result = v.varName.split('_')
                if result[0] == 'x':
                    print('%s %g' % (v.varName, v.x))

            print('Obj: %g' % model.objVal)

            # Store result
            for v in model.getVars():
                result = v.varName.split('_')
                if result[0] == 'x':
                    upgrade_from = int(result[1])
                    upgrade_to = int(result[2])
                    self.upgrade[upgrade_from, upgrade_to] = v.x

            self.revenue = model.objVal

        except gp.GurobiError as e:
            print('Error code ' + str(e.errno) + ': ' + str(e))

        except AttributeError:
            print('Encountered an attribute error')
            exit()

        return self.revenue    

def create_data():
    file_num = 20
    folder_prefix = "input/MDexperiment/"

    room_types = [2, 3, 4, 5]
    max_days = [2, 7, 14]
    basic_caps = [5, 25, 100]
    p_NBdist = [0.25, 0.5, 0.75]
    periods = [2, 5, 10, 15]
    
    scenarios = [(day, room, cap, p, t) 
                    for day in max_days 
                    for room in room_types
                    for cap in basic_caps 
                    for p in p_NBdist
                    for t in periods]

    for day, room, cap, p, t in scenarios:
        folder_name = folder_prefix \
                      + "S{}I{}C{}p{}t{}/".format(day, room, cap, int(100*p), t)
        if not os.path.exists(folder_name):
            os.mkdir(folder_name)
        
        data = UpgradeModelData(day, room, cap, p, t)
        for i in range(1, file_num + 1):
            file_name = folder_name + "{}.txt".format(i)
            data.create()
            data.store(file_name)

def create_data_MS():
    file_num = 1
    folder_prefix = "input/MSexperiment/"
    if not os.path.exists(folder_prefix):
        os.mkdir(folder_prefix)

    room_types = [2, 3, 4, 5]
    max_days = [2, 7, 14]
    basic_caps = [5, 25, 100]
    exps = [0.25, 0.5, 0.75]
    periods = [2, 5, 10, 15]
    # room_types = [2, 3, ]
    # max_days = [5]
    # basic_caps = [10]
    # p_NBdist = [0.25]
    # periods = [2]
    
    scenarios = [(day, room, cap, p, t) 
                    for day in max_days 
                    for room in room_types
                    for cap in basic_caps 
                    for p in exps
                    for t in periods]

    for day, room, cap, p, t in scenarios:
        suffix = "S{}I{}C{}p{}t{}/".format(day, room, cap, int(100*p), t)
        folder = folder_prefix + suffix
        if not os.path.exists(folder):
            os.mkdir(folder)

        # exp = p * cap * 1.5
        # prob = 1 - exp/(exp + t)
        # print("DEBUG: p:{} | exp:{} | prob:{} | expVal:{}".format(p, exp, prob, t*prob/(1-prob)))
        prob = p
        data = UpgradeModelDataSP(day, room, cap, prob, t)
        for i in range(1, file_num + 1): 
            name = "{}.txt".format(i)
            file_name = folder + name
            print("Create file: {}".format(suffix + name))
            data.create()
            data.store(file_name)

def create_data_MS_smart():
    file_num = 20
    folder_prefix = "input/MSexperiment_smart/"
    if not os.path.exists(folder_prefix):
        os.mkdir(folder_prefix)

    room_types = [2, 3, 4, 5]
    max_days = [2, 7, 14]
    basic_caps = [5, 25, 100]
    exps = [0.25, 0.5, 0.75]
    periods = [2, 5, 10, 15]
    # room_types = [2]
    # max_days = [2]
    # basic_caps = [5]
    # exps = [0.5]
    # periods = [2]
    
    scenarios = [(day, room, cap, p, t) 
                    for day in max_days 
                    for room in room_types
                    for cap in basic_caps 
                    for p in exps
                    for t in periods]

    for day, room, cap, p, t in scenarios:
        suffix = "S{}I{}C{}p{}t{}/".format(day, room, cap, int(100*p), t)
        folder = folder_prefix + suffix
        if not os.path.exists(folder):
            os.mkdir(folder)

        # exp = p * cap * 1.5
        # prob = 1 - exp/(exp + t)
        # print("DEBUG: p:{} | exp:{} | prob:{} | expVal:{}".format(p, exp, prob, t*prob/(1-prob)))
        prob = p
        data = UpgradeModelDataSPSmart(day, room, cap, prob, t)
        for i in range(1, file_num + 1): 
            name = "{}.txt".format(i)
            file_name = folder + name
            print("Create file: {}".format(suffix + name))
            data.create()
            data.store(file_name)

def run_exper_MD():
    # Set experiment environment
    file_num = 20
    folder_prefix = "input/MDexperiment/"

    room_types = [2, 3, 4, 5]
    max_days = [2, 7, 14]
    basic_caps = [5, 25, 100]
    p_NBdist = [25, 50, 75]
    periods = [2, 5, 10, 15]
    
    scenarios = list(sorted([(day, room, cap, p, t) 
                    for day in max_days 
                    for room in room_types
                    for cap in basic_caps 
                    for p in p_NBdist
                    for t in periods]))
    # print(scenarios)
    
    # Open output file
    folder_out = "result/MDexperiment/gurobi/"
    if not os.path.exists(folder_out):
        os.mkdir(folder_out)
    rev_out = folder_out + "revenue.csv"
    with open(rev_out, 'w+') as file:
        msg = ','.join(['Num_day', 'Num_room_type', 'Min_capacity', 
                        'prob_NB', 'period', 'file', 'revenue', 'time'])
        file.write(msg + '\n')
    file.close()
    
    # Run experiments
    revenues = {}
    times = {}
    for day, room, cap, p_NB, t in scenarios:
        folder_prefix = "input/MDexperiment/"
        folder_name = "S{}I{}C{}p{}t{}/".format(day, room, cap, p_NB, t)
        folder = folder_prefix + folder_name
        for i in range(1, file_num + 1):
            print("file: S{}I{}C{}p{}t{}i{} ------------------------------".format(day, room, cap, p_NB, t, i))
            file_path = folder + "{}.txt".format(i)
            myopic = MyopicModel()
            myopic.read_data(file_path)
            start = time.time()
            revenue = myopic.run()
            end = time.time()
            revenues[day, room, cap, p_NB, t, i] = revenue
            times[day, room, cap, p_NB, t, i] = end - start
            # Store results
            decision_folder = folder_out + folder_name
            if not os.path.exists(decision_folder):
                os.mkdir(decision_folder)
            path_upgrade = decision_folder + "upgrade_{}.csv".format(i)
            path_sold = decision_folder + "sold_{}.csv".format(i)
            myopic.store_result(path_upgrade, path_sold)
    
    # Store revenues
    revenues_key = list(sorted(revenues))
    with open(rev_out, 'a') as file:
        for day, room, cap, p_NB, t, file_id in revenues_key:
            revenue = revenues[day, room, cap, p_NB, t, file_id]
            duration = times[day, room, cap, p_NB, t, file_id]
            msg = ','.join(list(map(str, [day, room, cap, p_NB, t,
                                         file_id, int(revenue), duration])))
            file.write(msg + '\n')
    file.close()

def run_exper_MS(room_types, max_days, basic_caps, exps, periods, file_num, model_name, mode="naive"):
    # Set experiment environment
    folder_prefix = "input/" + model_name + "/"    
    scenarios = [(day, room, cap, p, t) 
                    for day in max_days 
                    for room in room_types
                    for cap in basic_caps 
                    for p in exps
                    for t in periods]
    # print(scenarios)
    
    # Open output file
    out_prefix = "result/" + model_name + "/"
    if not os.path.exists(out_prefix):
        os.mkdir(out_prefix)
    folder_out = out_prefix + "gurobi/"
    if not os.path.exists(folder_out):
        os.mkdir(folder_out)
    rev_out = folder_out + "revenue.csv"
    with open(rev_out, 'w+') as file:
        msg = ','.join(['Num_day', 'Num_room_type', 'Min_capacity', 
                        'prob_NB', 'period', 'file', 'revenue', 'time'])
        file.write(msg + '\n')
    file.close()

    # Run experiments
    scenarios_key = list(sorted(scenarios))
    with open(rev_out, 'a') as file:
        for day, room, cap, p_NB, t in scenarios_key:
            folder_prefix = "input/" + model_name + "/"
            folder_name = "S{}I{}C{}p{}t{}/".format(day, room, cap, int(100*p_NB), t)
            folder = folder_prefix + folder_name
            for i in range(1, file_num + 1):
                print("file: S{}I{}C{}p{}t{}i{} ------------------------------".format(day, room, cap, int(100*p_NB), t, i))
                file_path = folder + "{}.txt".format(i)
                if mode == "naive":
                    ms_model = MyopicModelSP(mode="LP")
                else:
                    ms_model = MyopicModelSPSmart(mode="LP")
                ms_model.read_data(file_path)
                start = time.time()
                revenue = ms_model.run()
                end = time.time()
                duration = end - start
                print("rev: {} | time: {}".format(revenue, duration))
                # Store results
                msg = ','.join(list(map(str, [day, room, cap, p_NB, t,
                                         i, revenue, duration])))
                file.write(msg + '\n')

                decision_folder = folder_out + folder_name
                if not os.path.exists(decision_folder):
                    os.mkdir(decision_folder)
                path_upgrade = decision_folder + "upgrade_{}.csv".format(i)
                ms_model.store_result(path_upgrade)

    file.close()

def run_exper():
    file_num = 20
    model_name = "MSexperiment_smart"
    mode = "smart"
    room_types = [2, 3, 4, 5]
    max_days = [2, 7, 14]
    basic_caps = [5, 25, 100]
    exps = [0.25, 0.5, 0.75]
    periods = [2, 5, 10, 15]
    # room_types = [2]
    # max_days = [2]
    # basic_caps = [5]
    # exps = [0.5]
    # periods = [2]
    run_exper_MS(room_types, max_days, basic_caps, exps, periods, file_num, model_name, mode)

def create_full_data():
    files = list(range(1, 2))
    param_prefix = "input/params/"
    prob_prefix = "input/prob/"

    K, T = 10, 10
    # days = [2, 5, 10]
    # rooms = [2, 3, 5]
    # caps = [10, 50, 100]
    days = [5]
    rooms = [3]
    caps = [25]
    
    # ms = [0.5, 1, 1.5, 2]
    # bs = [0.5, 0.7, 0.9]
    # rs = [0.1, 0.2, 0.3]
    ds = [0.1, 0.25, 0.4]
    # days = [2, 3, 4, 5]
    # rooms = [2, 3, 4, 5]
    # caps = [2]
    ms = [1]
    bs = [0.7]
    rs = [0.2]
    # ds = [0.25]

    scenarios = [(s, i, c, m, b, r, d) 
                    for s in days
                    for i in rooms
                    for c in caps
                    for m in ms
                    for b in bs
                    for r in rs
                    for d in ds]

    for s, i, c, m, b, r, d in scenarios:
        param_folder = param_prefix \
                      + "S{}I{}C{}m{}b{}r{}d{}K{}T{}_poisson/".format(s, i, c, int(10*m), 
                                                        int(100*b), int(10*r), int(100*d), K, T)
        prob_folder = prob_prefix \
                      + "S{}I{}C{}m{}b{}r{}d{}K{}T{}_poisson/".format(s, i, c, int(10*m), 
                                                        int(100*b), int(10*r), int(100*d), K, T)
        if not os.path.exists(param_folder):
            os.mkdir(param_folder)
        if not os.path.exists(prob_folder):
            os.mkdir(prob_folder)
        
        for f in files:
            file_name = "{}.txt".format(f)
            
            param_file = param_folder + file_name
            print("Create file: ", param_file)
            data = Data(s, i, c, K, T, m, b, r, d)
            data.create()
            data.store_params(param_file)

            prob_file = prob_folder + "T" + str(T) + "_" + file_name
            data.store_prob(prob_file, T)
            # for t in range(1, data.T + 1):
            #     prob_file = prob_folder + "T" + str(t) + "_" + file_name
            #     data.store_prob(prob_file, t)

def test():
    data = Data(5, 3, 25, 10, 10, 1, 0.7, 0.2, 0.25)
    data.create()

if __name__ == '__main__':
    create_full_data()
    # test()