#include "switcher.h"
#include "uart_can_all.h"
#include <boost/program_options.hpp>
#include <cstdio>

void panel_status_handler(int pgm, int pvw) {
  printf("receive: pgm %d, pvw %d\n", pgm, pvw);
}
void dsk_status_handler(std::vector<bool> dsks) {
  for (int i = 0; i < dsks.size(); i++) {
    printf("dsk[%i]: %s \n", i, dsks[i] ? "true" : "false");
  }
}

void transition_tyep_handler(std::string type) {
  printf("transition_type: %s \n", type.c_str());
}
int main(int argc, char **argv) {
  // 读取配置文件
  namespace po = boost::program_options;
  std::string config_path;
  po::options_description desc("Seeder Switcher Keybaord");
  desc.add_options()("config_file", po::value(&config_path)->required(),
                     "main config file");
  po::parsed_options parsed =
      po::command_line_parser(argc, argv).options(desc).run();
  po::variables_map po_vm;
  po::store(parsed, po_vm);
  po::notify(po_vm);

  seeder::Switcher switcher(config_path);
  switcher.init();
  switcher.set_panel_status_handler(panel_status_handler);
  switcher.set_dsk_status_handler(dsk_status_handler);
  switcher.set_transition_type_handler(transition_tyep_handler);
  switcher.pgm(1);
  switcher.pvw(1);
  while (true) {
  }
}