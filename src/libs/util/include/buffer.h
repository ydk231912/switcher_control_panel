/**
 * @file buffer.h
 * @author 
 * @brief memory buffer class, allocate memory at construction, 
 *   automatic release memory at destruction
 * @version 
 * @date 2022-05-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

namespace seeder::util
{
    class buffer
    {
      public:
        buffer();
        buffer(int size);
        ~buffer();

        uint8_t * begin();
        uint8_t * end();
        int size();

      private:
        int size_ = 0;
        int leng = 0;
        uint8_t * data_ = nullptr;
    };
} // namespace seeder::util