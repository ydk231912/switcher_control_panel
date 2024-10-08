#include "ascll_convert_16_demo.h"

namespace seeder {

// 将单个ASCII字符转换为带有0x前缀的16进制字符串
std::string ASCIIConvert16::charToHex(char c) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
    return ss.str();
}

// 将处理后的ASCII字符串转换为16进制字符串数组
std::vector<std::string> ASCIIConvert16::asciiToHex(const std::string& asciiStr) {
    std::vector<std::string> hexArray;

    // 截取字符串，只保留前5位
    std::string truncatedStr = asciiStr;
    
    if(truncatedStr.length() > 5){
        truncatedStr = truncatedStr.substr(0, 1) + truncatedStr.back() + "   ";
    } else {
        truncatedStr.resize(5, ' ');
    }

    // 将处理后的字符串转换为16进制表示
    for (char c : truncatedStr) {
        hexArray.push_back(charToHex(c));
    }

    return hexArray;
}
}// namespace seeder
