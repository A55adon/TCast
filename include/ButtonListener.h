#pragma once

#include <RmlUi/Core.h>
#include <functional>

class ButtonHandler final : public Rml::EventListener {
public:
    explicit ButtonHandler(std::function<void()> callback)
        : callback(std::move(callback)) {}

    void ProcessEvent(Rml::Event& event) override {
        if (event.GetId() == Rml::EventId::Click) {
            if (callback) callback();
        }
    }

private:
    std::function<void()> callback;
};