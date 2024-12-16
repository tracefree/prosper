#pragma once

template <typename... Types>
struct Signal {
    std::vector<void (*)(Types...)> observers;

    void connect(void (*f)(Types...)) {
        observers.push_back(f);
    }

    void emit(Types... arguments) {
        for (auto f : observers) {
            f(arguments...);
        }
    }
};