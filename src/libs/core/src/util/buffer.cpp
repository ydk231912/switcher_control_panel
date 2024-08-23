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
#include <cstdlib>

#include <core/util/buffer.h>

namespace {
    class PlainBuffer : public seeder::core::AbstractBuffer {
    public:
        explicit PlainBuffer(std::size_t _s): data(malloc(_s)), size(_s) {}

        ~PlainBuffer() {
            free(data);
        }

        void *get_data() override {
            return data;
        }

        std::size_t get_size() const override {
            return size;
        }

    private:
        void *data = nullptr;
        std::size_t size = 0;
    };

    class WrapBuffer : public seeder::core::AbstractBuffer {
    public:
        explicit WrapBuffer(void *in_data, std::size_t in_size, const std::function<void(void *)> &in_deleter): 
            data(in_data), size(in_size), deleter(in_deleter)
        {}

        ~WrapBuffer() {
            deleter(data);
        }

        void *get_data() override {
            return data;
        }

        std::size_t get_size() const override {
            return size;
        }

    private:
        void *data = nullptr;
        std::size_t size = 0;
        std::function<void(void *)> deleter;
    };
}

namespace seeder::core
{
    AbstractBuffer::~AbstractBuffer() {}

    std::shared_ptr<AbstractBuffer> alloc_plain_buffer(std::size_t size) {
        return std::make_shared<PlainBuffer>(size);
    }

    std::shared_ptr<AbstractBuffer> wrap_buffer(void *data, std::size_t size, std::function<void(void *)> deleter) {
        return std::make_shared<WrapBuffer>(data, size, deleter);
    }

} // namespace seeder::core