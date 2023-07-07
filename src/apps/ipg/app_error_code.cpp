#include "app_error_code.h"
#include <string>

const char* st_app_error_category::name() const noexcept {
    return "st_app_error";
}

std::string st_app_error_category::message(int ev) const {
    std::string r;
    r.append("code=");
    r.append(std::to_string(ev));
    return r;
}