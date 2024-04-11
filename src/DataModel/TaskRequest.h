//
// Created by Potato on 11.04.24.
//

#ifndef RABBIT_TASKREQUEST_H
#define RABBIT_TASKREQUEST_H

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct TaskRequest {
    int func;
    std::string data;
    int cores;
};

void to_json(json &j, const TaskRequest &tr) {
    j = json{{"func",  tr.func},
             {"data",  tr.data},
             {"cores", tr.cores}};
}

void from_json(const json &j, TaskRequest &tr) {
    j.at("func").get_to(tr.func);
    j.at("data").get_to(tr.data);
    j.at("cores").get_to(tr.cores);
}

#endif //RABBIT_TASKREQUEST_H