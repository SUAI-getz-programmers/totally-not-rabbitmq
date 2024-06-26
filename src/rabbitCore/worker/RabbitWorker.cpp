#include "RabbitWorker.h"
#include <utility>
#include "protocol/STIP.h"
#include "protocol/Connection.h"
#include "client/STIPClient.h"
#include "DataModel/TaskResult.h"
#include "DataModel/Worker.h"
#include "DataModel/Message.h"
#include "TaskRequest.h"

using boost::asio::ip::udp;

RabbitWorker::RabbitWorker(std::string id, std::string host, int port, int cores) {
    std::cout << "RabbitWorker::RabbitWorker - Initializing with id: " << id << ", host: " << host << ", port: " << port
              << ", cores: " << cores << std::endl;

    this->id = std::move(id);
    this->host = std::move(host);
    this->port = port;
    this->cores = cores;
}

void RabbitWorker::init() {
    std::cout << "RabbitWorker::init - Initialization started" << std::endl;

    resolver = new udp::resolver(io_context);
    auto endpoints = resolver->resolve(udp::v4(), host, std::to_string(port));

    server_endpoint = new udp::endpoint(*endpoints.begin());
    server_socket = new udp::socket(io_context);
    server_socket->open(udp::v4());

    udp::socket::receive_buffer_size bigbufsize(INT_MAX);
    server_socket->set_option(bigbufsize);

    client = new STIP::STIPClient(*server_socket);
    client->startListen();

    connection = client->connect(*server_endpoint);

    mapping["simpleMath"] = &RabbitWorker::simpleMathHandler;
    mapping["determinant"] = &RabbitWorker::determinantHandler;
    mapping["matrixMultiplication"] = &RabbitWorker::matrixMultiplicationHandler;

    // Register worker
    if (connection) {
        std::cout << "RabbitWorker::init - Connection established" << std::endl;

        Worker worker = {id, cores, 0, nullptr};
        nlohmann::json workerJson;
        to_json(workerJson, worker);
        Message message = {
                MessageType::RegisterWorker,
                workerJson.dump()
        };

        nlohmann::json messageJson;
        to_json(messageJson, message);

        std::string msg = messageJson.dump();
        connection->sendMessage(msg);
        std::cout << "RabbitWorker::init - Worker registered with server" << std::endl;
    } else {
        std::cerr << "RabbitWorker::init - Error: Failed to connect to server." << std::endl;
    }
}

void RabbitWorker::startPolling() {
    std::cout << "RabbitWorker::startPolling - Polling started" << std::endl;
    for (;;) {
        STIP::ReceiveMessageSession *received = connection->receiveMessage();

#ifdef SERVER_ARCH_DEBUG
        std::cout << "RabbitWorker::startPolling - Received message: " << received->getDataAsString() << std::endl;
#endif

        json request = json::parse(received->getDataAsString());
        Message message = request.get<Message>();
        json messageData = json::parse(message.data);

        switch (message.action) {
            case MessageType::TaskRequest: {
                std::cout << "RabbitWorker::startPolling - Received task request" << std::endl;
                struct TaskRequest task = messageData.get<struct TaskRequest>();
                json data = json::parse(task.data);

                // Call a separate function to handle the task asynchronously
                handleTaskRequest(task.id, task.func, data, task.cores);
                break;
            }

            default:
                std::cout << "RabbitWorker::startPolling - Unknown message type" << std::endl;
                break;
        }
    }
}

void RabbitWorker::handleTaskRequest(const std::string &request_id, const std::string &func, const json &data,
                                     int taskCores) {
    if (mapping.find(func) == mapping.end()) {
        std::cout << "RabbitWorker::handleTaskRequest - Function not found: " << func << std::endl;
        return;
    }

    // Create a new thread for each task request
    std::thread taskThread([this, request_id, func, data, taskCores]() {
        (this->*mapping[func])(request_id, data, taskCores);
    });

    taskThread.detach();
}

// worker function implementations

void RabbitWorker::doWait(int seconds) {
    std::cout << "RabbitWorker::doWait - Waiting for " << seconds << " seconds" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

int RabbitWorker::simpleMath(int a, int b) {
    std::cout << "RabbitWorker::simpleMath - Adding " << a << " + " << b << std::endl;
    int c = a + b;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "RabbitWorker::simpleMath - Result is " << c << std::endl;
    return c;
}

int RabbitWorker::determinant(std::vector<std::vector<int>> matrix, int n) {
    std::cout << "RabbitWorker::determinant - Calculating determinant for matrix of size " << n << std::endl;
    int det = 1;

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            while (matrix[j][i] != 0) {
                int ratio = matrix[i][i] / matrix[j][i];
                for (int k = i; k < n; k++) {
                    matrix[i][k] -= ratio * matrix[j][k];
                }
                std::swap(matrix[i], matrix[j]);
                det *= -1;
            }
        }
        det *= matrix[i][i];
    }

    std::cout << "RabbitWorker::determinant - Determinant result is " << det << std::endl;
    return det;
}

