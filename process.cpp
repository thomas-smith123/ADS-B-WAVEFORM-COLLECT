#include "process.h"
#include <algorithm>
#include "QDebug"

#define MODE_S_PREAMBLE_US 8       // microseconds
#define MODE_S_LONG_MSG_BITS 112
#define MODE_S_SHORT_MSG_BITS 56
#define MODE_S_FULL_LEN (MODE_S_PREAMBLE_US+MODE_S_LONG_MSG_BITS)
#define MODE_S_ICAO_CACHE_TTL 60   // Time to live of cached addresses.
static const char *ais_charset = "?ABCDEFGHIJKLMNOPQRSTUVWXYZ????? ???????????????0123456789??????";
static uint16_t maglut[129*129*2];
static int maglut_initialized = 0;


process::process(sharedsources *sharedresource,QObject *parent)
    : QObject{parent}
{
    sharedresources = sharedresource;
    float fs = 10e6;
    this->struct_init();
    // 获取当前时间
    std::time_t now = std::time(nullptr);
    std::tm localTime;

// 将time_t转换为tm
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    // 创建文件名
    std::ostringstream oss;
    oss << std::put_time(&localTime, "%m-%d-%Y_%H-%M-%S") << ".csv";
    fileName = oss.str();

    // 打开文件
    file.open(fileName, std::ios::out);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + fileName);
    }

    qDebug()<< "File " << fileName << " opened successfully.\n";

}
process::~process()
{
    if (file.is_open()) {
        file.close();
        qDebug()<< "File " << fileName << " closed successfully.\n";
    }
}

// 将16进制字符串转换为二进制字符串
std::string process::hexToBinary(const std::string &hex) {
    std::string binary = "";
    for (size_t i = 0; i < hex.length(); ++i) {
        binary += std::bitset<4>(hex[i] > '9' ? hex[i] - 'A' + 10 : hex[i] - '0').to_string();
    }
    return binary;
}

void process::struct_init()
{
    frame = new struct adsb_frame;
    frame->valid_flag = false;
    frame->ICAO = NULL;
    frame->altitude = NULL;
    frame->logitude = NULL;
    frame->latitude = NULL;

    frame->velocity = NULL;
    frame->message = NULL;
    frame->heading = NULL;
}

