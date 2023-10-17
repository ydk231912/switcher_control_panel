#pragma once

#include <memory>
#include <string>
#include <vector>
#include "util.h"

namespace seeder {

namespace decklink {


class device_status {
public:
    int index;
    std::string display_name; // IDeckLink::GetDisplayName
    int64_t persistent_id; // BMDDeckLinkPersistentID 
    std::string device_label; // bmdDeckLinkConfigDeviceInformationLabel 
    bool support_format_dection = false;
};


class device_manager {
public:
    device_manager();
    ~device_manager();

    static device_manager & instance();

    const std::vector<device_status> & get_device_status();

    // 返回的 IDeckLink * 不需要Release()，统一由device_manager管理
    IDeckLink * get_decklink(int index);

    void set_id_map(const std::string &_id_map);
    void set_id_map(const std::vector<int> &_id_map);

    // non copy, non move
    device_manager(const device_manager &) = delete;
    device_manager & operator=(const device_manager &) = delete;
private:
    class impl;
    std::unique_ptr<impl> p_impl;
};


} // namespace decklink

} // namespace seeder