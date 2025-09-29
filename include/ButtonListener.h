#pragma once

#include <RmlUi/Core.h>
#include <functional>
#include <iostream>

class ButtonHandler : public Rml::EventListener {
public:
    ButtonHandler(std::function<void()> callback)
        : callback(std::move(callback)) {}

    void ProcessEvent(Rml::Event& event) override {
        if (event.GetId() == Rml::EventId::Click) {
            if (callback) callback();
        }
    }

private:
    std::function<void()> callback;
};