std::string process::hex2bin(const std::string& hexstr) {
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

int process::hex2int(const std::string& hexstr) {
    int result;
    std::stringstream ss;
    ss << std::hex << hexstr;
    ss >> result;
    return result;
}

int bin2int(const std::string& binstr) {
    return std::bitset<32>(binstr).to_ulong();
}

// Function to split string into chunks
std::vector<std::string> process::wrap(const std::string &str, size_t chunk_size) {
    std::vector<std::string> result;
    for (size_t i = 0; i < str.length(); i += chunk_size) {
        result.push_back(str.substr(i, chunk_size));
    }
    return result;
}

// CRC Calculation and Verification
int process::crc(const std::string& msg, bool encode) {
    std::vector<int> G = { 0xFF, 0xFA, 0x04, 0x80 };

    std::string msg_copy = msg;
    if (encode) {
        msg_copy = msg.substr(0, msg.length() - 6) + "000000";
    }

    std::string msgbin = hex2bin(msg_copy);
    std::vector<std::string> msgbin_split = wrap(msgbin, 8);
    std::vector<int> mbytes(msgbin_split.size());

    std::transform(msgbin_split.begin(), msgbin_split.end(), mbytes.begin(), bin2int);

    for (size_t ibyte = 0; ibyte < mbytes.size() - 3; ibyte++) {
        for (int ibit = 0; ibit < 8; ibit++) {
            int mask = 0x80 >> ibit;
            if (mbytes[ibyte] & mask) {
                mbytes[ibyte] ^= (G[0] >> ibit);
                mbytes[ibyte + 1] ^= ((G[0] << (8 - ibit)) | (G[1] >> ibit)) & 0xFF;
                mbytes[ibyte + 2] ^= ((G[1] << (8 - ibit)) | (G[2] >> ibit)) & 0xFF;
                mbytes[ibyte + 3] ^= ((G[2] << (8 - ibit)) | (G[3] >> ibit)) & 0xFF;
            }
        }
    }

    int result = (mbytes[mbytes.size() - 3] << 16) | (mbytes[mbytes.size() - 2] << 8) | mbytes[mbytes.size() - 1];
    return result;
}

bool process::validate_crc(const std::string &msg, uint8_t l) {

    // Calculate the CRC of the message without the last 3 bytes (CRC)

    std::string msg_no_crc = msg.substr(0, l - 6) + "000000";
    int calculated_crc = crc(msg_no_crc, true);

    // Extract the actual CRC from the message

    int extracted_crc = std::stoi(msg.substr(l - 6), nullptr, 16);

    // Compare the calculated CRC with the extracted CRC
    return calculated_crc == extracted_crc;
}

int process::decode_downlink_format(const std::string& msg) {
    std::string hex_byte = msg.substr(0, 2);
    std::string bin_str = hex2bin(hex_byte);
    std::string df_bits = bin_str.substr(0, 5);
    int df_value = bin2int(df_bits);
    return std::min(df_value, 24);
}
std::string process::icao_address(const std::string& msg) {
    int DF = decode_downlink_format(msg);
    std::string addr;

    if (DF == 11 || DF == 17 || DF == 18) {
        addr = msg.substr(2, 6);
    }
    else if (DF == 0 || DF == 4 || DF == 5 || DF == 16 || DF == 20 || DF == 21) {
        int c0 = crc(msg, true);  // encode = true
        int c1 = hex2int(msg.substr(msg.length() - 6, 6));
        std::stringstream ss;
        ss << std::uppercase << std::hex << (c0 ^ c1);
        addr = ss.str();
    }
    else {
        return "None";
    }

    return addr;
}

// 解析ICAO地址
std::string process::decodeIcaoAddress(const std::string &msg) {
    std::stringstream icao;
    icao << std::hex << std::uppercase << std::bitset<24>(msg.substr(8, 24)).to_ulong();
    return icao.str();
}

int process::gray2int(const std::string& graystr) //FIXME
{
    int i;
    int gray = 0;
    int bin = 0;
    for (i = 0; i < graystr.length(); i++)
    {
        bin = bin ^ (graystr[i] - '0');
        gray = gray * 2 + bin;
    }
    return gray;
}

float process::gray2alt(const std::string& msg)
{
    char *gc500,*gc100;
    gc500 = new char[8];
    gc100 = new char[msg.length()-8];
    strcpy_s(gc500, 8, msg.substr(0, 7).c_str());
    int n500 = gray2int(gc500);
    strcpy_s(gc100, msg.length() - 8, msg.substr(8, msg.length()-8).c_str());
    int n100 = gray2int(gc100);
    if (n100 == 0 || n100 == 5 || n100 == 6)
        return 0;
    if (n100 == 7)
        n100 = 5;
    if (n500 % 2)
        n100 = 6 - n100;
    int alt = (n500 * 500 + n100 * 100) - 1300;
    return alt;
}

float process::altitude(const std::string& msg)
{
    char *vbin,*graystr;
    char C1,A1,C2,A2,C4,A4,B1,B2,D2,B4,D4;
    float alt;
    vbin = new char[msg.length() - 1];
    graystr = new char[msg.length() - 1];
    if (msg.length() != 13)
        return 1;
    char Mbit = msg[6], Qbit = msg[8];
    if (bin2int(msg) == 0)
        return 1; // altitude unknown or invalid
    else
    {
        if (Mbit == '0')
        {
            if (Qbit == '1')
            {
                strcpy_s(vbin, 6, msg.substr(0, 5).c_str());
                vbin[6] = msg[7];
                strcpy_s(vbin + 7, msg.length()-9, msg.substr(9, msg.length()).c_str());
                alt = bin2int(vbin) * 25 - 1000;
            }
            if (Qbit == '0')
            {
                graystr[0] = msg[10];
                graystr[1] = msg[12];
                graystr[2] = msg[1];
                graystr[3] = msg[3];
                graystr[4] = msg[5];
                graystr[5] = msg[7];
                graystr[6] = msg[9];
                graystr[7] = msg[11];
                graystr[8] = msg[0];
                graystr[9] = msg[2];
                graystr[10] = msg[4];
                alt = gray2alt(graystr);
            }
        }
        if (Mbit == '1')
        {
            strcpy_s(vbin, 6, msg.substr(0, 5).c_str());
            strcpy_s(vbin + 6, msg.length()-7, msg.substr(7, msg.length()).c_str());
            alt = (int)(bin2int(vbin) * 3.28084);
        }

    }
    return alt;
}
void process::do_process(int16_t *I, int16_t *Q, long int length, long long fs)
{
    int16_t *abs = new int16_t[length];//maybe
    for (int i = 0; i < length; i++)
    {
        abs[i] = sqrt(I[i]*I[i] + Q[i]*Q[i]);
        // qDebug()<< abs[i] ;
    }
    int point=fs/1e6/2;
    int total_points = fs * 120e-6;
    uint16_t mean[240];
    uint16_t mean_[112];
    //slide windows
    QMutexLocker locker(&sharedresources->mutex);
    sharedresources->isProcessing = true;
#ifndef simulate
    for (int i = 0; i < (length - total_points); i++)
    {
        for (int i_ = 0; i_ < 240; i_++)
        {
            mean[i_] = 0;
            for (int j = 0; j < point; j++)
            {
                mean[i_] += abs[i + i_ * point + j];
            }
            mean[i_] = mean[i_] / point;
        }
        if (*std::max_element(std::begin(mean), std::end(mean))<100)
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
            mean[9] > mean[14] && mean[9] > mean[15] ))
            continue;
        //是否必要
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
// good preamble
        {
            // qDebug()<< "found" ;
            //01-->0 10-->1
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
            char* str = array2string(mean_, 112);
            char* bytestr = bit2byte(mean_, 112);
            frame->msgtype = bytestr[0]>>3; //df
            frame->msgbits = mode_s_msg_len_by_type(frame->msgtype);
            frame->crc = ((uint32_t)bytestr[(frame->msgbits/8)-3] << 16) |
                ((uint32_t)bytestr[(frame->msgbits/8)-2] << 8) |
                (uint32_t)bytestr[(frame->msgbits/8)-1];
            uint32_t crc2 = mode_s_checksum(bytestr,frame->msgbits);
            frame->errorbit = -1;
            frame->crcok = (frame->crc == crc2);
            //修复暂时不要
            frame->ca = bytestr[0] & 7; // Responder capabilities.
            // DF 17 type (assuming this is a DF17, otherwise not used)
            frame->metype = bytestr[4] >> 3;   // Extended squitter message type.
            frame->mesub = bytestr[4] & 7;     // Extended squitter message subtype.

            // Fields for DF4,5,20,21
            frame->fs = bytestr[0] & 7;        // Flight status for DF4,5,20,21
            frame->dr = bytestr[1] >> 3 & 31;  // Request extraction of downlink request.
            frame->um = ((bytestr[1] & 7)<<3)| // Request extraction of downlink request.
                    bytestr[2]>>5;

            // In the squawk (identity) field bits are interleaved like that (message
            // bit 20 to bit 32):
            //
            // C1-A1-C2-A2-C4-A4-ZERO-B1-D1-B2-D2-B4-D4
            //
            // So every group of three bits A, B, C, D represent an integer from 0 to
            // 7.
            //
            // The actual meaning is just 4 octal numbers, but we convert it into a
            // base ten number tha happens to represent the four octal numbers.
            {
                int a, b, c, d;

                a = ((bytestr[3] & 0x80) >> 5) |
                    ((bytestr[2] & 0x02) >> 0) |
                    ((bytestr[2] & 0x08) >> 3);
                b = ((bytestr[3] & 0x02) << 1) |
                    ((bytestr[3] & 0x08) >> 2) |
                    ((bytestr[3] & 0x20) >> 5);
                c = ((bytestr[2] & 0x01) << 2) |
                    ((bytestr[2] & 0x04) >> 1) |
                    ((bytestr[2] & 0x10) >> 4);
                d = ((bytestr[3] & 0x01) << 2) |
                    ((bytestr[3] & 0x04) >> 1) |
                    ((bytestr[3] & 0x10) >> 4);
                frame->identity = a*1000 + b*100 + c*10 + d;
            }
            if (frame->msgtype != 11 && frame->msgtype != 17)
            {}

#ifdef analogout
            std::ofstream outFile("output.txt");

            // 检查文件是否成功打开
            if (!outFile.is_open()) {
                qDebug() << "无法打开文件";
                // return 1;
            }

            // 将数组元素逐行写入文件，每行写两个数组的元素，分别作为两列
            for (size_t ii = 0; ii < 120*10; ++ii) {
                outFile << I[ii+i] << "\t" << Q[ii+i] << std::endl;
            }

            // 关闭文件流
            outFile.close();
#endif
            //decode
            if (validate_crc(str, 28))
            {
                qDebug()<<str;
                int df_value = decode_downlink_format(str);
                frame->df = df_value;
                std::string icao_addr = icao_address(str);
                frame->ICAO = QString(QString::fromLocal8Bit(icao_addr.c_str()));
                std::string binaryMessage = hexToBinary(str);
                frame->typecode = std::bitset<5>(binaryMessage.substr(32, 5)).to_ulong();
                frame->transponderCapability = QString(QString::fromLocal8Bit(decodeTransponderCapability(binaryMessage).c_str()));

                if (frame->typecode >= 4 && frame->typecode <= 8)
                {
                    frame->flight = QString(QString::fromLocal8Bit(decodeFlightNumber(binaryMessage).c_str()));
                    qDebug()<<"flightnumber"<<frame->flight;
                }
                qDebug()<<"TypeCode"<<frame->typecode;

                frame->aircraftStatus = QString::fromStdString(decodeAircraftStatus(frame->typecode));

                if (frame->typecode >= 9 && frame->typecode <= 18) {
                    double trackAngle = decodeTrackAngle(binaryMessage);
                    frame->heading = trackAngle;
                    qDebug()<< "航向: " << trackAngle << "°" ;
                }

                // 解析地速
                if (frame->typecode >= 9 && frame->typecode <= 18) {
                    double groundSpeed = decodeGroundSpeed(binaryMessage);
                    frame->velocity = groundSpeed;
                    qDebug()<< "地速: " << groundSpeed << " km/h" ;
                }

                // 解析高度
                if (frame->typecode >= 9 && frame->typecode <= 18) {
                    int altitude = decodeAltitude(binaryMessage);
                    frame->altitude = altitude;
                }

                // 解析经纬度
                if (frame->typecode >= 9 && frame->typecode <= 18) {
                    auto position = decodePosition(binaryMessage);
                    frame->logitude = position.second;
                    frame->latitude = position.first;
                    qDebug()<< "纬度: " << position.first << "°" ;
                    qDebug()<< "经度: " << position.second << "°" ;
                }

                // 解析航空器识别码
                if (frame->typecode >= 1 && frame->typecode <= 4) {
                    std::string aircraftId = decodeAircraftIdentification(binaryMessage);
                    frame->aircraftID = QString::fromStdString(aircraftId);
                    qDebug()<< "航空器识别码: " << aircraftId ;
                }

                // 解析飞机是否着陆的标志位
                if (frame->typecode >= 9 && frame->typecode <= 18) {
                    bool onGround = isAircraftOnGround(binaryMessage);
                    frame->OnTheGround = onGround;
                    qDebug()<< "飞机是否着陆: " << (onGround ? "是" : "否") ;
                }

                // 解析目标状态和状态信息
                if (frame->typecode >= 29 && frame->typecode <= 31) {
                    std::string targetState = decodeTargetStateAndStatus(binaryMessage);
                    frame->aircraftStatus = QString::fromStdString(targetState);
                    qDebug()<< "目标状态和状态信息: " << targetState ;
                }
                emit process_done(frame);
            }
            // free(str);
        }
    }
    // emit process_whole_done();
    sharedresources->isProcessing = false;
    sharedresources->emit_signal_to_process = true;
    sharedresources->condition.wakeAll();
