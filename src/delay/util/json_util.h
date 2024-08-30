#pragma once

#include <optional>

#include <boost/lexical_cast.hpp>

#include <nlohmann/json.hpp>


NLOHMANN_JSON_NAMESPACE_BEGIN
template <typename T>
struct adl_serializer<std::optional<T>> {
    static void to_json(json &j, const std::optional<T> &opt) {
        if (opt == std::nullopt) {
            j = nullptr;
        } else {
            j = *opt;
        }
    }

    static void from_json(const json &j, std::optional<T> &value) {
        if (j.is_null()) {
            value = std::nullopt;
        } else {
            value = j.template get<T>();
        }
    }
};
NLOHMANN_JSON_NAMESPACE_END

namespace seeder {

namespace json {

template<class T>
struct MaybeString {
    MaybeString() {}
    MaybeString(const T &in_value): value(in_value) {}

    operator T() const {
        return value;
    }

    friend inline void to_json(nlohmann::json &j, const MaybeString &value) {
        j = value.value;
    }

    friend inline void from_json(const nlohmann::json &j, MaybeString &value) {
        if (j.is_string()) {
            boost::conversion::try_lexical_convert(j.get_ref<const std::string &>(), value.value);
        } else {
            value.value = j;
        }
    }

    T value;
};

template<class T>
struct BasePropsWrapper {
    T base;
    nlohmann::json props;
};

template<class T>
inline void to_json(nlohmann::json &j, const BasePropsWrapper<T> &value) {
    j = value.props;
    nlohmann::json base_j = value.base;
    j.update(base_j);
}

template<class T>
inline void from_json(const nlohmann::json &j, BasePropsWrapper<T> &value) {
    value.base = j;
    value.props = j;
}

template<class T, class S>
inline auto object_safe_get(const nlohmann::json &j, S &&key, T &&default_value) -> decltype(j.value(std::forward<S>(key), default_value)) {
    using ReturnType = decltype(j.value(std::forward<S>(key), default_value));
    if (!j.is_object()) {
        return std::forward<T>(default_value);
    }
    auto iter = j.find(std::forward<S>(key));
    if (iter == j.end()) {
        return std::forward<T>(default_value);
    }
    try {
        return iter->template get<ReturnType>();
    } catch (nlohmann::json::exception &e) {
        return std::forward<T>(default_value);
    }
}

template<class T, class S>
inline void object_safe_get_to(const nlohmann::json &j, S &&key, T &out_value) {
    if (!j.is_object()) {
        return;
    }
    auto iter = j.find(std::forward<S>(key));
    if (iter == j.end()) {
        return;
    }
    try {
        iter->get_to(out_value);
    } catch (nlohmann::json::exception &e) {
        return;
    }
}

template<class S, class F>
inline bool visit(const nlohmann::json &j, const S &key, F &&f) {
    if (j.contains(key)) {
        f(j[key]);
        return true;
    }
    return false;
}

template<class S>
inline bool is_not_null(const nlohmann::json &j, const S &key) {
    if (j.is_object()) {
        return false;
    }
    auto iter = j.find(key);
    if (iter == j.end()) {
        return false;
    }
    return !iter->is_null();
}

} // namespace seeder::json

} // namespace seeder