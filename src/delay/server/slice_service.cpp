#include "slice_service.h"

#include <string>

#include <boost/filesystem.hpp>

#include "core/util/logger.h"
#include "slice_manager.h"
#include "server/config_service.h"


namespace fs = boost::filesystem;

using seeder::core::logger;

namespace seeder {

namespace {

struct SliceConfig : SliceManager::Config {
    SliceConfig() {
        max_read_slice_count = 60;
    }

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        SliceConfig,
        data_dir,
        max_read_slice_count,
        preopen_read_slice_count,
        preopen_write_slice_count,
        file_mode,
        io_threads
    )
};

} // namespace seeder::{}

class SliceService::Impl {
public:
    SliceConfig slice_config;
    seeder::ConfigCategoryProxy<SliceConfig> config_proxy;
};

SliceService::SliceService(): p_impl(new Impl) {}

SliceService::~SliceService() {}

void SliceService::setup(ServiceSetupCtx &ctx) {
    auto config_service = ctx.get_service<ConfigService>();
    p_impl->config_proxy.init_add(*config_service, "slice");
}

void SliceService::start() {
    p_impl->config_proxy.try_get_to(p_impl->slice_config);
    if (!fs::is_directory(p_impl->slice_config.data_dir)) {
        logger->error("slice directory data_dir={}", p_impl->slice_config.data_dir);
        throw std::runtime_error("slice.data_dir");
    }
}

std::shared_ptr<SliceManager> SliceService::create_slice_manager(
    size_t in_slice_block_size,
    size_t in_slice_block_max_count
) {
    auto config = p_impl->slice_config;
    config.slice_file_prefix = "map";
    config.slice_first_block_offset = 0;
    config.slice_block_size = in_slice_block_size;
    config.slice_block_max_count = in_slice_block_max_count;
    std::shared_ptr<SliceManager> slice_manager(new SliceManager(config));
    slice_manager->init();
    return slice_manager;
}

int SliceService::get_max_read_slice_count() const noexcept {
    return p_impl->slice_config.max_read_slice_count;
}


} // namespace seeder