#pragma once

#include <memory>
#include <unordered_map>
#include <string>
#include <print>

enum class LoadStatus {
    UNLOADED,
    LOADING,
    LOADED,
};

template <typename T>
struct Resource {
private:
    T* pointer;
    uint reference_count {0};
    LoadStatus load_status { LoadStatus::UNLOADED };

public:
    static std::unordered_map<std::string, Resource<T>> resources;
    std::string guid;

    void reference() {
        reference_count += 1;
        std::println("REFERENCE+: {}, {}", reference_count, guid);
    }

    void unreference() {
        if (reference_count == 0) {
            std::println("UNREFERENCING DELETED: {}", guid);
        }
        reference_count -= 1;
        std::println("REFERENCE-: {}, {}", reference_count, guid);
        if (reference_count == 0) {
            unload();
            delete pointer;
            pointer = nullptr;
        }
    }

    void operator=(const Resource<T>& p_resource) {
        pointer = p_resource.pointer;
        load_status = p_resource.load_status;
        guid = p_resource.guid;
        reference_count = p_resource.reference_count;
    }

    T& operator*() {
        return *pointer;
    }

    T* operator->() {
        return pointer;
    }

    Resource() {
        pointer = new T;
    }

    Resource(std::string p_guid) {
        guid = p_guid;
        pointer = new T;
    }

    Resource(T p_resource) {
        pointer = &p_resource;
    }

    Resource(T* p_resource) {
        pointer = p_resource;
    }

    Resource(const Resource<T>& p_resource) {
        pointer = p_resource.pointer;
        load_status = p_resource.load_status;
        guid = p_resource.guid;
        reference_count = p_resource.reference_count;
    }

    bool loaded() {
        return (load_status == LoadStatus::LOADED);
    }

    void set_load_status(LoadStatus p_load_status) {
        load_status = p_load_status;
    }

    static Resource<T>& get(std::string p_guid) {
        auto resource = Resource<T>::resources[p_guid];
        Resource<T>::resources[p_guid].guid = p_guid;
        return Resource<T>::resources[p_guid];
    }

    template<typename... Ts>
    static Resource<T>& load(std::string p_guid, Ts... arguments);
    void unload() {};
};

template <typename T>
std::unordered_map<std::string, Resource<T>> Resource<T>::resources;