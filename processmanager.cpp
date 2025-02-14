#include "processmanager.h"
#include "predefine.h"
#include "sharedsource.h"
#include "mainwindow.h"
#include "filewriter.h"

processManager::processManager(MainWindow *window, sharedsource *sharedresource, filewriter_ *filewriter, int numthread, QObject *parent)
    : QObject{parent}
{
    this->numthread = numthread;
    long int dataLength = 1024*1024;
    this->window = window;
    this->filewriter = filewriter;
    this->sharedresources = sharedresource;
}

void processManager::startprocessing()
{
    long int chunkSize = 1024*1024/numthread;
    int remainSize = 1024*1024%numthread;
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
                WindowTask task = {I+i * chunkSize-tmp, Q+i * chunkSize-tmp, chunkSize, fs};
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
            WindowTask task = {I+i*chunkSize-tmp,Q+i*chunkSize-tmp, chunkSize+remainSize, fs};
            taskQueue.addTask(task);
        }
    }
}

void processManager::stopprocessing() {
    // emit stopThreads();
    for (adsb_decoder *decoder : this->workers) {
            decoder->requestInterruption();
    }
    sharedresources->isProcessing = false;
    sharedresources->emit_signal_to_process = true;
    sharedresources->condition.wakeAll();

    // this->workers.clear();
}

void processManager::allTaskFinished()
{
    remainingThreads--;
    if (remainingThreads==0)
    {
        // QMutexLocker locker(&sharedresources->mutex);
        sharedresources->isProcessing = false;
        sharedresources->emit_signal_to_process = true;
        sharedresources->condition.wakeAll();
    }
}
