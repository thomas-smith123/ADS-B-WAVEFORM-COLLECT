#include "process.h"
#include <sstream>

extern double pre_lat,pre_lon;
char ais_charset[] = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";

adsb_decoder::adsb_decoder(sharedsource *sharedresource,QObject *parent)
    : QObject{parent}
{
    sharedresources = sharedresource;
    cnt = 0;
    frame = new ADSBFrame;
    last_frame = new ADSBFrame;
    QString currentDateTime = QDateTime::currentDateTime().toString("yyyyMMdd_hhmm");
    QString fileName = currentDateTime + ".csv";
    qDebug()<<fileName;
    file = new QFile (fileName);
    // 以写模式打开文件
    if (!file->open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件";
    }
    out = new QTextStream (file);
}
adsb_decoder::~adsb_decoder() {
    file->close();
    delete out;
}
std::string hex2bin(const std::string& hexstr) {
    std::string binstr;
    for (char hex_digit : hexstr) {
        switch (toupper(hex_digit)) {
        case '0': binstr.append("0000"); break;
        case '1': binstr.append("0001"); break;
        case '2': binstr.append("0010"); break;
        case '3': binstr.append("0011"); break;
        case '4': binstr.append("0100"); break;
        case '5': binstr.append("0101"); break;
        case '6': binstr.append("0110"); break;
        case '7': binstr.append("0111"); break;
        case '8': binstr.append("1000"); break;
        case '9': binstr.append("1001"); break;
        case 'A': case 'a': binstr.append("1010"); break;
        case 'B': case 'b': binstr.append("1011"); break;
        case 'C': case 'c': binstr.append("1100"); break;
        case 'D': case 'd': binstr.append("1101"); break;
        case 'E': case 'e': binstr.append("1110"); break;
        case 'F': case 'f': binstr.append("1111"); break;
        }
    }
    return binstr;
}

// 分割二进制字符串
std::vector<std::string> adsb_decoder::wrap(const std::string& str, size_t width) {
    std::vector<std::string> result;
    for (size_t i = 0; i < str.length(); i += width) {
        result.push_back(str.substr(i, width));
    }
    return result;
}

// 将二进制字符串转换为整数
int adsb_decoder::bin2int(const std::string& binstr) {
    return std::bitset<8>(binstr).to_ulong();
}

// CRC生成和校验函数
unsigned int adsb_decoder::crc(const std::string& msg, bool encode = false, bool output_result = false) {
    /*输入为'1'和'0'数组*/
    // CRC生成多项式
    unsigned int G[4] = { 0xFF, 0xFA, 0x04, 0x80 };

    std::string msg_copy = msg;

    if (encode) {
        msg_copy = msg.substr(0, msg.length() - 6) + "000000";
    }

    std::string msgbin = hex2bin(msg_copy);
    std::vector<std::string> msgbin_split = wrap(msgbin, 8);
    std::vector<int> mbytes;

    for (const auto& bin_part : msgbin_split) {
        mbytes.push_back(bin2int(bin_part));
    }

    for (size_t ibyte = 0; ibyte < mbytes.size() - 3; ++ibyte) {
        for (int ibit = 0; ibit < 8; ++ibit) {
            unsigned int mask = 0x80 >> ibit;
            int bits = mbytes[ibyte] & mask;

            if (bits > 0) {
                mbytes[ibyte] ^= (G[0] >> ibit);
                mbytes[ibyte + 1] ^= (0xFF & ((G[0] << (8 - ibit)) | (G[1] >> ibit)));
                mbytes[ibyte + 2] ^= (0xFF & ((G[1] << (8 - ibit)) | (G[2] >> ibit)));
                mbytes[ibyte + 3] ^= (0xFF & ((G[2] << (8 - ibit)) | (G[3] >> ibit)));
            }
        }
    }

    unsigned int result = (mbytes[mbytes.size() - 3] << 16) |
                          (mbytes[mbytes.size() - 2] << 8) |
                          mbytes[mbytes.size() - 1];
    if(!output_result)
        return (result != 0);
    else
        return result;
}



unsigned int adsb_decoder::hex_to_int(const std::string& hex) {
    if (hex.length() > 6) {
        throw std::invalid_argument("Input string for hex_to_int must be at most 6 characters long.");
    }
    return std::stoul(hex, nullptr, 16);
}

std::string adsb_decoder::adsb_icao(const std::string& msg) {
    int df_ = adsb_decoder::df(msg);
    if (df_==0||df_==16||df_==4||df_==5||df_==20||df_==21)
    {
        unsigned int c0 = adsb_decoder::crc(msg, true, true);
        unsigned int c1 = hex_to_int(msg.substr(msg.length() - 6));
        unsigned int addr_int = c0 ^ c1;
        std::ostringstream oss;
        oss << std::setw(6) << std::setfill('0') << std::hex << addr_int;
        return oss.str();
        // std::string combined_value_hex = computeXorWithLast6Hex(msg);

    }
    else
        if(df_==11||df_==17||df_==18||df_==24)
        {
            std::string msgbin = hex2bin(msg);
            int icao = std::bitset<24>(msgbin.substr(8, 24)).to_ulong();
            std::stringstream ss;
            ss << std::hex << icao;
            std::string hexString = ss.str();
            return hexString;
        }
        else
            return "FFFFFF";
}
int adsb_decoder::df(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int df = std::bitset<5>(msgbin.substr(0, 5)).to_ulong();;
    return df;
}