#else
    // std::string str = "8D789249EA2D0858011C084942F1";
    std::string str = "8D7C451C6005871025918865645E";
    if (validate_crc(str, 28))
    {
        qDebug()<<"CRC Check: Success";
        qDebug()<<str; //长度注意

        int df_value = decode_downlink_format(str);
        frame->df = df_value;
        std::string icao_addr = icao_address(str);
        frame->ICAO = QString(QString::fromLocal8Bit(icao_addr.c_str()));
        std::string binaryMessage = hexToBinary(str);
        frame->typecode = std::bitset<5>(binaryMessage.substr(32, 5)).to_ulong();
        frame->transponderCapability = QString(QString::fromLocal8Bit(decodeTransponderCapability(binaryMessage).c_str()));

        if (frame->typecode >= 4 && frame->typecode <= 8)
        {
            frame->flight = QString(QString::fromLocal8Bit(decodeFlightNumber(binaryMessage).c_str()));
            // qDebug()<<"flightnumber"<<frame->flight;
        }
        // qDebug()<<"TypeCode"<<frame->typecode;

        frame->aircraftStatus = QString::fromStdString(decodeAircraftStatus(frame->typecode));
        // qDebug()<<"AS"<<frame->aircraftStatus;
        // qDebug()<<"DF"<<df_value;
        // qDebug()<<"ICAO"<<icao_addr;
        // 解析航向
        if (frame->typecode >= 9 && frame->typecode <= 18) {
            double trackAngle = decodeTrackAngle(binaryMessage);
            frame->heading = trackAngle;
            // qDebug()<< "航向: " << trackAngle << "°" ;
        }

        // 解析地速
        if (frame->typecode >= 9 && frame->typecode <= 18) {
            double groundSpeed = decodeGroundSpeed(binaryMessage);
            frame->velocity = groundSpeed;
            // qDebug()<< "地速: " << groundSpeed << " km/h" ;
        }

        // 解析高度
        if (frame->typecode >= 9 && frame->typecode <= 18) {
            int altitude = decodeAltitude(binaryMessage);
            frame->altitude = altitude;
            // if (altitude != -1) {
            //     qDebug()<< "高度: " << altitude << " 英尺" ;
            // } else {
            //     qDebug()<< "高度信息无效" ;
            // }

        }

        // 解析经纬度
        if (frame->typecode >= 9 && frame->typecode <= 18) {
            auto position = decodePosition(binaryMessage);
            frame->logitude = position.second;
            frame->latitude = position.first;
            // qDebug()<< "纬度: " << position.first << "°" ;
            // qDebug()<< "经度: " << position.second << "°" ;
        }

        // 解析航空器识别码
        if (frame->typecode >= 1 && frame->typecode <= 4) {
            std::string aircraftId = decodeAircraftIdentification(binaryMessage);
            frame->aircraftID = QString::fromStdString(aircraftId);
            // qDebug()<< "航空器识别码: " << aircraftId ;
        }

        // 解析飞机是否着陆的标志位
        if (frame->typecode >= 9 && frame->typecode <= 18) {
            bool onGround = isAircraftOnGround(binaryMessage);
            frame->OnTheGround = onGround;
            // qDebug()<< "飞机是否着陆: " << (onGround ? "是" : "否") ;
        }

        // 解析目标状态和状态信息
        if (frame->typecode >= 29 && frame->typecode <= 31) {
            std::string targetState = decodeTargetStateAndStatus(binaryMessage);
            frame->aircraftStatus = QString::fromStdString(targetState);
            // qDebug()<< "目标状态和状态信息: " << targetState ;
        }
        emit process_done(frame);
    }
    sharedresources->isProcessing = false;
    sharedresources->condition.wakeAll();
