#ifndef ADSB_DECODER_H
#define ADSB_DECODER_H

#endif // ADSB_DECODER_H
#pragma once
#include <QObject>
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

#define pi 3.1415926
#define NZ 15
#define MODES_SHORT_MSG_BITS 56
#define MODES_LONG_MSG_BITS 112
class adsb_decoder : public QObject
{
    Q_OBJECT
public:
    struct ADSBFrame *frame, *last_frame;
    // std::ofstream file;
    int cnt;
    std::string fileName;
    QFile *file;
    QTextStream *out;
    adsb_decoder(sharedsource *sharedresource,QObject *parent = nullptr);
    ~adsb_decoder();
    unsigned int crc(const std::string& msg, bool encode, bool output_result);
    static int df(const std::string& msg); //输入'1'和'0'数组
    static int ca(const std::string& msg); //输入'1'和'0'数组
    std::string adsb_icao(const std::string& msg);
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
    int decode(const std::string& msg, struct ADSBFrame& frame);
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

    sharedsource* sharedresources;

public slots:
    void do_process(int16_t *I, int16_t *Q, long int length, long long fs);
signals:
    void writelog(std::string);
    void planeUpdate(struct ADSBFrame a);
};
std::string hex2bin(const std::string& hexstr);
std::string binaryArrayToHex(const uint16_t* binaryArray, int size);
std::string binaryArrayToHex(const int* binaryArray, int size);
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
    float lat=999.0, lon=999.0, pre_lat, pre_lon;
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
int max(int a, int b);

float speed_for_df17(int a);
