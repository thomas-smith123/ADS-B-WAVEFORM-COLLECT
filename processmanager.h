#ifndef PROCESSMANAGER_H
#define PROCESSMANAGER_H

#include <QObject>
#include "process_.h"
#include "taskQueue.h"

class filewriter_;
class MainWindow; // forward declear
class processManager : public QObject
{
    Q_OBJECT
public:
    explicit processManager(MainWindow *window, sharedsource *sharedresource, filewriter_ *filewriter, int numthread, QObject *parent = nullptr);
    void startprocessing();
    // void enqueueTask(int16_t *I, int16_t *Q, long int length, long long fs);
    void stopprocessing();
    bool checkprocessing();
    MainWindow *window;
    filewriter_ *filewriter;
    std::string fileName;
    QFile *file;
public slots:
    void addNewTask(int16_t* I, int16_t* Q, long int dataSize, long long fs);
    void allTaskFinished();
private slots:
    void onTaskCompleted() {
        qDebug() << "Task completed for range";
    }

signals:
    void stopThreads();
    void taskReady(int16_t *I, int16_t *Q, long int length, long long fs);

private:
    float *data;
    float *result;
    int windowSize;
    int numthread;
    int remainingThreads;    // 记录剩余未完成的线程数
    long int dataLength;

    TaskQueue taskQueue;
    QList<QThread *> threads;
    QList<adsb_decoder *> workers;
    sharedsource* sharedresources;
    // QList<SlidingWindowWorker *> workers;
};

#endif // PROCESSMANAGER_H
