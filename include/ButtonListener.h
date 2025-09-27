#pragma once

#include <RmlUi/Core.h>
#include <iostream>

class ButtonHandler : public Rml::EventListener {
public:
    void ProcessEvent(Rml::Event& event) override {
        if (event.GetId() == Rml::EventId::Click) {
            std::cout << "sum" << std::endl;
        }
    }
};