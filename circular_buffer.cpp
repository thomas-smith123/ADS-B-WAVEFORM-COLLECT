#include "circular_buffer.h"
#include <QDebug>
#include "QThread"

void* aligned_alloc_qt(size_t alignment, size_t size) {
#if defined(_MSC_VER)  // MSVC
    return _aligned_malloc(size, alignment);
#elif defined(__GNUC__) || defined(__clang__)  // GCC/Clang
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#else
    return std::aligned_alloc(alignment, size);  // C++17+
#endif
}

// 释放对齐内存
void aligned_free_qt(void* ptr) {
#if defined(_MSC_VER)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

// #include "processmanager.h"
circular_buffer::circular_buffer(long int bufferSize, QObject *parent) : sharedMemory("CircularBufferSharedMemory") {
    size_t totalSize = sizeof(SharedMemoryHeader) + bufferSize * sizeof(int16_t) * 2;
    if (sharedMemory.attach())
        sharedMemory.detach();
    if (!sharedMemory.create(totalSize)) {
        qDebug() << "Shared memory creation failed:" << sharedMemory.errorString();
    }
    start_process = false;
    sharedMemory.lock();
    header = static_cast<SharedMemoryHeader*>(sharedMemory.data());
    bufferI = reinterpret_cast<int16_t*>(header + 1);
    bufferQ = bufferI + bufferSize;

    header->writeIndex.fetchAndStoreOrdered(0);
    header->readIndex.fetchAndStoreOrdered(0);
    header->bufferSize = bufferSize;
    sharedMemory.unlock();
}

circular_buffer::~circular_buffer(){
    dataAvailable = true;
    sharedMemory.detach();
}
// **写入数据（生产者）**
bool circular_buffer::pushData(const int16_t *I, const int16_t *Q, long int size) {
    memoryLock.lockForWrite();  // 获取写锁

    sharedMemory.lock();  // 对共享内存加锁

    int writePos = header->writeIndex.fetchAndAddRelaxed(0);  // 读取当前 writeIndex
    int readPos = header->readIndex.fetchAndAddRelaxed(0);    // 读取当前 readIndex
    size_t bufferSize = header->bufferSize;

    // 计算剩余空间
    size_t availableSpace = bufferSize - ((writePos - readPos + bufferSize) % bufferSize);

    // 如果空间不足，移动 readIndex 覆盖旧数据
    if (size > availableSpace) {
        header->readIndex.fetchAndStoreOrdered((writePos + size) % bufferSize);
    }

    // **使用 memcpy 写入数据**
    size_t copySize = size * sizeof(int16_t);  // 计算要复制的字节数

    // 写入 I 数组
    memcpy(&bufferI[(writePos) % bufferSize], I, copySize);

    // 写入 Q 数组
    memcpy(&bufferQ[(writePos) % bufferSize], Q, copySize);

    // 更新写位置
    header->writeIndex.fetchAndStoreOrdered((writePos + size) % bufferSize);

    sharedMemory.unlock();  // 解锁共享内存

    memoryLock.unlock();  // 释放写锁
    return true;
}

// **读取数据（消费者）**
bool circular_buffer::popData(int16_t *I, int16_t *Q, long int size) {
    memoryLock.lockForRead();  // 获取读锁

    sharedMemory.lock();  // 对共享内存加锁

    int writePos = header->writeIndex.fetchAndAddRelaxed(0);  // 读取当前 writeIndex
    int readPos = header->readIndex.fetchAndAddRelaxed(0);    // 读取当前 readIndex
    size_t bufferSize = header->bufferSize;

    // **如果数据不足，则返回 false**
    size_t availableData = (writePos - readPos + bufferSize) % bufferSize;
    if (size > availableData) {
        sharedMemory.unlock();  // 解锁共享内存
        memoryLock.unlock();  // 释放读锁
        return false;
    }

    // **使用 memcpy 复制数据**
    size_t copySize = size * sizeof(int16_t);  // 计算要复制的字节数

    // 复制 I 数组
    memcpy(I, &bufferI[(readPos) % bufferSize], copySize);

    // 复制 Q 数组
    memcpy(Q, &bufferQ[(readPos) % bufferSize], copySize);

    // 更新读位置
    header->readIndex.fetchAndStoreOrdered((readPos + size) % bufferSize);

    sharedMemory.unlock();  // 解锁共享内存

    memoryLock.unlock();  // 释放读锁
    return true;
}

void circular_buffer::receiveDataSlot(int16_t* I0, int16_t* Q0, long int size, long long fs_hz)
{
    pushData(I0, Q0, size);
    fs = fs_hz;
    // qDebug() << "pushed.";
}
// void circular_buffer::getdata(){
//     const long int batchSize = SAMPLE_SIZE;  // 每次处理的数据量
//     int16_t I0[batchSize], Q0[batchSize];

//     while (start_process) {

//         if (popData(this->I0, this->Q0, batchSize)) {  // 获取数据
//             qDebug() << "Processing data...";

//             // 发出处理完成的信号
//             emit(this->I0,this->Q0,size,fs);
//             dataAvailable = false;

//         } else {
//             // 如果没有数据处理，可以选择睡眠一段时间，避免忙等
//             QThread::msleep(10);  // 睡眠10毫秒
//         }
//     }
// }
// 处理数据的函数，独立线程中运行
void circular_buffer::processData() {
    qDebug() << "Processing data...";
    // getdata();
}

void circular_buffer::onProcessDone()
{
    dataAvailable = true;
}

