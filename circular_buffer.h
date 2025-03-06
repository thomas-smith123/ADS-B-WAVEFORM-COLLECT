#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H
#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <cstdlib>
#include <atomic>
#include <cstring>
#include <vector>
#include <QSharedMemory>
#include <QAtomicInt>
#include <QReadWriteLock>

class circular_buffer: public QObject {
    Q_OBJECT

public:
    static constexpr long int BUFFER_SIZE = 1024*1024*10*10; // 缓冲区大小（必须是2的幂次，优化取模运算）
    static constexpr int SAMPLE_SIZE = 1024*1024*5; // 每次读/写数据大小（与你的读写一致）
    explicit circular_buffer(long int bufferSize = BUFFER_SIZE, QObject *parent = nullptr);
    ~circular_buffer();

    // 写入数据（生产者）
    bool pushData(const int16_t *I0, const int16_t *Q0, long int size);

    // 读取数据（消费者）
    bool popData(int16_t *I0, int16_t *Q0, long int size);
    void getdata();
    int16_t *I0; int16_t *Q0;
    long int size;
    bool start_process;
    bool dataAvailable;
    long long fs;
// signals:
//     void dataAvailable(); // 当有数据可读时，发出信号
public slots:
    void receiveDataSlot(int16_t* I0, int16_t* Q0, long int size, long long fs_hz);  // 数据接收槽函数
    void onProcessDone();
    void processData();

private:
    struct SharedMemoryHeader {
        QAtomicInt writeIndex;  // 原子变量，支持并发写入
        QAtomicInt readIndex;   // 原子变量，支持并发读取
        size_t bufferSize;      // 缓冲区大小
    };
    QSharedMemory sharedMemory;
    SharedMemoryHeader* header;
    QReadWriteLock memoryLock;  // 读写锁
    int16_t* bufferI; // 存储 I 分量
    int16_t* bufferQ; // 存储 Q 分量
    long int bufferSize;  // 每帧大小
    int alignment;   // 内存对齐大小

    // std::atomic<long int> writeOffset;  // 写入索引
    // std::atomic<long int> readOffset;   // 读取索引
    // std::atomic<long int> availableSamples; // 当前可用样本数

    QMutex mutex; // 线程安全
    QWaitCondition dataReady; // 条件变量（等待数据可用）

};


#endif // CIRCULAR_BUFFER_H
