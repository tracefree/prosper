#pragma once

#include <unordered_map>
#include <memory>

struct ResourceManager {
private:
    template<typename T>
    inline static std::unordered_map<std::string, std::shared_ptr<Resource<T>>> resources;

public:
    template<typename T>
    static std::shared_ptr<Resource<T>> get(const char* p_guid) {
        if (!resources<T>.contains(p_guid)) {
            resources<T>[p_guid] = std::make_shared<Resource<T>>();
            resources<T>[p_guid]->guid = p_guid;
        }
        return resources<T>[p_guid];
    }

    template<typename T>
    static void set(const char* p_guid, T p_value) {
        resources<T>[p_guid] = p_value;
    }

    template<typename T, typename... Ts>
    static void load(const char* p_guid, Ts... p_arguments);

    template<typename T>
    static void save(const char* p_guid);

    template<typename T>
    static void unload(const char* p_guid);
};