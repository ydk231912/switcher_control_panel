#include "uri.h"

#include <unordered_map>


namespace seeder {

static const std::string schema_sep = "://";
static const std::string param_mark = "?";
static const std::string param_sep = "&";
static const std::string key_value_sep = "=";

// 后面可以改为用glib的GUri或者Boost::url
class UriParser::Impl {
public:
    void init(const std::string &in_str) {
        auto scheme_sep_pos = in_str.find(schema_sep);
        std::size_t start = 0;
        if (scheme_sep_pos != std::string::npos) {
            scheme = in_str.substr(0, scheme_sep_pos);
            start = scheme_sep_pos + schema_sep.size();
        }
        if (start >= in_str.size()) {
            return;
        }
        auto param_mark_pos = in_str.find(param_mark, start);

        if (param_mark_pos == std::string::npos) {
            path = in_str.substr(start);
        } else if (param_mark_pos > start) {
            path = in_str.substr(start, param_mark_pos - start);

            for (std::size_t beg = param_mark_pos + param_mark.size(); beg < in_str.size();) {
                auto param_sep_pos = in_str.find(param_sep, beg);
                if (param_sep_pos == std::string::npos) {
                    parse_query_param(in_str.substr(beg));
                    break;
                } else {
                    parse_query_param(in_str.substr(beg, param_sep_pos - beg));
                    beg = param_sep_pos + param_sep.size();
                }
            }
        }
    }

    void parse_query_param(const std::string &param_item) {
        auto key_value_sep_pos = param_item.find(key_value_sep);
        if (key_value_sep_pos == std::string::npos) {
            return;
        }
        std::string key = param_item.substr(0, key_value_sep_pos);
        std::string value = param_item.substr(key_value_sep_pos + key_value_sep.size());
        query_params[key] = value;
    }

    std::string scheme;
    std::string path;
    std::unordered_map<std::string, std::string> query_params;
};

UriParser::UriParser(const std::string &s): p_impl(new Impl) {
    p_impl->init(s);
}

UriParser::~UriParser() {}

std::string UriParser::peek_scheme(const std::string &s) {
    auto schema_sep_pos = s.find(schema_sep);
    if (schema_sep_pos == std::string::npos) {
        return "";
    }
    return s.substr(0, schema_sep_pos);
}

std::string UriParser::get_scheme() {
    return p_impl->scheme;
}

std::string UriParser::get_path() {
    return p_impl->path;
}

std::string UriParser::get_query_param(const std::string &key) {
    auto iter = p_impl->query_params.find(key);
    if (iter == p_impl->query_params.end()) {
        return "";
    }
    return iter->second;
}


} // namespace seeder
