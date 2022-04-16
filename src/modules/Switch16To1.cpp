//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 16 to 1 Sequential Switch
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	16

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME Switch16To1
#define WIDGET_NAME Switch16To1Widget
#define MODULE_NAME "Switch16To1"
#define PANEL_FILE "Switch16To1.svg"
#define MODEL_NAME	modelSwitch16To1

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Switch16To1

#define N_TO_ONE

#include "SwitchNToNSrc.hpp"