#include "processmanager.h"
#include "predefine.h"
#include "sharedsource.h"
#include "mainwindow.h"
#include "filewriter.h"

processManager::processManager(MainWindow *window, sharedsource *sharedresource, filewriter_ *filewriter, int numthread, circular_buffer* ringHandle, long int datalength, QObject *parent)
    : QObject{parent}
{
    ring_buffer = ringHandle;
    dataLength = datalength;
    this->numthread = numthread;

    this->window = window;
    this->filewriter = filewriter;
    this->sharedresources = sharedresource;
    I = new int16_t[datalength];  // 为 I 分配内存
    Q = new int16_t[datalength];  // 为 I 分配内存
    connect(this,&processManager::taskCompleted,this,&processManager::getAndProcess);
}
processManager::~processManager()
{
    delete[] I;
    delete[] Q;
}
void processManager::startprocessing()
{
    long int chunkSize = dataLength/numthread;
    int remainSize = dataLength%numthread;
    for (int i=0;i<this->numthread;i++)
    {
        adsb_decoder *decoder = new adsb_decoder(&this->taskQueue, &buffer);
        connect(decoder, &adsb_decoder::taskComplete, this, &processManager::allTaskFinished,Qt::ConnectionType::QueuedConnection);
        connect(decoder, &QThread::finished, decoder, &QObject::deleteLater);
        connect(decoder, &adsb_decoder::planeUpdate, window, &MainWindow::writeFramelog,Qt::ConnectionType::QueuedConnection);
        connect(decoder, &adsb_decoder::writefile, filewriter, &filewriter_::writeBuffer,Qt::ConnectionType::QueuedConnection);
        connect(decoder, &adsb_decoder::planeUpdate, window, &MainWindow::table_update);
        decoder->start();
        threads.append(QThread::currentThread());
        // QThread::currentThread().
        workers.append(decoder);  // 将线程添加到列表
    }
}
void processManager::getAndProcess()
{
    // while(ring_buffer->start_process)
    {
        while(!ring_buffer->popData(I,Q,dataLength))
            qDebug()<<"waiting";
            // QThread::sleep(5);
        addNewTask(I, Q, dataLength, ring_buffer->fs);
        // while(remainingThreads!=0);
        // qDebug()<<"waiting";

    }

}

void processManager::addNewTask(int16_t* I, int16_t* Q, long int dataSize, long long fs) {
    long int chunkSize = dataSize / numthread; // 分成 4 段
    int remainSize = dataSize % numthread;
    remainingThreads = numthread;
    QMutexLocker locker(&sharedresources->mutex);
    sharedresources->isProcessing = true;
    int tmp = fs/1000000*120;
    for (int i = 0; i < numthread; ++i) {
        if (i< numthread)
        {
            if (i>=1)
            {
                WindowTask task = {I+i * chunkSize-tmp+1, Q+i * chunkSize-tmp+1, chunkSize+tmp-1, fs};
                taskQueue.addTask(task);
            }
            else
            {
                WindowTask task = {I+i * chunkSize, Q+i * chunkSize, chunkSize, fs};
                taskQueue.addTask(task);
            }

        }
        else
        {
            WindowTask task = {I+i*chunkSize-tmp+1,Q+i*chunkSize-tmp+1, chunkSize+remainSize+tmp-1, fs};
            taskQueue.addTask(task);
        }
        // qDebug()<<"added.";
    }
}

void processManager::stopprocessing() {
    // emit stopThreads();
    for (adsb_decoder *decoder : this->workers) {
            decoder->requestInterruption();
    }
}

void processManager::allTaskFinished()
{
    remainingThreads--;

    if (remainingThreads==0)
    {
        emit taskCompleted();
        // qDebug()<<"uuu";
    }
}