#endif
}
///////////////////////////////////////////////////
void process::writeIQData(int16_t *I, int16_t *Q, long length) {
    std::vector<uint16_t> iData;
    std::vector<uint16_t> qData;
    for(int i=0;i<length;i++)
    {
        iData.push_back(I[i]);
        qData.push_back(Q[i]);
    }
    if (file.is_open()) {
        if (iData.size() != qData.size()) {
            throw std::invalid_argument("I and Q data vectors must have the same size.");
        }

        for (size_t i = 0; i < iData.size(); ++i) {
            Complex c(iData[i], qData[i]);
            file << complexToString(c);
        }
    }
}

std::string process::complexToString(const Complex& c) 
{
    std::ostringstream oss;
    oss << c.real();
    if (c.imag() >= 0) {
        oss << "+j" << c.imag();
    } else {
        oss << "-j" << -c.imag();
    }
    return oss.str();
}

// 将6-bit编码的字符转换为ASCII字符
char process::decodeChar(int code) 
{
    if (code >= 1 && code <= 26) {
        return 'A' + (code - 1);
    } else if (code >= 48 && code <= 57) {
        return '0' + (code - 48);
    } else if (code == 32) {
        return ' ';
    }
    return '?';
}
// 解析航班号
std::string process::decodeFlightNumber(const std::string &msg) {
    std::string flightNumber = "";
    for (int i = 0; i < 8; ++i) {
        int start = 40 + i * 6; // 航班号从第40位开始，每个字符占6位
        std::string p = msg.substr(start, 6);
        int code = std::bitset<6>(p).to_ulong();
        flightNumber += decodeChar(code);
    }
    return flightNumber;
}

