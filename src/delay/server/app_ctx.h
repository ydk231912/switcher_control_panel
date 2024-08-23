#pragma once

#include <memory>


namespace seeder {

class DvrAppContext {
public:
    explicit DvrAppContext();
    ~DvrAppContext();

    void init_args(int argc, char **argv);

    void run();

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

} // namespace seeder