int adsb_decoder::ca(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int df = std::bitset<3>(msgbin.substr(5, 3)).to_ulong();;
    return df;
}
int adsb_decoder::tc(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int tc = std::bitset<5>(msgbin.substr(32, 5)).to_ulong();;
    return tc;
}

std::string adsb_decoder::callsign(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    //std::bitset<48> bit(msgbin.substr(40, 48));
    char hexString[8], cnt = 0;
    for (int i = 0; i < 48; i += 6)
    {

        int index = (msgbin[i + 40 + 0] - '0') << 5 | (msgbin[i + 40 + 1] - '0') << 4 | (msgbin[i + 40 + 2] - '0') << 3 | (msgbin[i + 40 + 3] - '0') << 2 | (msgbin[i + 40 + 4] - '0') << 1 | (msgbin[i + 40 + 5] - '0');
        hexString[cnt++] = ais_charset[index];
    }
    std::string str(hexString);

    return str.substr(0, 8);
}

int adsb_decoder::ss(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int ss = std::bitset<5>(msgbin.substr(37, 2)).to_ulong();;
    return ss;
}

int adsb_decoder::saf(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int saf = std::bitset<1>(msgbin.substr(39, 1)).to_ulong();;
    return saf;
}

double adsb_decoder::alt_Barometric(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    std::bitset<12> tmp(msgbin.substr(40, 12));
    int n;
    if (tmp[7] == 1)
        n = 25;
    else
        n = 100;
    int32_t alt = std::bitset<11>(tmp.to_string().substr(0, 7) + tmp.to_string().substr(8, 4)).to_ulong()*n-1000;
    return alt*0.3048;
}
double adsb_decoder::alt_GNSS(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int32_t alt = std::bitset<12>(msgbin.substr(40, 12)).to_ulong();;
    return alt*0.3048;
}

int adsb_decoder::cpr_flag(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int flag = std::bitset<1>(msgbin.substr(53, 1)).to_ulong();;
    return flag;
}

uint64_t adsb_decoder::cpr_lat(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    uint64_t alt = std::bitset<17>(msgbin.substr(54, 17)).to_ulong();;
    return alt;
}

uint64_t adsb_decoder::cpr_lon(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    uint64_t alt = std::bitset<17>(msgbin.substr(71, 17)).to_ulong();;
    return alt;
}

uint64_t adsb_decoder::movement(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    uint64_t movement = std::bitset<37>(msgbin.substr(37, 7)).to_ulong();;
    return movement;
}

int adsb_decoder::ground_track(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int gt = std::bitset<7>(msgbin.substr(45, 7)).to_ulong();;
    return gt;
}

int adsb_decoder::subtype(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int st = std::bitset<3>(msgbin.substr(37, 3)).to_ulong();;
    return st;
}

int adsb_decoder::Intent_change_flag(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int flag = std::bitset<1>(msgbin.substr(40, 1)).to_ulong();;
    return flag;
}

int adsb_decoder::nac(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int nac = std::bitset<3>(msgbin.substr(42, 3)).to_ulong();;
    return nac;
}

int adsb_decoder::swe(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int swe = std::bitset<1>(msgbin.substr(45, 1)).to_ulong();;
    return swe;
}

int adsb_decoder::vwe(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int vwe = std::bitset<10>(msgbin.substr(46, 10)).to_ulong();;
    return vwe;
}

int adsb_decoder::sns(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int swe = std::bitset<1>(msgbin.substr(56, 1)).to_ulong();;
    return swe;
}

int adsb_decoder::vns(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int vwe = std::bitset<10>(msgbin.substr(57, 10)).to_ulong();;
    return vwe;
}

int adsb_decoder::vrsc(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int vrsc = std::bitset<1>(msgbin.substr(67, 1)).to_ulong();;
    return vrsc;
}

int adsb_decoder::svr(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int svr = std::bitset<1>(msgbin.substr(68, 1)).to_ulong();;
    return svr;
}

int adsb_decoder::vr(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int vr = std::bitset<9>(msgbin.substr(69, 9)).to_ulong();;
    return vr;
}

int adsb_decoder::sdif(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int svr = std::bitset<1>(msgbin.substr(80, 1)).to_ulong();;
    return svr;
}

int adsb_decoder::dif(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int vr = std::bitset<7>(msgbin.substr(81, 7)).to_ulong();;
    return vr;
}

int adsb_decoder::hs(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int hs = std::bitset<1>(msgbin.substr(45, 1)).to_ulong();;
    return hs;
}

int adsb_decoder::hdg(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int hdg = std::bitset<10>(msgbin.substr(46, 10)).to_ulong();;
    return hdg;
}

