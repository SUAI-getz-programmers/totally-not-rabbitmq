//
// Created by Serge on 23.03.2024.
//

#ifndef RABBIT_RabbitWorker_H
#define RABBIT_RabbitWorker_H

#include <boost/asio.hpp>
#include "protocol/Connection.h"
#include <nlohmann/json.hpp>

using boost::asio::ip::udp;
using json = nlohmann::json;

class RabbitWorker {
public:
    RabbitWorker(std::string host, int port, int cores);

    void init();

    // - нужно?
    //   void startPolling();
    //   void processConnection(STIP::Connection *connection);
    //


    ~RabbitWorker() {
        delete server_socket;
    }

private:
    std::string host;
    int port;
    int cores;

    boost::asio::io_context io_context;
    udp::socket *server_socket{};

    // json parseMessage(std::string message);
    static bool validateRequest(json request);


    // functions (demo for now)

    static void doWait(int seconds);
    int doSimpleMath(int a, int b);

};


#endif //RABBIT_RabbitWorker_H