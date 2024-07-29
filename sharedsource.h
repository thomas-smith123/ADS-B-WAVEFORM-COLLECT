#ifndef SHAREDSOURCE_H
#define SHAREDSOURCE_H
#include <QMutex>
#include <QWaitCondition>
class sharedsources {
public:
    QMutex mutex;
    QWaitCondition condition;
    bool isProcessing = false;
    bool emit_signal_to_process = true;
};

#endif // SHAREDSOURCE_H
