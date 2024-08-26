#pragma once

#include <memory>

#include "server/service.h"

namespace seeder {

class SliceManager;

class SliceService : public Service {
public:
    explicit SliceService();
    ~SliceService();

    SEEDER_SERVICE_STATIC_ID("slice");

    void setup(ServiceSetupCtx &ctx) override;

    void start() override;

    std::shared_ptr<SliceManager> create_slice_manager(
        size_t in_slice_block_size,
        size_t in_slice_block_max_count
    );

    int get_max_read_slice_count() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder