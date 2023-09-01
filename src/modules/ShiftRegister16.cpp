//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Shift Register 16
//	Copyright (C) 2023  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define MAX_LEN 16
#define LAST_CELL 15

#define STRUCT_NAME ShiftRegister16
#define WIDGET_NAME ShiftRegister16Widget
#define MODULE_NAME "ShiftRegister16"
#define PANEL_FILE "ShiftRegister16.svg"
#define MODEL_NAME	modelShiftRegister16

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ShiftRegister16

#define ROW_POSITIONS {STD_ROW1, STD_ROW1, STD_ROW2, STD_ROW3, STD_ROW4, STD_ROW5, STD_ROW6, STD_ROW7, STD_ROW8, STD_ROW8, STD_ROW7, STD_ROW6, STD_ROW5, STD_ROW4, STD_ROW3, STD_ROW2};
#define COL_POSITIONS {STD_COL4, STD_COL5, STD_COL5, STD_COL5, STD_COL5, STD_COL5, STD_COL5, STD_COL5, STD_COL5, STD_COL4, STD_COL4, STD_COL4, STD_COL4, STD_COL4, STD_COL4, STD_COL4};


#include "ShiftRegisterSrc.hpp"