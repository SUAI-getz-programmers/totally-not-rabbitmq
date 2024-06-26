#include "protocol/STIP.h"
#include "server/STIPServer.h"
#include "protocol/Connection.h"
#include "DataModel/Message.h"
#include "RabbitServer.h"
#include "TaskRequest.h"
#include "TaskResult.h"
#include <queue>
#include <iomanip> // For std::put_time

using namespace STIP;
using boost::asio::ip::udp;

RabbitServer::RabbitServer(int port) {
    this->port = port;
    taskService = TaskService();
}

void RabbitServer::init() {
    server_socket = new udp::socket(io_context, udp::endpoint(udp::v4(), port));
    udp::socket::receive_buffer_size bigbufsize(INT_MAX);
    server_socket->set_option(bigbufsize);
}

void RabbitServer::startPolling() {
    STIPServer server(*server_socket);
    std::cout << "Server started on port " << port << std::endl;

    std::vector<std::thread> threadsProcessors;
    for (;;) {
        Connection *connection = server.acceptConnection();
        if (connection == nullptr) break;

        std::cout << "Connection accepted" << std::endl;
        threadsProcessors.emplace_back(&RabbitServer::processConnection, this, connection);
    }

    std::cout << "Server thread finished" << std::endl;

    // hard stopping
    server_socket->close();
    server_socket->shutdown(udp::socket::shutdown_both);

    // exit program
    exit(0);
}

void RabbitServer::processConnection(STIP::Connection *connection) {
    auto receiveMessage = connection->receiveMessage();
    json request = json::parse(receiveMessage->getDataAsString());

#ifdef SERVER_ARCH_DEBUG
    std::cout << "Received message: " << request.dump() << std::endl;
#endif

    Message message;
    json data;

    try {
        message = request.template get<Message>();
        data = json::parse(message.data);
    } catch (json::exception &e) {
        std::cerr << "Error parsing message: " << e.what() << std::endl;
        return;
    }

    switch (message.action) {
        case MessageType::RegisterClient: {
            std::cout << logTime() << "Received Client registration request\n";
            Client client;

            try {
                client = data.template get<Client>();
                client.connection = connection;
                std::cout << logTime() << "Client registered: " << client.id << std::endl;
            } catch (json::exception &e) {
                std::cerr << logTime() << "Error parsing client: " << e.what() << std::endl;
                break;
            }

            userDBService.addClient(client);

            try {
                processClient(client);
            } catch (std::exception &e) {
                std::cerr << logTime() << "Error processing client: " << e.what() << std::endl;
            }

            userDBService.removeClient(client);
            std::cout << logTime() << "Client disconnected: " << client.id << std::endl;
            break;
        }

        case MessageType::RegisterWorker: {
            std::cout << logTime() << "Received Worker registration request\n";
            Worker worker;

            try {
                worker = data.template get<Worker>();
                worker.connection = connection;
                std::cout << logTime() << "Worker registered: " << worker.id << " (Cores: " << worker.cores << ")\n";
            } catch (json::exception &e) {
                std::cerr << logTime() << "Error parsing worker: " << e.what() << std::endl;
                break;
            }

            userDBService.addWorker(worker);

            try {
                processWorker(worker);
            } catch (std::exception &e) {
                std::cerr << "Error processing worker: " << e.what() << std::endl;
            }

            userDBService.removeWorker(worker);
            std::cout << logTime() << "Worker disconnected: " << worker.id << std::endl;
            break;
        }

        default:
            std::cerr << logTime() << "Unknown action: " << message.action << std::endl;
            break;
    }

    delete connection;
    delete receiveMessage;
    delete connection;
}

void RabbitServer::processWorker(Worker &worker) {
    checkTaskQueue(worker);

    for (;;) {
        auto receiveMessage = worker.connection->receiveMessage();
        json request = json::parse(receiveMessage->getDataAsString()); // TODO change parse

#ifdef SERVER_ARCH_DEBUG
        std::cout << "Received message: " << request.dump() << std::endl;
#endif

        Message message;
        json data;

        try {
            message = request.template get<Message>();
            data = json::parse(message.data);
        } catch (json::exception &e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
            continue;
        }

        switch (message.action) {
            case MessageType::TaskResult: {
                std::cout << logTime() << "Received TaskResult from worker: " << worker.id << std::endl;
                struct TaskResult result;
                try {
                    result = data.template get<struct TaskResult>();
                } catch (json::exception &e) {
                    std::cerr << logTime() << "Error parsing task: " << e.what() << std::endl;
                    continue;
                }

                Task task = taskService.findTaskByID(result.id);

#ifdef SERVER_ARCH_DEBUG
                std::cout << json(task).dump() << std::endl;
#endif

                task.status = TaskStatus::Ready;
                std::cout << logTime() << "Task marked as Ready: " << result.id << std::endl;
                task.output = result.data;

                taskService.updateTask(task);
                userDBService.modifyWorkerUsedCores(task.worker_hash_id, task.cores, false);

                Client client = userDBService.findClientByID(task.client_hash_id);
                Message result_message = {
                        MessageType::TaskResult,
                        message.data
                };

                try {
                    client.connection->sendMessage(json(result_message).dump());
                } catch (json::exception &e) {
                    std::cerr << logTime() << "Error sending task result to client: " << e.what() << std::endl;
                    continue;
                }

                std::cout << logTime() << "Sent TaskResult to client: " << client.id << std::endl;
                checkTaskQueue(worker);
                break;
            }

            default:
                std::cerr << logTime() << "Unknown action: " << message.action << std::endl;
                break;
        }
    }
}

