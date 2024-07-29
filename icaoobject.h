#ifndef ICAOOBJECT_H
#define ICAOOBJECT_H
#include "qstring.h"
#include "process.h"
class ICAOObject {
public:
    ICAOObject(struct adsb_frame *data);


    void updateData(struct adsb_frame &data) {
        // 更新对象数据
        lastData = data;
        resetSurvivalTime();
    }

    void incrementSurvivalTime();

    void resetSurvivalTime();

    bool isExpired();

private:
    QString icao;
    int survivalTime;
    adsb_frame lastData;

    static const int MAX_SURVIVAL_TIME = 300; // 假设存活时间为300秒
};


#endif // ICAOOBJECT_H