// 解析Transponder capability
std::string process::decodeTransponderCapability(const std::string &msg) {
    int capability = std::bitset<3>(msg.substr(5, 3)).to_ulong();
    std::map<int, std::string> capabilityMap = {
        {0, "Level 1 (Surveillance Only)"},
        {1, "Level 2 (Enhanced Surveillance)"},
        {2, "Level 3 (Surveillance + ADS-B Out)"},
        {3, "Level 4 (Enhanced Surveillance + ADS-B Out)"},
        {4, "Level 5 (Surveillance + ADS-B Out + ADS-B In)"},
        {5, "Level 6 (Enhanced Surveillance + ADS-B Out + ADS-B In)"},
        {6, "Reserved"},
        {7, "Reserved"}
    };
    return capabilityMap[capability];
}

// 解析飞行器状态
std::string process::decodeAircraftStatus(int typeCode) {
    if (frame->typecode >= 9 && frame->typecode <= 18) {
        return "空速和垂直速率信息";
    } else if (frame->typecode >= 19 && frame->typecode <= 22) {
        return "空速和垂直速率信息";
    } else if (frame->typecode >= 23 && frame->typecode <= 27) {
        return "GNSS高度";
    }
    return "未知";
}

// 解析航向
double process::decodeTrackAngle(const std::string &msg) {
    int track = std::bitset<10>(msg.substr(55, 10)).to_ulong();
    return track * 360.0 / 1024.0;
}

