#ifndef RABBIT_USERDBSERVICE_H
#define RABBIT_USERDBSERVICE_H

#include "DataModel/Client.h"
#include "DataModel/Worker.h"

class UserDBService {
public:
    UserDBService() {
        clients = std::vector<Client>();
        workers = std::vector<Worker>();
    };

    void addClient(const Client &client);

    void addWorker(const Worker &worker);

    void removeClient(const Client &client);

    void removeWorker(const Worker &worker);

    void updateWorker(const Worker &worker);

    Client findClientByID(const std::string &id);

    Worker findWorkerByID(const std::string &id);

    Worker findMostFreeWorker(int cores);

    void modifyWorkerUsedCores(const std::string &id, int cores, bool increase);

    void printLog();

private:
    std::vector<Client> clients;
    std::vector<Worker> workers;

    void saveStateToFile(const std::string &filename);
};

#endif //RABBIT_USERDBSERVICE_H
