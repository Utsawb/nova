#pragma once

#ifndef GUI_HH
#define GUI_HH

#include "pch.hh"

#include "ParameterStore.hh"

#include "CascadiaCode.ttf.h"

class GUI
{
    private:
        ParameterStore &parameter_store;

    public:
        GUI(ParameterStore &parameter_store) : parameter_store(parameter_store)
        {

        }

        ~GUI()
        {
        }

        void event_handler(SDL_Event *event)
        {
        }

        void render_frame()
        {
        }
};

#endif // GUI_HH