void RabbitWorker::matrixMultiplication(const std::vector<std::vector<int>> &matrixA,
                                        const std::vector<std::vector<int>> &matrixB,
                                        std::vector<std::vector<int>> &resultMatrix, int row, int col) {
    int colsA = matrixA[0].size();
    for (int k = 0; k < colsA; ++k) {
        resultMatrix[row][col] += matrixA[row][k] * matrixB[k][col];
    }
}

void RabbitWorker::simpleMathHandler(const std::string &request_id, json data, int taskCores) {
    std::cout << "RabbitWorker::simpleMathHandler - Handling simpleMath for request_id: " << request_id << std::endl;
    int a, b, result;
    try {
        a = data["a"];
        b = data["b"];
        result = simpleMath(a, b);
    } catch (json::exception &e) {
        std::cerr << "RabbitWorker::simpleMathHandler - Error parsing data: " << e.what() << std::endl;
        return;
    }

    struct TaskResult taskResult = {
            request_id,
            json({{"result", result}}).dump(),
            1
    };

    Message message = {
            MessageType::TaskResult,
            json(taskResult).dump()
    };

    connection->sendMessage(json(message).dump());
    std::cout << "RabbitWorker::simpleMathHandler - Result sent for request_id: " << request_id << std::endl;
}

void RabbitWorker::determinantHandler(const std::string &request_id, json data, int taskCores) {
    std::cout << "RabbitWorker::determinantHandler - Handling determinant for request_id: " << id << std::endl;
    std::vector<std::thread> threads;
    threads.reserve(taskCores);

    std::vector<std::vector<std::vector<int>>> matrices = data.get<std::vector<std::vector<std::vector<int>>>>();
    std::vector<int> results(matrices.size());

    for (int i = 0; i < matrices.size(); ++i) {
        threads.emplace_back([this, &matrices, &results, i] {
            std::cout << "RabbitWorker::determinantHandler - Starting thread for matrix " << i << std::endl;
            results[i] = this->determinant(matrices[i], matrices[i].size());
        });

        if (threads.size() == taskCores || i == matrices.size() - 1) {
            for (auto &t: threads) t.join();
            threads.clear();
        }
    }

    struct TaskResult taskResult = {
            request_id,
            json(results).dump(),
            1
    };

    Message message = {
            MessageType::TaskResult,
            json(taskResult).dump()
    };

    connection->sendMessage(json(message).dump());
    std::cout << "RabbitWorker::determinantHandler - Results sent for request_id: " << request_id << std::endl;
}

void RabbitWorker::matrixMultiplicationHandler(const std::string &request_id, json data, int taskCores) {
    std::cout << "RabbitWorker::matrixMultiplicationHandler - Handling matrix multiplication for request_id: "
              << request_id << std::endl;
    std::vector<std::thread> threads;
    threads.reserve(taskCores);

    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::vector<std::vector<int>>> matrices = data.get<std::vector<std::vector<std::vector<int>>>>();
    std::vector<std::vector<int>> matrixA = matrices[0];
    std::vector<std::vector<int>> matrixB = matrices[1];

    if (matrixA[0].size() != matrixB.size()) {
        std::cerr << "RabbitWorker::matrixMultiplicationHandler - Matrix dimensions do not match for multiplication"
                  << std::endl;
        return;
    }

    int rowsA = matrixA.size();
    int colsB = matrixB[0].size();
    std::vector<std::vector<int>> resultMatrix(rowsA, std::vector<int>(colsB, 0));

    for (int row = 0; row < rowsA; ++row) {
        for (int col = 0; col < colsB; ++col) {
            threads.emplace_back(&RabbitWorker::matrixMultiplication, this, std::ref(matrixA), std::ref(matrixB),
                                 std::ref(resultMatrix), row, col);

            if (threads.size() == taskCores || (row == rowsA - 1 && col == colsB - 1)) {
                for (auto &t: threads) t.join();
                threads.clear();
            }
        }
    }

    struct TaskResult taskResult = {
            request_id,
            json(resultMatrix).dump(),
            1
    };

    Message message = {
            MessageType::TaskResult,
            json(taskResult).dump()
    };

    connection->sendMessage(json(message).dump());
    std::cout << "RabbitWorker::matrixMultiplicationHandler - Results sent for request_id: " << request_id << std::endl;

    // считаем время выполнения
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "RabbitWorker::matrixMultiplicationHandler - Execution time: " << elapsed.count() << "s" << std::endl;
}