void RabbitServer::processClient(Client &client) {
    std::vector<std::thread> threads;

    for (;;) {
        auto receiveMessage = client.connection->receiveMessage();

#ifdef SERVER_ARCH_DEBUG
        std::cout << "Received message: " << receiveMessage->getDataAsString() << std::endl;
#endif

        std::cout << "start converting" << std::endl;
        auto rawMessage = receiveMessage->getData();
        char *payload = static_cast<char *>(rawMessage.first);

        std::cout << "start parsing" << std::endl;
        json request = json::parse(payload, payload + rawMessage.second);

        Message message;
        json data;

        try {
            std::cout << "Loaded to request" << std::endl;
            message = request.template get<Message>();
            data = json::parse(message.data);
            std::cout << "Loaded to data1" << std::endl;
        } catch (json::exception &e) {
            std::cerr << "Error parsing message: " << e.what() << std::endl;
            continue;
        }

        switch (message.action) {
            case MessageType::TaskRequest: {
                std::cout << logTime() << "Received TaskRequest from client: " << client.id << std::endl;
                struct TaskRequest tRequest;

                try {
                    std::cout << "Loaded to data2" << std::endl;
                    tRequest = data.template get<struct TaskRequest>();
                    std::cout << "Loaded to taskRequest" << std::endl;
                    std::cout << logTime() << "Task " << tRequest.id << " added (Cores: " << tRequest.cores << ")";
                    std::cout << std::endl;
                } catch (json::exception &e) {
                    std::cerr << logTime() << "Error parsing task: " << e.what() << std::endl;
                    break;
                }

                Task task = {
                        tRequest.id,
                        tRequest.func,
                        tRequest.data,
                        "",
                        tRequest.cores,
                        TaskStatus::Queued,
                        "",
                        client.id
                };

                try {
                    taskService.addTask(task);
                    threads.emplace_back([this, task]() mutable {
                        this->processTask(task);
                    });
                } catch (std::exception &e) {
                    std::cerr << logTime() << "Error processing task: " << e.what() << std::endl;
                }
                break;
            }

            default:
                std::cerr << logTime() << "Unknown action: " << message.action << std::endl;
                break;
        }
    }
    for (auto &thread: threads) {
        if (thread.joinable()) thread.join();
    }
}

void RabbitServer::checkTaskQueue(Worker &worker) {
    userDBService.printLog();
    std::cout << logTime() << "Checking task queue for worker " << worker.id << std::endl;
    std::cout << logTime() << "Worker " << worker.id << " has " << worker.cores - worker.usedCores
              << " free cores " << "(cores: " << worker.cores << ", used: " << worker.usedCores << ")\n";

    Task pendingTask;
    if (pendingTasks.tryDequeue(pendingTask, worker.cores - worker.usedCores)) {
        std::cout << logTime() << "Assigning pending task " << pendingTask.id << " to worker "
                  << worker.id << "\n";

        struct TaskRequest taskRequest = {
                pendingTask.id,
                pendingTask.func,
                pendingTask.input,
                pendingTask.cores
        };

        Message message = {
                MessageType::TaskRequest,
                json(taskRequest).dump()
        };

        // update worker
        userDBService.modifyWorkerUsedCores(worker.id, pendingTask.cores, true);

        pendingTask.worker_hash_id = worker.id;
        pendingTask.status = TaskStatus::SentToWorker;
        taskService.updateTask(pendingTask);

        try {
            json message_json = message;
            std::string strToSend = json(message_json).dump();
            worker.connection->sendMessage(strToSend);
        } catch (json::exception &e) {
            std::cerr << logTime() << "[checkTaskQueue] Error sending task to worker: " << e.what() << std::endl;
            return;
        }

        std::cout << logTime() << "Task " << pendingTask.id << " sent to worker " << worker.id << std::endl;
    }
}


void RabbitServer::processTask(Task &task) {
    userDBService.printLog();
    Worker worker = userDBService.findMostFreeWorker(task.cores);
    std::cout << logTime() << "Found worker: " << worker.id << " (Cores: " << worker.cores << ")\n";

    if (worker.id.empty()) {
        pendingTasks.enqueue(task);
        std::cout << logTime() << "Task " << task.id << " added to queue.\n";
        return;
    }

    task.worker_hash_id = worker.id;
    task.status = TaskStatus::SentToWorker;
    taskService.updateTask(task);

    // update worker
    userDBService.modifyWorkerUsedCores(worker.id, task.cores, true);
    struct TaskRequest taskRequest = {
            task.id,
            task.func,
            task.input,
            task.cores
    };

    Message message = {
            MessageType::TaskRequest,
            json(taskRequest).dump()
    };

    try {
        std::string strToSend = json(message).dump();
        worker.connection->sendMessage(strToSend);
    } catch (json::exception &e) {
        std::cerr << logTime() << "[processTask] Error sending task to worker: " << e.what() << std::endl;
        return;
    }

    std::cout << logTime() << "Task " << task.id << " sent to worker " << worker.id << std::endl;
}

std::string RabbitServer::logTime() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X: ");
    return ss.str();
}