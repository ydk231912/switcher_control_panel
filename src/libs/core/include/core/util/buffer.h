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

#include <memory>
#include <any>
#include <functional>

namespace seeder::core {
class AbstractBuffer {
public:
    virtual void *get_data() = 0;
    virtual std::size_t get_size() const = 0;
    virtual ~AbstractBuffer();

    virtual std::any get_meta() const {
        return std::any();
    }
};

std::shared_ptr<AbstractBuffer> alloc_plain_buffer(std::size_t size);

std::shared_ptr<AbstractBuffer> wrap_buffer(void *data, std::size_t size, std::function<void(void *)> deleter);

}  // namespace seeder::core