// 解析地速
double process::decodeGroundSpeed(const std::string &msg) {
    int speed = std::bitset<10>(msg.substr(65, 10)).to_ulong();
    return speed * 1.852; // 换算为公里/小时
}

// 解析高度
int process::decodeAltitude(const std::string &msg) {
    int qBit = std::bitset<1>(msg.substr(47, 1)).to_ulong();
    if (qBit == 1) {
        int altitude = std::bitset<12>(msg.substr(40, 12)).to_ulong();
        return altitude * 25 - 1000; // 海拔高度（单位：英尺）
    }
    return -1; // Q-bit为0时，高度信息无效
}

// 解析经纬度
std::pair<double, double> process::decodePosition(const std::string &msg) {
    int latitude = std::bitset<17>(msg.substr(54, 17)).to_ulong();
    int longitude = std::bitset<17>(msg.substr(71, 17)).to_ulong();
    return std::make_pair(latitude * 180.0 / 131072.0 - 90.0, longitude * 360.0 / 131072.0 - 180.0);
}

// 将6-bit编码的字符转换为ASCII字符
char decodeChar(int code) {
    if (code >= 1 && code <= 26) {
        return 'A' + (code - 1);
    } else if (code >= 48 && code <= 57) {
        return '0' + (code - 48);
    } else if (code == 32) {
        return ' ';
    }
    return '?';
}
// 解析航空器识别码
std::string process::decodeAircraftIdentification(const std::string &msg) {
    std::string aircraftId = "";
    for (int i = 0; i < 8; ++i) {
        int start = 32 + i * 6; // 从第33位开始，每个字符占6位
        int code = std::bitset<6>(msg.substr(start, 6)).to_ulong();
        aircraftId += decodeChar(code);
    }
    return aircraftId;
}

