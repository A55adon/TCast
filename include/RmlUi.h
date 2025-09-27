#pragma once

// Manages initialization, lifetime, and shutdown of RmlUi, Backend, and Shell.
// Provides a single entry point to set up the rendering context and load documents.

#include <RmlUi/Core.h>
#include <RmlUi/Debugger.h>
#include <RmlUi_Backend.h>
#include <Shell.h>

#include "ButtonListener.h"

class RmlUi
{

    public:
        RmlUi();
        ~RmlUi();

        int init();
        void run();


};


