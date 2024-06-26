#include "RabbitClient.h"

#include <utility>
#include "DataModel/TaskResult.h"
#include "protocol/STIP.h"
#include "client/STIPClient.h"
#include "protocol/Connection.h"
#include "DataModel/Client.h"
#include "Message.h"
#include <algorithm>

using namespace STIP;
using boost::asio::ip::udp;

#define PRETTY_MAX_LINES_THR 35

RabbitClient::RabbitClient(std::string id, std::string host, int port) {
    this->id = std::move(id);
    this->host = std::move(host);
    this->port = port;
}

void RabbitClient::init() {
    resolver = new udp::resolver(io_context);
    auto endpoints = resolver->resolve(udp::v4(), host, std::to_string(port));
    server_endpoint = new udp::endpoint(*endpoints.begin());

    server_socket = new udp::socket(io_context);
    server_socket->open(udp::v4());

    client = new STIP::STIPClient(*server_socket);
    client->startListen();

    connection = client->connect(*server_endpoint);

    // Register client
    if (connection) {
        // Create Client object to send
        Client clientInfo = {id, connection};
        nlohmann::json clientJson;
        to_json(clientJson, clientInfo);
        Message message = {
                MessageType::RegisterClient,
                clientJson.dump()
        };

        nlohmann::json messageJson;
        to_json(messageJson, message);
        std::string msg = messageJson.dump();
        connection->sendMessage(msg);
    } else {
        std::cerr << "Error: Failed to connect to server." << std::endl;
    }
}

void RabbitClient::receiveResutls() {
    for (;;) {
        STIP::ReceiveMessageSession *received = connection->receiveMessage();
        auto message = json::parse(received->getDataAsString()).template get<struct Message>();
        auto item = json::parse(message.data);
        struct TaskResult result;

        try {
            result = item.template get<struct TaskResult>();
        } catch (json::exception &e) {
            std::cerr << "Error parsing task: " << e.what() << std::endl;
            continue;
        }

        json data = json::parse(result.data);
        std::string pretty = data.dump(2);
        int count = std::count(pretty.begin(), pretty.end(), '\n');

        if (count > PRETTY_MAX_LINES_THR) {
            pretty = data.dump();
        }

        std::cout << pretty << std::endl;
    }
}

void RabbitClient::sendTask(struct TaskRequest t) {
    json task = t;
    Message m = {
            MessageType::TaskRequest,
            task.dump()
    };
    json message = m;
    connection->sendMessage(message.dump());
}
