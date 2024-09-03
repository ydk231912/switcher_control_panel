#include <iostream>
#include <iomanip>  // 用于 std::hex 和 std::setw
#include <sstream>  // 用于 std::stringstream
#include <vector>

namespace seeder {

// 将 ASCII 字符串转换为十六进制字符串并存储在 std::vector 中
std::vector<std::string> asciiToHex(const std::string& asciiStr) {
    std::vector<std::string> convert_data;  // 创建一个存储转换结果的向量
    std::stringstream ss;
    for (char c : asciiStr) {
        // 将字符转换为对应的十六进制表示
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)c;
    }
    // 将十六进制字符串添加到向量中
    convert_data.push_back(ss.str());
    return convert_data;
}

// 将十六进制字符串转换为 ASCII 字符串并存储在 std::vector 中
std::vector<std::string> hexToAscii(const std::string& hexStr) {
    std::vector<std::string> convert_data;  // 创建一个存储转换结果的向量
    std::string asciiStr;
    for (size_t i = 0; i < hexStr.length(); i += 2) {
        std::string byte = hexStr.substr(i, 2);  // 提取两个十六进制字符
        char c = (char) (int) strtol(byte.c_str(), nullptr, 16);  // 将十六进制字符转换为 ASCII 字符
        asciiStr.push_back(c);  // 将转换后的字符添加到 ASCII 字符串中
    }
    // 将 ASCII 字符串添加到向量中
    convert_data.push_back(asciiStr);
    return convert_data;
}
}// namespace seeder

int main() {
    /*将 ASCII 字符串转换为十六进制字符串*/
    std::string asciiStr = "Hello, World!";  // 示例 ASCII 字符串
    std::vector<std::string> hexVector = seeder::asciiToHex(asciiStr);  // 将 ASCII 字符串转换为十六进制字符串并存储在向量中
    
    // 输出结果
    std::cout << "ASCII_1: " << asciiStr << "\n";
    std::cout << "Hexadecimal_1: ";
    for (const std::string& hexStr : hexVector) {
        std::cout << hexStr;
    }
    std::cout << "\n";

    /*将十六进制字符串转换为 ASCII 字符串*/
     std::string hexStr = "48656c6c6f2c20576f726c6421";  // 示例十六进制字符串（对应 "Hello, World!"）
    std::vector<std::string> asciiVector = seeder::hexToAscii(hexStr);  // 将十六进制字符串转换为 ASCII 字符串并存储在向量中
    
    // 输出结果
    std::cout << "Hexadecimal_2: " << hexStr << "\n";
    std::cout << "ASCII_2: ";
    for (const std::string& asciiStr : asciiVector) {
        std::cout << asciiStr;
    }
    std::cout << "\n";

    return 0;
} // main