int adsb_decoder::ast(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int ast = std::bitset<1>(msgbin.substr(56, 1)).to_ulong();;
    return ast;
}

int adsb_decoder::as(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int as = std::bitset<10>(msgbin.substr(57, 10)).to_ulong();;
    return as;
}

int adsb_decoder::fs(const std::string& msg)
{
    std::string msgbin = hex2bin(msg);
    int fs = std::bitset<3>(msgbin.substr(5, 3)).to_ulong();;
    return fs;
}
int adsb_decoder::dr(const std::string& msg)
{
    std::string msgbin = hex2bin(msg);
    int dr = std::bitset<5>(msgbin.substr(8, 5)).to_ulong();;
    return dr;
}
int adsb_decoder::um(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int um = std::bitset<6>(msgbin.substr(13, 6)).to_ulong();;
    return um;
}
int gray2int(const std::string& graystr) {
    int binary = 0;
    binary = graystr[0] - '0'; // 第一位直接赋值
    for (size_t i = 1; i < graystr.size(); ++i) {
        binary = (binary << 1) | (((binary >> (i - 1)) & 1) ^ (graystr[i] - '0'));
    }
    return binary;
}

// 将灰码转为高度
int gray2alt(const std::string& binstr) {
    // 分割出 500 英尺和 100 英尺的部分
    std::string gc500 = binstr.substr(0, 8);
    int n500 = gray2int(gc500);

    // 100 英尺部分
    std::string gc100 = binstr.substr(8);
    int n100 = gray2int(gc100);

    if (n100 == 0 || n100 == 5 || n100 == 6) {
        return -99999;
    }

    if (n100 == 7) {
        n100 = 5;
    }

    if (n500 % 2) {
        n100 = 6 - n100;
    }

    int alt = (n500 * 500 + n100 * 100) - 1300;
    return alt;
}
double adsb_decoder::ac(const std::string& msg, struct ADSBFrame& frame)
{
    double alt = 0;
    std::string msgbin = hex2bin(msg);
    std::string tmp = std::bitset<13> (msgbin.substr(19, 13)).to_string();
    // int w = tmp[6],q = tmp[8], j = tmp[6],k=tmp[8];
    // if (tmp.to_ulong()==0)
    //     return -99999;
    if (tmp[6] == '0' && tmp[8] == '0')
    {
        char C1 = tmp[0];
        char A1 = tmp[1];
        char C2 = tmp[2];
        char A2 = tmp[3];
        char C4 = tmp[4];
        char A4 = tmp[5];
        // char M = binstr[6];
        char B1 = tmp[7];
        // char Q = binstr[8];
        char B2 = tmp[9];
        char D2 = tmp[10];
        char B4 = tmp[11];
        char D4 = tmp[12];
        std::string graystr = {D2, D4, A1, A2, A4, B1, B2, B4, C1, C2, C4};
        double alt = gray2alt(graystr)*1.0;
        return alt;
    }
        // return NULL;
    else if (tmp[6] == '1')
    {
        alt = std::bitset<12>(tmp.substr(0, 6) + tmp.substr(7, 6)).to_ulong();//ft
        alt = alt*3.28084;
        frame.unit = 0;
    }
    else if (tmp[6] == '0' && tmp[8] == '1')
    {
        alt = std::bitset<11>(tmp.substr(0, 6) + tmp.substr(7, 1) + tmp.substr(9, 4)).to_ulong() * 25 - 1000;//ft
        frame.unit = 0;
        // alt *= 0.3048;
    }
    return alt;
}

int adsb_decoder::vs(const std::string& msg) {
    std::string msgbin = hex2bin(msg);
    int vs = std::bitset<1>(msgbin.substr(5, 1)).to_ulong();;
    return vs;
}


std::string adsb_decoder::id(const std::string& msg)
{
    std::string msgbin = hex2bin(msg);
    std::bitset<13> tmp(msgbin.substr(19, 13));
    std::string tmp_str = tmp.to_string();
    std::stringstream A,B,C,D;
    A << tmp[5] << tmp[3] << tmp[1];

    B << tmp[7] << tmp[9] << tmp[11];
    C << tmp[4] << tmp[2] << tmp[0];
    D << tmp[13] << tmp[11] << tmp[9];
    std::string A_ = A.str(),B_ = B.str(),C_=C.str(),D_=D.str();

    return A_+B_+C_+D_;
}


