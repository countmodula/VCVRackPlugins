//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 8 to 1 Sequential Switch
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	8

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME Switch8To1
#define WIDGET_NAME Switch8To1Widget
#define MODULE_NAME "Switch8To1"
#define PANEL_FILE "Switch8To1.svg"
#define MODEL_NAME	modelSwitch8To1

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Switch8To1

#define N_TO_ONE

#include "SwitchNToNSrc.hpp"