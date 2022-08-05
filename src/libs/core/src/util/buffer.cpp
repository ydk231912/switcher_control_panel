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

#include <memory>

#include "core/util/buffer.h"

namespace seeder::core
{
    buffer::buffer(int size)
    {
        data_ = (uint8_t*)malloc(size);
        size_ = size;
    }

    buffer::~buffer()
    {
        // if(data_)
        //     free(data_);
    }

    void buffer::free_buffer()
    {
        if(data_)
            free(data_);
    }

    uint8_t * buffer::begin()
    {
        return data_;
    }

    uint8_t * buffer::end()
    {
        return data_ + size_ - 1;
    }

    int buffer::size()
    {
        return size_;
    }
} // namespace seeder::core