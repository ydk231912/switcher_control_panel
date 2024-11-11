#pragma once

#include <iostream>
#include <iomanip>  // 用于 std::hex 和 std::setw
#include <sstream>  // 用于 std::stringstream
#include <vector>
#include <string>
namespace seeder{

class ASCIIConvert16{
public:
    std::string charToHex(char c);
    std::vector<std::string> asciiToHex(const std::string& asciiStr);
private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
}; //class ASCLLConvert16Demo
} // namespace seeder