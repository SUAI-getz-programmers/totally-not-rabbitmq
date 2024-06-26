#ifndef RABBIT_TASKSERVICE_H
#define RABBIT_TASKSERVICE_H

#include "DataModel/Task.h"
#include <vector>

class TaskService {
public:
    TaskService();

    void addTask(Task task);

    void changeTaskStatus(const std::string &id, TaskStatus status);

    void updateTask(const Task &task);

    Task findTaskByID(std::string id);

    void saveTasksToFile(const std::string &filename);

private:
    std::vector<Task> tasks;

    std::string newTaskID();

};


#endif //RABBIT_TASKSERVICE_H



//#include <sqlite_orm/sqlite_orm.h>
//using namespace sqlite_orm; -- remone DB for now
/*
auto storage = make_storage(
    "db.sqlite",
    make_table("tasks",
               make_column("id", &Task::id, primary_key().autoincrement()),
               make_column("queue", &Task::queue),
               make_column("message", &Task::message),
               make_column("result_message", &Task::result_message),
               make_column("internal_status", &Task::internal_status),
               make_column("worker_hash_id", &Task::worker_hash_id),
               make_column("client_hash_id", &Task::client_hash_id)
    )
);
*/
