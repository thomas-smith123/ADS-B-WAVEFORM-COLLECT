#include "sharedmemorymanager.h"

// SharedMemoryManager::SharedMemoryManager(size_t bufferSize) : buffer(bufferSize), head(0), tail(0), full(false) {}
void SharedMemoryManager::write(const QByteArray &data) {
    QMutexLocker locker(&mutex);
    buffer[head] = data;
    head = (head + 1) % buffer.size();
    if (full) {
        tail = (tail + 1) % buffer.size();  // 覆盖旧数据
    }
    full = (head == tail);
    cond.wakeAll();
}

QByteArray SharedMemoryManager::read() {
    QMutexLocker locker(&mutex);
    while (isEmpty()) {
        cond.wait(&mutex);
    }
    QByteArray data = buffer[tail];
    full = false;
    tail = (tail + 1) % buffer.size();
    return data;
}
