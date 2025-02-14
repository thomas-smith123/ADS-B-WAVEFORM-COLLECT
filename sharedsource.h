#ifndef SHAREDSOURCE_H
#define SHAREDSOURCE_H
#include <QMutex>
#include <QWaitCondition>
// #include <predefine.h>
// #include <QReadWriteLock>
class sharedsource {
public:
    sharedsource();
    QMutex mutex;
    QWaitCondition condition;
    bool isProcessing = false;
    bool emit_signal_to_process = true;
};

// class GlobalMap
// {
// public:
//     static GlobalMap& instance()
//     {
//         static GlobalMap instance;
//         return instance;
//     }

//     void insert(std::string key, struct ADSBFrame value)
//     {
//         QWriteLocker locker(&lock);
//         map[key] = value;
//     }
//     void remove(std::string key)
//     {
//         QWriteLocker locker(&lock);
//         map.remove(key);
//     }
//     bool contains(std::string key)  // 返回true如果key存在，false如果key不存在
//     {
//         QReadLocker locker(&lock);
//         return map.contains(key);  // 检查key是否存在
//     }
//     struct ADSBFrame get(std::string key)
//     {
//         QReadLocker locker(&lock);
//         return map[key];  // 返回对应的值
//     }
// private:
//     QMap<std::string, struct ADSBFrame> map;
//     QReadWriteLock lock;


//     GlobalMap() {}
//     GlobalMap(const GlobalMap&) = delete;
//     GlobalMap& operator=(const GlobalMap&) = delete;
// };
#endif // SHAREDSOURCE_H
