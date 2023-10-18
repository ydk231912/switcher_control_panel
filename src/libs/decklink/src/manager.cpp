#include "decklink/manager.h"
#include "core/util/logger.h"

#include <stdexcept>
#include <unordered_map>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/lexical_cast.hpp>

#include <spdlog/fmt/ranges.h>

namespace seeder {

namespace decklink {


class device_manager::impl {
    friend class device_manager;
public:
    impl() {
        boost::intrusive_ptr<IDeckLinkIterator> decklink_interator {CreateDeckLinkIteratorInstance(), false};
        if (!decklink_interator) {
            throw std::runtime_error("CreateDeckLinkIteratorInstance failed");
        }

        {
            IDeckLink* decklink;
            HRESULT result;
            while((result = decklink_interator->Next(&decklink)) == S_OK)
            {
                decklink_list.emplace_back(decklink, false);
            }
        }
        

        for (int i = 0; i < decklink_list.size(); ++i) {
            auto &decklink = decklink_list[i];
            device_status &device_status = device_status_list.emplace_back();
            device_status.index = i;
            const char *device_name_str;
            if (decklink->GetDisplayName(&device_name_str) == S_OK && device_name_str) {
                device_status.display_name = device_name_str;
                free((void*)device_name_str);
            } else {
                device_status.display_name = "DeckLink";
            }

            IDeckLinkConfiguration *p_decklink_configuration;
            if (decklink->QueryInterface(IID_IDeckLinkConfiguration, (void**)&p_decklink_configuration) != S_OK)
            {
                throw std::runtime_error("QueryInterface IID_IDeckLinkConfiguration failed");
            }
            auto &decklink_configuration = decklink_configuration_list.emplace_back(p_decklink_configuration, false);

            IDeckLinkProfileAttributes *p_decklink_attributes;
            if (decklink->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&p_decklink_attributes) != S_OK)
            {
                throw std::runtime_error("QueryInterface IID_IDeckLinkProfileAttributes failed");
            }
            auto &decklink_attributes = decklink_attributes_list.emplace_back(p_decklink_attributes, false);
            bool support_format_dection = false;
            decklink_attributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &support_format_dection);
            device_status.support_format_dection = support_format_dection;
            if (decklink_attributes->GetInt(BMDDeckLinkPersistentID, &device_status.persistent_id) == S_OK) {
                const char *deviceLabelStr;

                if (decklink_configuration->GetString(bmdDeckLinkConfigDeviceInformationLabel, &deviceLabelStr) == S_OK && deviceLabelStr)
                {
                    device_status.device_label = deviceLabelStr;
                    free((void *) deviceLabelStr);
                }
            }
        }
    }

private:
    std::vector<boost::intrusive_ptr<IDeckLink>> decklink_list;
    std::vector<boost::intrusive_ptr<IDeckLinkConfiguration>> decklink_configuration_list;
    std::vector<boost::intrusive_ptr<IDeckLinkProfileAttributes>> decklink_attributes_list;
    std::vector<device_status> device_status_list;
    std::unordered_map<std::size_t, std::size_t> id_map;
};


device_manager::device_manager(): p_impl(new impl) {}

device_manager::~device_manager() {}

device_manager & device_manager::instance() {
    static device_manager manager;
    return manager;
}

const std::vector<device_status> & device_manager::get_device_status() {
    return p_impl->device_status_list;
}

IDeckLink * device_manager::get_decklink(int index) {
    if (auto it = p_impl->id_map.find(index); it != p_impl->id_map.end()) {
        index = it->second;
    }
    if (index >= 0 && index < p_impl->decklink_list.size()) {
        return p_impl->decklink_list[index].get();
    }
    return nullptr;
}

void device_manager::set_id_map(const std::string &_id_map) {
    if (_id_map.empty()) {
        return;
    }
    std::vector<std::string> ids;
    boost::split(ids, _id_map, boost::is_any_of(","));
    for (std::size_t i = 0; i < ids.size(); ++i) {
        p_impl->id_map[i] = boost::lexical_cast<std::size_t>(ids[i]) - 1;
    }
    seeder::core::logger->info("decklink id_map={}", p_impl->id_map);
}

void device_manager::set_id_map(const std::vector<int> &_id_map) {
    if (_id_map.size() != p_impl->decklink_list.size()) {
        seeder::core::logger->error("seeder::decklink::device_manager::set_id_map decklink_list size mismatch!");
        throw std::runtime_error("device_manager::set_id_map");
    }
    for (std::size_t i = 0; i < _id_map.size(); ++i) {
        p_impl->id_map[i] = _id_map[i] - 1;
    }
}



} // namespace decklink

} // namespace seeder