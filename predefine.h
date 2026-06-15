#ifndef PREDEFINE_H
#define PREDEFINE_H
#include "iostream"
#include"QDateTime"
#define simulate

struct WindowTask {
    int16_t *I;
    int16_t *Q;
    // int startIndex;  // 任务起始索引
    int windowSize;  // 要处理的长度大小
    long long fs; //采样率
};

struct ADSBFrame
{
    //flag
    bool tc31flag = false; //true for version 1 or 2
    int version=0;
    //plane attribute
    int category = 0;
    std::string callsign;
    uint64_t latcpr,loncpr;
    uint64_t latcpr_even, loncpr_even;
    uint64_t latcpr_odd, loncpr_odd;
    float lat=999.0, lon=999.0;
    float pre_lat=0.0, pre_lon=0.0;
    bool hasPreLocation=false; // Flag to indicate if pre_lat/pre_lon are valid
    float velocity,heading, vertical_rate;
    uint32_t svr; //0 for down, 1 for descending
    uint64_t movement;
    int cprflag;
    uint64_t oddtime,eventime;
    double alt;
    int ss,saf,tc,st;

    int df; //Downlink Format
    int ca; //Transponder capability
    std::string ICAO;
    int vs;//Vertical status: aircraft status, airborne (0) or on the ground (1)
    uint8_t flight_status, downlink_request, utility_message;
    bool unit; // 0 for ft, 1 for m
    // control
    int delthis; // 0 for new, 1 for keep, 2 for delete
    QDateTime lastSeen;
    std::string msg;
};

enum planeCategory_
{
    Surface_emergency_vehicle = 1,
    Surface_service_vehicle,
    Ground_obstruction,
    Glider_sailplane,
    Lighter_than_air,
    Parachutist_skydiver,
    Ultralight_handglider_paraglider,
    uav,
    Space_or_transatmospheric_vehicle,
    light,
    medium1,
    medium2,
    High_vortex_aircraft,
    heavy,
    High_performance_and_high_speed,
    Rotorcraft,
};
enum surveillanceStatus
{
    No_condition=0,
    Permanent_alert,
    Temporary_alert,
    spiCondition,
};
enum flightStatue {
    no_alert_no_SPI_aircraft_airborne=0,
    no_alert_no_SPI_aircraft_onground,
    alert_no_SPI_aircraft_airborne,
    alert_no_SPI_aircraft_onground,
    alert_SPI_aircraft_airborne_or_ground,
    no_alert_SPI_aircraft_airborne,
    resreve,
    not_assigned
};
enum downlinkRequest {
    no_request = 0,
    send_CommB_message,
    CommB_broadcast_message1_available,
    CommB_broadcast_message2_available,
};
enum utilityMessage {
    no_information = 0,
    IIS_contains_CommB_interrogator_identifier_code,
    IIS_contains_CommC_interrogator_identifier_code,
    IIS_contains_CommD_interrogator_identifier_code,
};


#endif // PREDEFINE_H
