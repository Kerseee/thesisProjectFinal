#include <stdio.h>
#include <map>
#include <string>
#include <ostream>
#include <vector>
#include "data.hpp"
#include "caseData.hpp"
#include "planner.hpp"
#include "caseGenerator.hpp"

namespace planner{

Order::Order() {
    this->is_order = false;
    this->price = 0;
}

Order::Order(const Order& order){
    this->operator=(order);
}

Order& Order::operator=(const Order& order){
    this->is_order = order.is_order;
    this->price = order.price;
    this->request_days = order.request_days;
    this->request_rooms = order.request_rooms;
    this->upgrade_price = order.upgrade_price;
    return *this;
}

}

std::ostream& operator<<(std::ostream& os, const planner::Order& order){
    os << "is_order: " << order.is_order << "\n"
       << "price: " << order.price << "\n"
       << "request_days: " << order.request_days << "\n"
       << "request_rooms: " << order.request_rooms << "\n"
       << "upgrade_price: " << order.upgrade_price << "\n";
    return os;
}