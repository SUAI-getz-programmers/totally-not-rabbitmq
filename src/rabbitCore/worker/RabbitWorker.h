#ifndef RABBIT_RabbitWorker_H
#define RABBIT_RabbitWorker_H

#include <boost/asio.hpp>
#include "protocol/Connection.h"
#include "client/STIPClient.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>

using boost::asio::ip::udp;
using json = nlohmann::json;

class RabbitWorker;

typedef void (RabbitWorker::*func_type)(const std::string &, json, int);

typedef std::map<std::string, func_type> func_map_type;

class RabbitWorker {
public:
    RabbitWorker(std::string id, std::string host, int port, int cores);

    void init();

    void startPolling();

    ~RabbitWorker() {
        delete server_socket;
    }

private:
    STIP::STIPClient *client;

    int port;
    int cores;
    std::string id;
    std::string host;

    udp::resolver *resolver;
    udp::endpoint *server_endpoint;
    udp::socket *server_socket{};

    STIP::Connection *connection{};
    boost::asio::io_context io_context;

    // functions

    int simpleMath(int a, int b);

    int determinant(std::vector<std::vector<int>> matrix, int n);

    void
    matrixMultiplication(const std::vector<std::vector<int>> &matrixA, const std::vector<std::vector<int>> &matrixB,
                         std::vector<std::vector<int>> &resultMatrix, int row, int col);

    static void doWait(int seconds);

    void simpleMathHandler(const std::string &request_id, json data, int taskCores);

    void determinantHandler(const std::string &id, json data, int taskCores);

    void matrixMultiplicationHandler(const std::string &id, json data, int taskCores);

    func_map_type mapping;

    void handleTaskRequest(const std::string &request_id, const std::string &func, const json &data, int taskCores);
};


#endif //RABBIT_RabbitWorker_H