// 解析飞机是否着陆的标志位
bool process::isAircraftOnGround(const std::string &msg) {
    return std::bitset<1>(msg.substr(53, 1)).to_ulong() == 1;
}

// 解析目标状态和状态信息
std::string process::decodeTargetStateAndStatus(const std::string &msg) {
    int targetState = std::bitset<7>(msg.substr(88, 7)).to_ulong();
    std::map<int, std::string> targetStateMap = {
        {0, "No Target State"},
        {1, "Target State Achieved"},
        {2, "Altitude Hold"},
        {3, "Autopilot Hold"},
        {4, "Approach Mode"},
        {5, "Land Mode"},
        {6, "Go Around"},
        {7, "Missed Approach"},
        {8, "Altitude Capture"},
        {9, "Vertical Speed Hold"},
        {10, "Ground Track Hold"},
        {11, "Ground Speed Hold"},
        {12, "Reserved"},
        {13, "Reserved"},
        {14, "Reserved"},
        {15, "Reserved"}
    };
    return targetStateMap[targetState];
}
static char* bit2byte(uint16_t* array,int length)
{
    int tmp_length,flag=0,cnt=0;
    if (length%4==0)
        tmp_length = length/4;
    else
    {
        tmp_length = length / 4 + 1;
        flag = 1;
    }
    char* str = new char[tmp_length];
    uint8_t tmp;
    if (flag)
    {
        for (int i = 0; i < length; i+=4)
        {
            if (i > length)break;
            str[cnt] = (array[i] << 4) | (array[i + 1] << 3) | (array[i + 2] << 2) | array[i + 3];
        }
        cnt++;
        int tmp = 0;
        for (int i = 0; i < length % 4; i++)
        {
            tmp |= (array[length-length%4+i] << (4 - i));
        }
        str[cnt-1] = tmp;
    }
    else
    {
        for (int i = 0; i < length; i += 4)
        {
            tmp = (array[i] << 3) | (array[i + 1] << 2) | (array[i + 2] << 1) | array[i + 3];
            if (tmp<10 && tmp>=0)
                str[cnt] = tmp;
            else
                str[cnt] = tmp-10;
            cnt++;
        }
    }
    return str;
}
static char* array2string(uint16_t* array,int length)
{
    int tmp_length,flag=0,cnt=0;
    if (length%4==0)
        tmp_length = length/4;
    else
    {
        tmp_length = length / 4 + 1;
        flag = 1;
    }
    char* str = new char[tmp_length];
    // char str[28];
    // qDebug() << "Allocated size: " << sizeof(str);
    uint8_t tmp;
    if (flag)
    {
        for (int i = 0; i < length; i+=4)
        {
            if (i > length)break;
            str[cnt] = (array[i] << 4) | (array[i + 1] << 3) | (array[i + 2] << 2) | array[i + 3];
        }
        cnt++;
        int tmp = 0;
        for (int i = 0; i < length % 4; i++)
        {
            tmp |= (array[length-length%4+i] << (4 - i));
        }
        str[cnt-1] = tmp;
    }
    else
    {
        for (int i = 0; i < length; i += 4)
        {
            tmp = (array[i] << 3) | (array[i + 1] << 2) | (array[i + 2] << 1) | array[i + 3];
            if (tmp<10 && tmp>=0)
                str[cnt] = tmp+'0';
            else
                str[cnt] = tmp-10+'A';
            cnt++;

        }

    }
    return str;
}

int mode_s_msg_len_by_type(int type) 
{
    if (type == 16 || type == 17 ||
        type == 19 || type == 20 ||
        type == 21)
        return MODE_S_LONG_MSG_BITS;
    else
        return MODE_S_SHORT_MSG_BITS;
}

uint32_t mode_s_checksum(unsigned char *msg, int bits) {
    uint32_t crc = 0;
    int offset = (bits == 112) ? 0 : (112-56);
    int j;

    for(j = 0; j < bits; j++) 
    {
        int byte = j/8;
        int bit = j%8;
        int bitmask = 1 << (7-bit);

        // If bit is set, xor with corresponding table entry.
        if (msg[byte] & bitmask)
            crc ^= mode_s_checksum_table[j+offset];
    }
    return crc; // 24 bit checksum.
}
