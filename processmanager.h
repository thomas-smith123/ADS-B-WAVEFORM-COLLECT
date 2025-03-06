#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include "process_.h"
#include "taskQueue.h"
#include "globalBuffer.h"
#include "circular_buffer.h"

class filewriter_;
class MainWindow; // forward declear
class processManager : public QObject
{
    Q_OBJECT
public:
    explicit processManager(MainWindow *window, sharedsource *sharedresource, filewriter_ *filewriter, int numthread, circular_buffer* ringHandle, long int datalength, QObject *parent = nullptr);
    ~processManager();
    void startprocessing();
    // void enqueueTask(int16_t *I, int16_t *Q, long int length, long long fs);
    void stopprocessing();
    bool checkprocessing();
    MainWindow *window;
    filewriter_ *filewriter;
    std::string fileName;
    QFile *file;
    GlobalBuffer buffer;
    bool allTaskFinish;

public slots:
    void addNewTask(int16_t* I, int16_t* Q, long int dataSize, long long fs);
    void allTaskFinished();
    void getAndProcess();

signals:
    void stopThreads();
    void taskReady(int16_t *I, int16_t *Q, long int length, long long fs);
    void taskCompleted();

private:
    float *data;
    float *result;
    int windowSize;
    int numthread;
    int remainingThreads;    // 记录剩余未完成的线程数
    long int dataLength; // 一次取出的数据长度
    circular_buffer *ring_buffer;
    TaskQueue taskQueue;
    QList<QThread *> threads;
    QList<adsb_decoder *> workers;
    sharedsource* sharedresources;
    int16_t* I; int16_t* Q;
    // QList<SlidingWindowWorker *> workers;
};

#endif // PROCESSMANAGER_H
