#pragma once

#include <RmlUi/Core.h>
#include <functional>

class ButtonHandler final : public Rml::EventListener {
    std::function<void()> callback;
    bool stopPropagation;
public:
    explicit ButtonHandler(std::function<void()> callback, bool stopPropagation = false)
        : callback(std::move(callback)), stopPropagation(stopPropagation) {}

    void ProcessEvent(Rml::Event& event) override {
        if (stopPropagation) {
            event.StopPropagation();
        }
        callback();
    }
};

class KeyEventHandler : public Rml::EventListener {
    std::function<void(Rml::Event&)> callback;
public:
    explicit KeyEventHandler(std::function<void(Rml::Event&)> callback)
        : callback(std::move(callback)) {}

    void ProcessEvent(Rml::Event& event) override {
        callback(event);
    }
};