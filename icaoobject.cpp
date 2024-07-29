#include "icaoobject.h"


ICAOObject::ICAOObject(struct adsb_frame *data)
{
    icao = data->ICAO;
    survivalTime = 0;
}

void ICAOObject::resetSurvivalTime()
{
    survivalTime = 0;
}

void ICAOObject::incrementSurvivalTime()
{
    survivalTime++;
}

bool ICAOObject::isExpired()
{
    return survivalTime > MAX_SURVIVAL_TIME;
}
