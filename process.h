#ifndef PROCESS_H
#define PROCESS_H

#include <QObject>
#include <iostream>
#include <string>
#include <bitset>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <cstring>
#include "math.h"
#include <fstream>
#include <ctime>
#include <stdexcept> // for std::runtime_error
#include <complex>
#include "sharedsource.h"
#include "overall_control.h"

using Complex = std::complex<double>;

#define MODE_S_ICAO_CACHE_LEN 1024 // Power of two required
#define MODE_S_LONG_MSG_BYTES (112/8)
#define MODE_S_UNIT_FEET 0
#define MODE_S_UNIT_METERS 1

// Program state
typedef struct {
    // Internal state
    uint32_t icao_cache[sizeof(uint32_t)*MODE_S_ICAO_CACHE_LEN*2]; // Recently seen ICAO addresses cache

    // Configuration
    int fix_errors; // Single bit error correction if true
    int aggressive; // Aggressive detection algorithm
    int check_crc;  // Only display messages with good CRC
} mode_s_t;
struct mode_s_msg {
    // Generic fields
    unsigned char msg[MODE_S_LONG_MSG_BYTES]; // Binary message
    int msgbits;                // Number of bits in message
    int msgtype;                // Downlink format #
    int crcok;                  // True if CRC was valid
    uint32_t crc;               // Message CRC
    int errorbit;               // Bit corrected. -1 if no bit corrected.
    int aa1, aa2, aa3;          // ICAO Address bytes 1 2 and 3
    int phase_corrected;        // True if phase correction was applied.

    // DF 11
    int ca;                     // Responder capabilities.

    // DF 17
    int metype;                 // Extended squitter message type.
    int mesub;                  // Extended squitter message subtype.
    int heading_is_valid;
    int heading;
    int aircraft_type;
    int fflag;                  // 1 = Odd, 0 = Even CPR message.
    int tflag;                  // UTC synchronized?
    int raw_latitude;           // Non decoded latitude
    int raw_longitude;          // Non decoded longitude
    char flight[9];             // 8 chars flight number.
    int ew_dir;                 // 0 = East, 1 = West.
    int ew_velocity;            // E/W velocity.
    int ns_dir;                 // 0 = North, 1 = South.
    int ns_velocity;            // N/S velocity.
    int vert_rate_source;       // Vertical rate source.
    int vert_rate_sign;         // Vertical rate sign.
    int vert_rate;              // Vertical rate.
    int velocity;               // Computed from EW and NS velocity.

    // DF4, DF5, DF20, DF21
    int fs;                     // Flight status for DF4,5,20,21
    int dr;                     // Request extraction of downlink request.
    int um;                     // Request extraction of downlink request.
    int identity;               // 13 bits identity (Squawk).

    // Fields used by multiple message types.
    int altitude, unit;
};




struct adsb_frame// 飞行器真实参数
{
    bool valid_flag;
    char df;
    int msgbit;
    int msgtype;
    int typecode;
    uint32_t crc;
    //df 17
    QString flight;             // 8 chars flight number.
    // DF4, DF5, DF20, DF21
    int fs;                     // Flight status for DF4,5,20,21
    int dr;                     // Request extraction of downlink request.
    int um;                     // Request extraction of downlink request.
    int identity;               // 13 bits identity (Squawk).
    // Fields used by multiple message types.
    int altitude;
    int unit;

    QString ICAO;
    float logitude;
    float latitude;
    double velocity;
    double heading;

    // uint16_t* I;
    // uint16_t* Q;
    bool OnTheGround;
    QString transponderCapability;
    QString aircraftStatus;
    QString aircraftID;
    QString targetState;
    char *message;
};

class process : public QObject
{
    Q_OBJECT
public:
    float fs;
    std::ofstream file;
    std::string fileName;
    struct adsb_frame *frame;
    char AllMessage[120];
    explicit process(sharedsources *sharedresource,QObject *parent = nullptr);
    ~process();
    std::string hex2bin(const std::string& hexstr);
    std::vector<std::string> wrap(const std::string &str, size_t chunk_size);
    int crc(const std::string& msg, bool encode);
    bool validate_crc(const std::string &msg, uint8_t l);
    int decode_downlink_format(const std::string& msg);
    std::string icao_address(const std::string& msg);
    // Aircraft Identification and Category
    std::string decodeFlightNumber(const std::string& msg);//航班号？

    float altitude(const std::string& msg);
//private:
    int hex2int(const std::string& hexstr);
    // int bin2int(const std::string& binstr);
    int gray2int(const std::string& graystr);
    float gray2alt(const std::string& msg);
    std::string hexToBinary(const std::string &hex);
    void struct_init();
    // 解析航向
    double decodeTrackAngle(const std::string &msg);

    // 解析地速
    double decodeGroundSpeed(const std::string &msg);

    // 解析高度
    int decodeAltitude(const std::string &msg);

    // 解析经纬度
    std::pair<double, double> decodePosition(const std::string &msg);

    // 解析Transponder capability
    std::string decodeTransponderCapability(const std::string &msg);

    // 解析飞行器状态
    std::string decodeAircraftStatus(int typeCode);

    // 解析航空器识别码
    std::string decodeAircraftIdentification(const std::string &msg);

    // 解析飞机是否着陆的标志位
    bool isAircraftOnGround(const std::string &msg);

    // 解析目标状态和状态信息
    std::string decodeTargetStateAndStatus(const std::string &msg);
    // 将6-bit编码的字符转换为ASCII字符
    char decodeChar(int code);

    std::string decodeIcaoAddress(const std::string &msg);
signals:
    void process_done(struct adsb_frame *adsb_frame);
    void process_whole_done();
private:

    void writeIQData(int16_t *I, int16_t *Q, long length);
    std::string complexToString(const Complex& c);
    sharedsources* sharedresources;
public slots:
    void do_process(int16_t *I, int16_t *Q, long int length, long long fs);
};

char* array2string(uint16_t* array,int length);
char decodeChar(int code);
static char* bit2byte(uint16_t* array,int length);
#endif // PROCESS_H
