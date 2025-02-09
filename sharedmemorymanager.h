#ifndef SHAREDMEMORYMANAGER_H
#define SHAREDMEMORYMANAGER_H

#include <QMutex>
#include <QWaitCondition>
#include <vector>

class SharedMemoryManager {
public:
    SharedMemoryManager(size_t bufferSize)
        : buffer(bufferSize), head(0), tail(0), full(false) {}

    void write(const QByteArray &data);

    QByteArray read();

private:
    bool isEmpty() const {
        return (!full && (head == tail));
    }

    std::vector<QByteArray> buffer;
    size_t head, tail;
    bool full;
    QMutex mutex;
    QWaitCondition cond;
};

#endif // SHAREDMEMORYMANAGER_H
