#pragma once

#ifndef OS_H
#define OS_H

// 操作系统相关接口

namespace seeder {

namespace util {


// 重启计算机
void restart();
// 关机
void poweroff();

} // namespace seeder::util

} // namespace seeder
#endif // OS_H