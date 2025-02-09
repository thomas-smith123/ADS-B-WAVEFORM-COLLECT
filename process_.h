#ifndef ADSB_DECODER_H
#define ADSB_DECODER_H

#endif // ADSB_DECODER_H
#pragma once
#include "predefine.h"
#include "taskQueue.h"
#include <QObject>
#include "QThread"
#include "QMutex"
#include <iostream>
#include <string>
#include <bitset>
#include <vector>
#include <math.h>
#include "sharedsource.h"
#include <QDebug>
#include <fstream>
#include"QDateTime"
#include <QFile>
#include "QDebug"
#include "QQueue"
// #include "processmanager.h"

#define pi 3.1415926
#define NZ 15
#define MODES_SHORT_MSG_BITS 56
#define MODES_LONG_MSG_BITS 112

int max(int a, int b);

float speed_for_df17(int a);

class adsb_decoder : public QThread
{
    Q_OBJECT
public:
    struct ADSBFrame *frame, *last_frame;
    // std::ofstream file;
    int cnt;
    std::string fileName;
    QFile *file;
    QTextStream *out;
    QString buffer;
    int bufferLineCount;
    explicit adsb_decoder(TaskQueue *taskQueue, QObject *parent = nullptr):taskQueue(taskQueue){
        // sharedresources = sharedresource;
        cnt = 0;
        // adsb_decoder::bufferSize = 100000;
        frame = new ADSBFrame;
        last_frame = new ADSBFrame;
        adsb_decoder::bufferLineCount = 0;
    }
    ~adsb_decoder();
    unsigned int crc(const std::string& msg, bool encode, bool output_result);
    static int df(const std::string& msg); //输入'1'和'0'数组
    static int ca(const std::string& msg); //输入'1'和'0'数组
    std::string adsb_icao(const std::string& msg,const std::string& msgbin);
    static int tc(const std::string& msg); //输入'1'和'0'数组
    static std::string callsign(const std::string& msg);
    static int ss(const std::string& msg); //输入'1'和'0'数组
    static int saf(const std::string& msg); //输入'1'和'0'数组
    static double alt_Barometric(const std::string& msg); //输入'1'和'0'数组
    static double alt_GNSS(const std::string& msg); //输入'1'和'0'数组
    static int cpr_flag(const std::string& msg); //输入'1'和'0'数组
    static uint64_t cpr_lat(const std::string& msg); //输入'1'和'0'数组
    static uint64_t cpr_lon(const std::string& msg); //输入'1'和'0'数组
    static int subtype(const std::string& msg); //输入'1'和'0'数组
    static int Intent_change_flag(const std::string& msg); //输入'1'和'0'数组
    static uint64_t movement(const std::string& msg); //输入'1'和'0'数组
    static int ground_track(const std::string& msg); //输入'1'和'0'数组

    int nac(const std::string& msg); //输入'1'和'0'数组
    int swe(const std::string& msg); //输入'1'和'0'数组
    int vwe(const std::string& msg); //输入'1'和'0'数组
    int sns(const std::string& msg); //输入'1'和'0'数组
    int vns(const std::string& msg); //输入'1'和'0'数组
    int vrsc(const std::string& msg); //Vertical rate source
    int svr(const std::string& msg); //Vertical rate sign
    int vr(const std::string& msg); //Vertical rate sign
    int sdif(const std::string& msg); //Diff from baro alt, sign
    int dif(const std::string& msg); //Diff from baro alt

    int hs(const std::string& msg); //Vertical rate sign
    int hdg(const std::string& msg); //Vertical rate sign
    int ast(const std::string& msg); //Diff from baro alt, sign
    int as(const std::string& msg); //Diff from baro alt
    int vs(const std::string& msg);
    static int modesMessageLenByType(int type);
    int decode(const std::string& msg, const std::string& msg_bin, struct ADSBFrame& frame);
    // mode s
    int fs(const std::string& msg);//flight status
    int dr(const std::string& msg);//downlint request
    int um(const std::string& msg);//utlity message
    double ac(const std::string& msg, struct ADSBFrame& frame);//altitude code
    std::string id(const std::string& msg);

    QMap<std::string, struct ADSBFrame> buff;

private:

    std::vector<std::string> wrap(const std::string& str, size_t width);
    int bin2int(const std::string& binstr);
    unsigned int hex_to_int(const std::string& hex);
    int mod(int a, int b);
    int nlz(uint64_t x);//number of longitude zones
    int adsb_commb(int df);

    TaskQueue *taskQueue;
    QWaitCondition waitCondition;
    sharedsource* sharedresources;
    std::atomic<bool> running{true};  // 线程安全的 bool 变量
protected:
    void run() override{
        while (!QThread::currentThread()->isInterruptionRequested())
        {
            WindowTask task;
            if(!taskQueue->waitForTask(10)){
                continue;
            }
            // taskQueue->waitForTask();
            if (!taskQueue->getTask(task))
            {
                continue;
            }
            //do process
            this->do_process(task.I,task.Q,task.windowSize,task.fs);
        }
        qDebug() << "Thread exiting...";
    }; //  this need to be decleared first, and then override it

public:
    void do_process(int16_t *I, int16_t *Q, long int length, long long fs);
signals:
    void writelog(std::string);
    void writefile(QString buffer);
    void planeUpdate(struct ADSBFrame a);
    void taskComplete();
};
std::string hex2bin(const std::string& hexstr);
std::string binaryArrayToHex(const uint16_t* binaryArray, int size);
std::string binaryArrayToHex(const int* binaryArray, int size);

