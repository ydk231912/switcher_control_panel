#include <memory>

#include "server/app_ctx.h"

int main(int argc, char **argv) {
    std::unique_ptr<seeder::DvrAppContext> app_ctx(new seeder::DvrAppContext);
    app_ctx->init_args(argc, argv);
    app_ctx->run();
    return 0;
}