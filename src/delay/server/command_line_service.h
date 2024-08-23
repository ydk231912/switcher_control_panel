#pragma once

#include <boost/program_options.hpp>

#include "server/service.h"


namespace seeder {

class CommandLineService : public Service {
public:
    SEEDER_SERVICE_STATIC_ID("command_line");

    void set_args(int in_argc, const char * const *in_argv) {
        argc = in_argc;
        argv = in_argv;
    }

    void start() override {
        boost::program_options::parsed_options parsed = boost::program_options::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        boost::program_options::store(parsed, po_vm);
        boost::program_options::notify(po_vm);
    }

    boost::program_options::options_description & get_desc() {
        return desc;
    }

    template<class T>
    void add_option(const char *name, T &out_value) {
        desc.add_options()(name, boost::program_options::value(&out_value));
    }

private:
    boost::program_options::options_description desc;
    boost::program_options::variables_map po_vm;
    int argc = 0;
    const char * const *argv = nullptr;
};

} // namespace