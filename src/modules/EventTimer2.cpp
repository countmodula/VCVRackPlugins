//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Event timer
//	A count down timer to generate events at a specific moment
//	Copyright (C) 2022  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../components/CountModulaLEDDisplay.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

#define STRUCT_NAME EventTimer2
#define WIDGET_NAME EventTimer2Widget
#define MODULE_NAME "EventTimer2"
#define PANEL_FILE "EventTimer2.svg"
#define MODEL_NAME	modelEventTimer2

// set the module name for the theme selection functions
#define THEME_MODULE_NAME EventTimer2

#define NUM_DIGITS 5

#include "EventTimer.hpp"
