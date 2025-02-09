#ifndef TASKQUEUE_H
#define TASKQUEUE_H

#include "QQueue"
#include "predefine.h"
#include "QMutex"
#include "QCoreApplication"
#include "QWaitCondition"

// 线程安全任务队列
class TaskQueue {
public:
    TaskQueue()
    {

    }
    void addTask(const WindowTask &task) {
        QMutexLocker locker(&mutex);
        queue.enqueue(task);
        condition.wakeOne();  // 唤醒等待的线程
    }

    bool getTask(WindowTask &task) {
        QMutexLocker locker(&mutex);
        if (queue.isEmpty()) {
            return false;
        }
        task = queue.dequeue();
        return true;
    }

    bool waitForTask(int timeoutMs=10)
    {
        QMutexLocker locker(&mutex);
        if (queue.isEmpty()) {
            return condition.wait(&mutex, timeoutMs);  // **超时返回 false**
        }
        return true;  // **队列不空，返回 true**
    }
    // void waitForTask() {
    //     QMutexLocker locker(&mutex);  // 自动管理锁
    //     while (queue.isEmpty()) {
    //         condition.wait(&mutex);  // 等待任务
    //     }
    // }

private:
    QQueue<WindowTask> queue;
    QMutex mutex;
    QWaitCondition condition;
};

#endif // TASKQUEUE_H