// private
int adsb_decoder::mod(int a, int b) {
    return a - b * floor(a / b);
}
int adsb_decoder::nlz(uint64_t x) {
    int result = floor(2 * pi / std::acos(1 - ((1 - std::cos(pi / 2 / NZ)) / (std::cos(pi / 180 * x) * std::cos(pi / 180 * x)))));
    return result;
}
int adsb_decoder::modesMessageLenByType(int type) {
    if (type == 16 || type == 17 ||
        type == 19 || type == 20 ||
        type == 21 || type == 22 ||
        type == 18)
        return MODES_LONG_MSG_BITS;
    else
        if (type == 0 || type == 4 ||
            type == 5 || type == 11 ||
            type == 21)
            return MODES_SHORT_MSG_BITS;
        else
            return 0;
}
int adsb_decoder::decode(const std::string& msg, struct ADSBFrame& frame)
{
    //ads-b: df 17, 18
    //mode s: df 4, 5, 20, 21
    //others: df 0, 16
    //adsb commb 前提是crc通过

    int DF = adsb_decoder::df(msg);
    frame.df = DF;
    int bits = adsb_decoder::modesMessageLenByType(DF);

    if (bits == 0)
        return 0; //wrong df
    else if (bits == MODES_SHORT_MSG_BITS)
        std::string msg = msg.substr(0, MODES_SHORT_MSG_BITS);
    else std::string msg = msg;
    int tc = adsb_decoder::tc(msg);
    frame.tc = tc;
    switch (DF)
    {
        case 0:
            frame.alt = ac(msg, frame);
            frame.vs = vs(msg);
            break;

        case 4:
            frame.flight_status = fs(msg);
            frame.downlink_request = dr(msg);
            frame.utility_message = um(msg);
            frame.alt = ac(msg, frame);
            break;
        case 5:
            frame.downlink_request = dr(msg);
            frame.utility_message = um(msg);
            frame.flight_status = fs(msg);
            frame.downlink_request = dr(msg);
            frame.utility_message = um(msg);
            // frame.ca = ca(msg);
            break;
        case 16:
            //todo worong icao
            frame.alt = ac(msg,frame);
            frame.vs = vs(msg);
            break;
        case 17:

            if (tc <= 4 && tc >= 1)
            {
                //Aircraft identification
                if (tc == 2)
                {
                    switch (adsb_decoder::ca(msg))
                    {
                    case 1:
                        frame.category = planeCategory_::Surface_emergency_vehicle;
                        break;
                    case 2:
                        break;
                    case 3:
                        frame.category = planeCategory_::Surface_service_vehicle;
                        break;
                    case 4:
                        frame.category = planeCategory_::Ground_obstruction;
                    case 5:
                        frame.category = planeCategory_::Ground_obstruction;
                    case 6:
                        frame.category = planeCategory_::Ground_obstruction;
                    case 7:
                        frame.category = planeCategory_::Ground_obstruction;
                        break;
                    }
                }
                else if (tc == 3)
                {

                    switch (adsb_decoder::ca(msg))
                    {
                    case 1:
                        frame.category = planeCategory_::Glider_sailplane;
                        break;
                    case 2:
                        frame.category = planeCategory_::Lighter_than_air;
                        break;
                    case 3:
                        frame.category = planeCategory_::Parachutist_skydiver;
                        break;
                    case 4:
                        frame.category = planeCategory_::Ultralight_handglider_paraglider;
                        break;
                    case 5:
                        break;
                    case 6:
                        frame.category = planeCategory_::uav;
                        break;
                    case 7:
                        frame.category = planeCategory_::Space_or_transatmospheric_vehicle;
                        break;
                    }
                }
                else if (tc == 4)
                {
                    // frame.callsign = adsb_decoder::callsign(msg);
                    switch (adsb_decoder::ca(msg))
                    {
                    case 1:
                        frame.category = planeCategory_::light;
                        break;
                    case 2:
                        frame.category = planeCategory_::medium1;
                        break;
                    case 3:
                        frame.category = planeCategory_::medium2;
                        break;
                    case 4:
                        frame.category = planeCategory_::High_vortex_aircraft;
                        break;
                    case 5:
                        frame.category = planeCategory_::heavy;
                        break;
                    case 6:
                        frame.category = planeCategory_::High_performance_and_high_speed;
                        break;
                    case 7:
                        frame.category = planeCategory_::Rotorcraft;
                        break;
                    }
                }
                frame.callsign = callsign(msg);
            }
            else if (tc <= 8 && tc >= 5)
            {
                //todo Locally unambiguous decoding
                //Surface position
                frame.ss = adsb_decoder::ss(msg);
                frame.saf = adsb_decoder::saf(msg);
                frame.cprflag = adsb_decoder::cpr_flag(msg);
                frame.latcpr = adsb_decoder::cpr_lat(msg);
                frame.loncpr = adsb_decoder::cpr_lon(msg);
                frame.movement = adsb_decoder::movement(msg);
                if (frame.pre_lat==NULL)
                    break;
                if (frame.cprflag)
                {
                    //odd frame
                    frame.oddtime = clock();
                    frame.latcpr_odd = adsb_decoder::cpr_lat(msg);
                    frame.loncpr_odd = adsb_decoder::cpr_lon(msg);
                }
                else
                {
                    //even frame
                    frame.eventime = clock();
                    frame.latcpr_even = adsb_decoder::cpr_lat(msg);
                    frame.loncpr_even = adsb_decoder::cpr_lon(msg);
                }
                // cal lat & lon
                if (abs((double)(frame.eventime - frame.oddtime)) / CLOCKS_PER_SEC<500000)
                {
                    //decode cpr
                    float LAT_CPR_EVEN = frame.latcpr_even*1.0 / 131072;
                    float LON_CPR_EVEN = frame.loncpr_even*1.0 / 131072;
                    float LAT_CPR_ODD = frame.latcpr_odd*1.0 / 131072;
                    float LON_CPR_ODD = frame.loncpr_odd*1.0 / 131072;
                    int j = floor(59 * LAT_CPR_EVEN - 60 * LAT_CPR_ODD + 0.5);//latitude index
                    float DLATE = 90 / 60, DLATO = 90 / 59;
                    float LAT_EVEN = DLATE * (mod(j, 60) + LAT_CPR_EVEN);
                    float LAT_ODD = DLATE * (mod(j, 59) + LAT_CPR_ODD);
                    // if (LAT_EVEN >= 270) LAT_EVEN -= 360;
                    // if (LAT_ODD >= 270) LAT_ODD -= 360;
                    //check
                    if (nlz(LAT_EVEN) != nlz(LAT_ODD))
                    {
                        //invalid
                        frame.lat = NULL;
                    }
                    else
                    {

                        float latN = LAT_ODD, latS = LAT_ODD-90;
                        if (abs(latN-frame.pre_lat)<=abs(latS-frame.pre_lat))
                            frame.lat = latN;
                        else
                            frame.lat = latS;

                        //check passed

                        {
                            int ni = max(nlz(LAT_EVEN), 1);
                            float DLON = 90 / ni;

                            int m = floor(LON_CPR_EVEN * (nlz(LAT_EVEN - 1) - LON_CPR_ODD * nlz(LAT_EVEN) + 0.5));
                            frame.lon = DLON * (mod(m, ni) + LON_CPR_EVEN);
                        }
                        int n = (int)frame.pre_lon/90;
                        while ((int)frame.lon/90!=n) {
                            frame.lon += 90;
                            if (frame.lon>360)
                                frame.lon -= 360;

                            }
                    }

                }

                frame.heading = adsb_decoder::ground_track(msg)*360.0/128.0;
                frame.velocity = speed_for_df17(frame.movement);
            }
            else if (tc <= 18 && tc >= 9)
            {
                //Airborne position
                //NICb is defined in airborne position messages. (TC=9–18)
                //Introduced an additional Horizontal Containment Radius (Rc) level within
                //NIC = 6 of the airborne position message(TC = 13)
                //altitude represents the barometric altitude of the aircraft
                frame.ss = adsb_decoder::ss(msg);
                frame.saf = adsb_decoder::saf(msg);
                frame.cprflag = adsb_decoder::cpr_flag(msg);
                frame.latcpr = adsb_decoder::cpr_lat(msg);
                frame.loncpr = adsb_decoder::cpr_lon(msg);
                if (frame.cprflag)
                {
                    //odd frame
                    frame.oddtime = clock();
                    frame.latcpr_odd = adsb_decoder::cpr_lat(msg);
                    frame.loncpr_odd = adsb_decoder::cpr_lon(msg);
                }
                else
                {
                    //even frame
                    frame.eventime = clock();
                    frame.latcpr_even = adsb_decoder::cpr_lat(msg);
                    frame.loncpr_even = adsb_decoder::cpr_lon(msg);
                }
                // cal lat & lon
                if (abs((double)(frame.eventime - frame.oddtime)) / CLOCKS_PER_SEC<500000)
                {
                    //decode cpr
                    float LAT_CPR_EVEN = frame.latcpr_even*1.0 / 131072;
                    float LON_CPR_EVEN = frame.loncpr_even*1.0 / 131072;
                    float LAT_CPR_ODD = frame.latcpr_odd*1.0 / 131072;
                    float LON_CPR_ODD = frame.loncpr_odd*1.0 / 131072;
                    int j = floor(59 * LAT_CPR_EVEN - 60 * LAT_CPR_ODD + 0.5);//latitude index
                    float DLATE = 360 / 60, DLATO = 360 / 59;
                    float LAT_EVEN = DLATE * (mod(j, 60) + LAT_CPR_EVEN);
                    float LAT_ODD = DLATE * (mod(j, 59) + LAT_CPR_ODD);
                    if (LAT_EVEN >= 270) LAT_EVEN -= 360;
                    if (LAT_ODD >= 270) LAT_ODD -= 360;
                    //check
                    if (nlz(LAT_EVEN) != nlz(LAT_ODD))
                    {
                        //invalid
                        frame.lat = NULL;
                    }
                    else
                    {
                        if (frame.eventime > frame.oddtime)
                            frame.lat = LAT_EVEN;
                        else
                            frame.lat = LAT_ODD;

                        //check passed
                        if (frame.eventime > frame.oddtime)
                        {
                            int ni = max(nlz(LAT_EVEN), 1);
                            float DLON = 360.0 / ni;

                            int m = floor(LON_CPR_EVEN * (nlz(LAT_EVEN - 1) - LON_CPR_ODD * nlz(LAT_EVEN) + 0.5));
                            frame.lon = DLON * (mod(m, ni) + LON_CPR_EVEN);
                        }
                        else
                        {
                            int ni = max(nlz(LAT_ODD) - 1, 1);
                            float DLON = 360.0 / ni;

                            int m = floor(LON_CPR_EVEN * (nlz(LAT_ODD - 1) - LON_CPR_ODD * nlz(LAT_ODD) + 0.5));
                            frame.lon = DLON * (mod(m, ni) + LON_CPR_ODD);
                        }
                        if (frame.lon >= 180)
                            frame.lon = frame.lon - 360;
                    }

                }
                //cal alt
                frame.alt = adsb_decoder::alt_Barometric(msg);
            }
            else if (tc <= 22 && tc >= 20)
            //高度、经纬度
            {   //GNSS altitude of the aircraft
                //Airborne velocity
                frame.ss = adsb_decoder::ss(msg);
                frame.saf = adsb_decoder::saf(msg);
                frame.cprflag = adsb_decoder::cpr_flag(msg);
                frame.latcpr = adsb_decoder::cpr_lat(msg);
                frame.loncpr = adsb_decoder::cpr_lon(msg);
                if (frame.cprflag)
                {
                    //odd frame
                    frame.oddtime = clock();
                    frame.latcpr_odd = adsb_decoder::cpr_lat(msg);
                    frame.loncpr_odd = adsb_decoder::cpr_lon(msg);
                }
                else
                {
                    //even frame
                    frame.eventime = clock();
                    frame.latcpr_even = adsb_decoder::cpr_lat(msg);
                    frame.loncpr_even = adsb_decoder::cpr_lon(msg);
                }
                // cal lat & lon
                if (abs((double)(frame.eventime - frame.oddtime)) / CLOCKS_PER_SEC<500000)
                {
                    //decode cpr
                    float LAT_CPR_EVEN = frame.latcpr_even*1.0 / 131072;
                    float LON_CPR_EVEN = frame.loncpr_even*1.0 / 131072;
                    float LAT_CPR_ODD = frame.latcpr_odd*1.0 / 131072;
                    float LON_CPR_ODD = frame.loncpr_odd*1.0 / 131072;
                    int j = floor(59 * LAT_CPR_EVEN - 60 * LAT_CPR_ODD + 0.5);//latitude index
                    float DLATE = 360 / 60, DLATO = 360 / 59;
                    float LAT_EVEN = DLATE * (mod(j, 60) + LAT_CPR_EVEN);
                    float LAT_ODD = DLATE * (mod(j, 59) + LAT_CPR_ODD);
                    if (LAT_EVEN >= 270) LAT_EVEN -= 360;
                    if (LAT_ODD >= 270) LAT_ODD -= 360;
                    float LAT;
                    //check
                    if (nlz(LAT_EVEN) != nlz(LAT_ODD))
                    {
                        //invalid
                        frame.lat = NULL;
                    }
                    else
                    {
                        if (frame.eventime > frame.oddtime)
                            frame.lat = LAT_EVEN;
                        else
                            frame.lat = LAT_ODD;

                        //check passed
                        if (frame.eventime > frame.oddtime)
                        {
                            int ni = max(nlz(LAT_EVEN), 1);
                            float DLON = 360.0 / ni;

                            int m = floor(LON_CPR_EVEN * (nlz(LAT_EVEN - 1) - LON_CPR_ODD * nlz(LAT_EVEN) + 0.5));
                            frame.lon = DLON * (mod(m, ni) + LON_CPR_EVEN);
                        }
                        else
                        {
                            int ni = max(nlz(LAT_ODD) - 1, 1);
                            float DLON = 360.0 / ni;

                            int m = floor(LON_CPR_EVEN * (nlz(LAT_ODD - 1) - LON_CPR_ODD * nlz(LAT_ODD) + 0.5));
                            frame.lon = DLON * (mod(m, ni) + LON_CPR_ODD);
                        }
                        if (frame.lon >= 180)
                            frame.lon = frame.lon - 360;
                    }
                }
                //cal alt
                frame.alt = adsb_decoder::alt_GNSS(msg);
            }
            else if (tc == 19)
            {
                //Airborne velocity
                frame.st = adsb_decoder::subtype(msg);
                if (frame.st <= 2)
                {
                    int sns_ = sns(msg);
                    int sew_ = swe(msg);
                    int vwe_, vsn_;
                    if (sew_ == 1)
                        vwe_ = -1 * (vwe(msg) - 1);
                    else
                        vwe_ = (vwe(msg) - 1);

                    if (sns_ == 1)
                        vsn_ = -1 * (vns(msg) - 1);
                    else
                        vsn_ = (vns(msg) - 1);
                    frame.velocity = sqrtf(vwe_ * vwe_ + vsn_ * vsn_);
                    frame.heading = atan((vwe_*1.0 / vsn_)) * 180.0 / pi;
                    if (frame.heading < 0)
                        frame.heading += 360.0;
                    frame.vertical_rate = (vr(msg) - 1 * 64);
                    frame.svr = svr(msg);
                }
                else
                {
                    frame.velocity = as(msg);

                    if (hs(msg))
                        frame.heading = hdg(msg)*1.0 / 1024.0 * 360.0;
                    else
                        frame.heading = NULL;
                }
            }
            else if (tc == 28)
            {
                //Aircraft status
            }
            else if (tc == 29)
            {
                //Target states and status
            }
            else if (tc == 31)
            {
                //Operational status
                //NICa is defined in operational status messages. (TC=31)
                //NICc is defined in operational status messages. (TC = 31)
                if (frame.tc31flag == 0)
                {
                    std::string msgbin = hex2bin(msg);
                    frame.version = std::bitset<10>(msgbin.substr(57, 10)).to_ulong();;
                }
            }
            break;
        case 18:
            frame.category = adsb_decoder::ca(msg);
            break;
        case 20:
            frame.alt = ac(msg,frame);
            frame.flight_status = fs(msg);
            frame.vs = vs(msg);
            break;
        case 21:
            frame.flight_status = fs(msg);
            frame.downlink_request = dr(msg);
            frame.utility_message = um(msg);
            //id flight number
            break;
        case 24:
            break;
        default:
            break;
    }




}


