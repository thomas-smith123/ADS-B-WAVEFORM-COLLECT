#include <iostream>
#include <string>
#include <bitset>
#include <sstream>
#include <iomanip>
#include <map>

// 将16进制字符串转换为二进制字符串
std::string hexToBinary(const std::string &hex) {
    std::string binary = "";
    for (size_t i = 0; i < hex.length(); ++i) {
        binary += std::bitset<4>(hex[i] > '9' ? hex[i] - 'A' + 10 : hex[i] - '0').to_string();
    }
    return binary;
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

// 解析航班号
std::string decodeFlightNumber(const std::string &msg) {
    std::string flightNumber = "";
    for (int i = 0; i < 8; ++i) {
        int start = 40 + i * 6; // 航班号从第40位开始，每个字符占6位
        int code = std::bitset<6>(msg.substr(start, 6)).to_ulong();
        flightNumber += decodeChar(code);
    }
    return flightNumber;
}

// 解析ICAO地址
std::string decodeIcaoAddress(const std::string &msg) {
    std::stringstream icao;
    icao << std::hex << std::uppercase << std::bitset<24>(msg.substr(8, 24)).to_ulong();
    return icao.str();
}

// 解析DF
int decodeDf(const std::string &msg) {
    return std::bitset<5>(msg.substr(0, 5)).to_ulong();
}

// 解析Transponder capability
std::string decodeTransponderCapability(const std::string &msg) {
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
std::string decodeAircraftStatus(int typeCode) {
    if (typeCode >= 9 && typeCode <= 18) {
        return "空速和垂直速率信息";
    } else if (typeCode >= 19 && typeCode <= 22) {
        return "空速和垂直速率信息";
    } else if (typeCode >= 23 && typeCode <= 27) {
        return "GNSS高度";
    }
    return "未知";
}

// 解析航向
double decodeTrackAngle(const std::string &msg) {
    int track = std::bitset<10>(msg.substr(55, 10)).to_ulong();
    return track * 360.0 / 1024.0;
}

// 解析地速
double decodeGroundSpeed(const std::string &msg) {
    int speed = std::bitset<10>(msg.substr(65, 10)).to_ulong();
    return speed * 1.852; // 换算为公里/小时
}

// 解析高度
int decodeAltitude(const std::string &msg) {
    int qBit = std::bitset<1>(msg.substr(47, 1)).to_ulong();
    if (qBit == 1) {
        int altitude = std::bitset<12>(msg.substr(40, 12)).to_ulong();
        return altitude * 25 - 1000; // 海拔高度（单位：英尺）
    }
    return -1; // Q-bit为0时，高度信息无效
}

// 解析经纬度
std::pair<double, double> decodePosition(const std::string &msg) {
    int latitude = std::bitset<17>(msg.substr(54, 17)).to_ulong();
    int longitude = std::bitset<17>(msg.substr(71, 17)).to_ulong();
    return std::make_pair(latitude * 180.0 / 131072.0 - 90.0, longitude * 360.0 / 131072.0 - 180.0);
}

// 解析航空器识别码
std::string decodeAircraftIdentification(const std::string &msg) {
    std::string aircraftId = "";
    for (int i = 0; i < 8; ++i) {
        int start = 32 + i * 6; // 从第33位开始，每个字符占6位
        int code = std::bitset<6>(msg.substr(start, 6)).to_ulong();
        aircraftId += decodeChar(code);
    }
    return aircraftId;
}

// 解析飞机是否着陆的标志位
bool isAircraftOnGround(const std::string &msg) {
    return std::bitset<1>(msg.substr(53, 1)).to_ulong() == 1;
}

// 解析目标状态和状态信息
std::string decodeTargetStateAndStatus(const std::string &msg) {
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

int main() {
    std::string adsbMessage = "8D789249EA2D0858011C084942F1";
    
    // 将16进制字符串转换为二进制字符串
    std::string binaryMessage = hexToBinary(adsbMessage);
    
    // 提取DF（第1-5位）
    int df = decodeDf(binaryMessage);
    std::cout << "DF: " << df << std::endl;

    // 提取ICAO地址（第9-32位）
    std::string icaoAddress = decodeIcaoAddress(binaryMessage);
    std::cout << "ICAO地址: " << icaoAddress << std::endl;

    // 提取Type Code（第33-37位）
    int typeCode = std::bitset<5>(binaryMessage.substr(32, 5)).to_ulong();
    std::cout << "Type Code: " << typeCode << std::endl;

    // 解析Transponder Capability
    std::string transponderCapability = decodeTransponderCapability(binaryMessage);
    std::cout << "Transponder Capability: " << transponderCapability << std::endl;

    // 解析航班号
    if (typeCode >= 4 && typeCode <= 8) {
        std::string flightNumber = decodeFlightNumber(binaryMessage);
        std::cout << "航班号: " << flightNumber << std::endl;
    }

    // 解析飞行器状态
    std::string aircraftStatus = decodeAircraftStatus(typeCode);
    std::cout << "飞行器状态: " << aircraftStatus << std::endl;

    // 解析航向
    if (typeCode >= 9 && typeCode <= 18) {
        double trackAngle = decodeTrackAngle(binaryMessage);
        std::cout << "航向: " << std::fixed << std::setprecision(2) << trackAngle << "°" << std::endl;
    }

    // 解析地速
    if (typeCode >= 9 && typeCode <= 18) {
        double groundSpeed = decodeGroundSpeed(binaryMessage);
        std::cout << "地速: " << std::fixed << std::setprecision(2) << groundSpeed << " km/h" << std::endl;
    }

    // 解析高度
    if (typeCode >= 9 && typeCode <= 18) {
        int altitude = decodeAltitude(binaryMessage);
        if (altitude != -1) {
            std::cout << "高度: " << altitude << " 英尺" << std::endl;
        } else {
            std::cout << "高度信息无效" << std::endl;
        }
    }

    // 解析经纬度
    if (typeCode >= 9 && typeCode <= 18) {
        auto position = decodePosition(binaryMessage);
        std::cout << "纬度: " << std::fixed << std::setprecision(6) << position.first << "°" << std::endl;
        std::cout << "经度: " << std::fixed << std::setprecision(6) << position.second << "°" << std::endl;
    }

    // 解析航空器识别码
    if (typeCode >= 1 && typeCode <= 4) {
        std::string aircraftId = decodeAircraftIdentification(binaryMessage);
        std::cout << "航空器识别码: " << aircraftId << std::endl;
    }

    // 解析飞机是否着陆的标志位
    if (typeCode >= 9 && typeCode <= 18) {
        bool onGround = isAircraftOnGround(binaryMessage);
        std::cout << "飞机是否着陆: " << (onGround ? "是" : "否") << std::endl;
    }

    // 解析目标状态和状态信息
    if (typeCode >= 29 && typeCode <= 31) {
        std::string targetState = decodeTargetStateAndStatus(binaryMessage);
        std::cout << "目标状态和状态信息: " << targetState << std::endl;
    }

    return 0;
}
