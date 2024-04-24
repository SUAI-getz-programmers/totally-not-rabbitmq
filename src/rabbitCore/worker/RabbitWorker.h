//
// Created by Serge on 23.03.2024.
//

#ifndef RABBIT_RabbitWorker_H
#define RABBIT_RabbitWorker_H

#include <boost/asio.hpp>
#include "protocol/Connection.h"
#include <nlohmann/json.hpp>
#include <adoint_backcompat.h>
#include <vector>
#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>

using boost::asio::ip::udp;
using json = nlohmann::json;

class RabbitWorker;

typedef void (RabbitWorker::*func_type)(int, json, int);

typedef std::map<std::string, func_type> func_map_type;

class RabbitWorker {
public:
    RabbitWorker(std::string host, int port, int cores);

    void init();

    void startPolling();

    ~RabbitWorker() {
        delete server_socket;
    }

private:
    std::string host;
    int port;
    int cores;
    STIP::Connection *connection{};

    boost::asio::io_context io_context;
    udp::socket *server_socket{};

    // functions (demo for now)

    int simpleMath(int a, int b);

    int determinant(std::vector<std::vector<int>> matrix, int n);

    static void doWait(int seconds);

    void simpleMathHandler(int id, json data, int taskCores);

    void determinantHandler(int id, json data, int taskCores);

    func_map_type mapping;

};


#endif //RABBIT_RabbitWorker_H