std::string binaryArrayToHex(const uint16_t* binaryArray, int size) {
    std::stringstream hexStream;

    // Iterate through the binary array, 4 bits at a time
    for (int i = 0; i < size; i += 4) {
        int hexValue = 0;

        // Convert each group of 4 binary digits to a single hex digit
        for (int j = 0; j < 4; ++j) {
            if (i + j < size) {
                hexValue = (hexValue << 1) | binaryArray[i + j];
            }
        }

        // Convert to hexadecimal character and append to the string stream
        hexStream << std::hex << hexValue;
    }

    return hexStream.str();
}
std::string binaryArrayToHex(const int* binaryArray, int size) {
    std::stringstream hexStream;

    // Iterate through the binary array, 4 bits at a time
    for (int i = 0; i < size; i += 4) {
        int hexValue = 0;

        // Convert each group of 4 binary digits to a single hex digit
        for (int j = 0; j < 4; ++j) {
            if (i + j < size) {
                hexValue = (hexValue << 1) | binaryArray[i + j];
            }
        }

        // Convert to hexadecimal character and append to the string stream
        hexStream << std::hex << hexValue;
    }

    return hexStream.str();
}

int max(int a, int b)
{
    if (a > b)
        return a;
    else
        return b;
}
double calculateSignalPower(const int16_t* I, const int16_t* Q, size_t n) {
    double power = 0.0;

    // 计算信号的功率
    for (size_t i = 0; i < n; ++i) {
        power += I[i] * I[i] + Q[i] * Q[i];
    }

    // 平均值，即信号功率
    power /= n*4096*4096;

    return power;
}
void adsb_decoder::do_process(int16_t *I, int16_t *Q, long int length, long long fs)
{
    // this->struct_init();
    int16_t *abs_ = new int16_t[length];//maybe
    for (int i = 0; i < length; i++)
    {
        abs_[i] = sqrt(I[i]*I[i] + Q[i]*Q[i]);
        // qDebug()<< abs_[i] ;
    }
    int point=fs/1e6/2;
    int total_points = fs * 130e-6;//多给1us
    int mean[240];
    int mean_[112];
    float power;
    //     //slide windows
    QMutexLocker locker(&sharedresources->mutex);
    sharedresources->isProcessing = true;
    // #ifndef simulate
    for (int i = 0; i < (length - total_points); i++)
    {
        for (int i_ = 0; i_ < 240; i_++)
        {
            mean[i_] = 0;
            for (int j = 0; j < point; j++)
            {
                mean[i_] += abs_[i + i_ * point + j];
            }
            mean[i_] = mean[i_] / point;
        }
        if (*std::max_element(std::begin(mean), std::end(mean))<200)
            continue;
        if(!(mean[0] > mean[1] && mean[0] > mean[3] && \
            mean[0] > mean[4] && mean[0] > mean[5] && \
              mean[0] > mean[6] && mean[0] > mean[8] && \
              mean[0] > mean[10] && mean[0] > mean[11] && \
               mean[0] > mean[12] && mean[0] > mean[13] && \
               mean[0] > mean[14] && mean[0] > mean[15] && \
               mean[2] > mean[1] && mean[2] > mean[3] && \
              mean[2] > mean[4] && mean[2] > mean[5] && \
              mean[2] > mean[6] && mean[2] > mean[8] && \
              mean[2] > mean[10] && mean[2] > mean[11] && \
               mean[2] > mean[12] && mean[2] > mean[13] && \
               mean[2] > mean[14] && mean[2] > mean[15] && \
               mean[7] > mean[1] && mean[7] > mean[3] && \
              mean[7] > mean[4] && mean[7] > mean[5] && \
              mean[7] > mean[6] && mean[7] > mean[8] && \
              mean[7] > mean[10] && mean[7] > mean[11] && \
               mean[7] > mean[12] && mean[7] > mean[13] && \
               mean[7] > mean[14] && mean[7] > mean[15] && \
               mean[9] > mean[1] && mean[9] > mean[3] && \
              mean[9] > mean[4] && mean[9] > mean[5] && \
              mean[9] > mean[6] && mean[9] > mean[8] && \
              mean[9] > mean[10] && mean[9] > mean[11] && \
               mean[9] > mean[12] && mean[9] > mean[13] && \
               mean[9] > mean[14] && mean[9] > mean[15] &&
              mean[0]>900 ))
            continue;

        int high = (mean[0]+mean[2]+mean[7]+mean[9])/6;
        if (mean[4] >= high ||
            mean[5] >= high)
        {
            continue;
        }
        if (mean[11] >= high ||
            mean[12] >= high ||
            mean[13] >= high ||
            mean[14] >= high)
        {
            continue;
        }
        // // good preamble
        //         {
        //             // i += total_points/2-1;
        //             // qDebug()<< "found" ;
        //             //01-->0 10-->1
        for (int j = 0; j < 112; j++)
        {
            if (mean[j*2+16] > mean[16+j*2+1])
            {
                mean_[j] = 1;
            }
            else
            {
                mean_[j] = 0;
            }
        }
        int df = mean_[0]<<4 | mean_[1]<<3 |mean_[2]<<2 |mean_[3]<<1 |mean_[4];
        std::string str;
        std::string tmp_icao;
        char len = modesMessageLenByType(df);
        if (len==0)
            continue;
        else
        {
            if(len==MODES_SHORT_MSG_BITS)
            {
                str = binaryArrayToHex(mean_, MODES_SHORT_MSG_BITS);

            }
            else
            {
                str = binaryArrayToHex(mean_, MODES_LONG_MSG_BITS);


            }
        }
        power = calculateSignalPower(I+i,Q+i,(len+8)*point*2);
        if (power<0.02)
            continue;

        if (!crc(str,false))
            continue;

        tmp_icao = adsb_icao(str);
        *out<<QString::fromStdString(tmp_icao)<<","<<df<<","<<QString::number(len)<<",";
        // qDebug()<<QString::fromStdString(tmp_icao);
        int raw_len = fs/1000000*(len+8+10);
        *out<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")<<",";
        for(int p=0;p<raw_len;p++)
        {
            // *out<<QString::number(*(I+i+p))+QString("+j")+QString(*(Q+i+p))<<",";
            // *out<<QString::number(static_cast<int>(*(I+i+p)))<<"+j"<<QString::number(static_cast<int>(*(Q+i+p)))<<",";
            *out<<static_cast<int>(*(I+i+p))<<"+j"<<static_cast<int>(*(Q+i+p))<<",";
        }
        for(int p=0;p<(len+8)*2+10;p++)
        {
            // *out<<*(mean_+p)<<",";
            *out<<static_cast<int>(*(mean+p))<<",";
        }
        *out<<"\n";
        if (cnt>=5000)
        {
            cnt=0;
            file->flush();
        }

        i+=len-1;
        cnt ++;

        // emit writelog(str);




        frame->ICAO = tmp_icao;
        qDebug()<<tmp_icao;
        if(buff.contains(tmp_icao)) //已有
        {
            *last_frame = buff[tmp_icao];
            last_frame->lastSeen = QDateTime::currentDateTime();
            buff[tmp_icao] = *last_frame;
            decode(str,*last_frame);
            last_frame->msg = str;
            // struct ADSBFrame frame_cp = *frame;
            emit planeUpdate(*last_frame);
        }
        else //新增
        {
            frame->delthis = 0;
            frame->alt = 0;
            frame->lon = 999;
            frame->lat = 999;
            frame->velocity = 0;
            decode(str,*frame);
            frame->lastSeen = QDateTime::currentDateTime();
            buff.insert(frame->ICAO,*frame);
            frame->msg = str;
            // struct ADSBFrame frame_cp = *frame;
            emit planeUpdate(*frame);
            frame->alt = 0;
            frame->pre_lat = pre_lat;
            frame->pre_lon = pre_lon;
            frame->lon = 999;
            frame->lat = 999;
            frame->velocity = 0;

        }

        // decode(const std::string& msg, struct ADSBFrame& frame);


    }
    //     emit process_whole_done();
    sharedresources->isProcessing = false;
    sharedresources->emit_signal_to_process = true;
    sharedresources->condition.wakeAll();
    // #else
    delete abs_;
    // #endif
}
///////////////////////////////////////////////////
float speed_for_df17(int a)
{
    if(a==0)
        return NULL;
    else if(a==1)
        return 0.0;//stopped
    else if(a<=8)
        return 0.125*(a-1);
    else if(a<=12)
        return 0.25*(a-9)+1.0;
    else if(a<=38)
        return 0.5*(a-13)+2.0;
    else if(a<=93)
        return 1*(a-39)+15;
    else if(a<=108)
        return 2*(a-94)+70.0;
    else if(a<=123)
        return 5*(a-109)+100.0;
    else if (a==124)
        return 180;
    else return NULL;

}
