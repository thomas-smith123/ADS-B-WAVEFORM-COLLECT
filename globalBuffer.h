#ifndef GLOBALBUFFER_H
#define GLOBALBUFFER_H
#include <QMap>
#include <QMutex>
#include <QMutexLocker>
#include <QReadWriteLock>
#include <QThread>
#include <QDebug>
#include "predefine.h"
class GlobalBuffer
{
public:
    // static GlobalBuffer& instance()
    // {
    //     static GlobalBuffer instance;
    //     return instance;
    // }
    GlobalBuffer(){}
    void insert(std::string key, struct ADSBFrame value)
    {
        // QMutexLocker locker(&mutex);
        QWriteLocker locker(&lock);  // 写操作加独占锁
        map[key] = value;
    }

    bool contains(std::string key)
    {
        // QMutexLocker locker(&mutex);
        QReadLocker locker(&lock);  // 读操作加共享锁
        return map.contains(key);
    }

    struct ADSBFrame get(std::string key)
    {
        // QMutexLocker locker(&mutex);
        QReadLocker locker(&lock);  // 读操作加共享锁
        return map[key];  // 返回对应的值
    }
    void update(std::string key, struct ADSBFrame Value)
    {
        // QMutexLocker locker(&mutex);
        QWriteLocker locker(&lock);  // 写操作加独占锁
        // if (map.contains(key))  // 只有当key存在时才修改
            map[key] = Value;  // 更新value
            // qDebug() << "Updated key" << key << "to new value:" << newValue;
    }
    void remove(std::string key)
    {
        // QMutexLocker locker(&mutex);
        QWriteLocker locker(&lock);  // 写操作加独占锁
        if (map.contains(key))
        {
            map.remove(key);  // 删除指定的key
            // qDebug() << "Removed key" << key;
        }
        else
        {
            // qDebug() << "Key" << key << "does not exist. Cannot remove.";
        }
    }
private:
    QMap<std::string, struct ADSBFrame> map;
    // QMutex mutex;
    QReadWriteLock lock;  // 使用 QReadWriteLock 替代 QMutex
    // GlobalBuffer() {}
    // GlobalBuffer(const GlobalBuffer&) = delete;
    // GlobalBuffer& operator=(const GlobalBuffer&) = delete;
};

#endif // GLOBALBUFFER_H
