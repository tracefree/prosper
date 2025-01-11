#pragma once
#include <functional>
#include <list>

template<typename T, typename... U>
size_t get_address(std::function<T(U...)> f) {
    typedef T(fnType)(U...);
    fnType** fnPointer = f.template target<fnType*>();
    return (size_t) *fnPointer;
}


template <typename... Types>
struct Signal {
    std::vector<void (*)(Types...)> ptr_observers;
    std::vector<std::function<void(Types...)>> fn_observers;

    void connect(void (*f)(Types...)) {
        ptr_observers.push_back(f);
    }

    std::vector<std::function<void(Types...)>>::iterator
    connect(std::function<void(Types...)>& f) {
        fn_observers.push_back(f);
        return fn_observers.end();
    }

    void disconnect(std::vector<std::function<void(Types...)>>::iterator handle) {
        fn_observers.erase(handle);
    }
    
    void emit(Types... arguments) {
        for (auto f : ptr_observers) {
            f(arguments...);
        }
        for (auto f : fn_observers) {
            f(arguments...);
        }
    }
};