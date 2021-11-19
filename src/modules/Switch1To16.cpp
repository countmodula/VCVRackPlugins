//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 1 to 16 Sequential Switch
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	16

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME Switch1To16
#define WIDGET_NAME Switch1To16Widget
#define MODULE_NAME "Switch1To16"
#define PANEL_FILE "Switch1To16.svg"
#define MODEL_NAME	modelSwitch1To16

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Switch1To16

#define ONE_TO_N

#include "SwitchNToNSrc.hpp"