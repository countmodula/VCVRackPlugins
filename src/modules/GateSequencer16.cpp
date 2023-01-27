//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 8 Channel Gate Sequencer Module
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME GateSequencer16
#define WIDGET_NAME GateSequencer16Widget
#define MODULE_NAME "GateSequencer16"
#define PANEL_FILE "GateSequencer16.svg"
#define MODEL_NAME	modelGateSequencer16

#define GATESEQ_NUM_ROWS	8
#define GATESEQ_NUM_STEPS	16

#define SEQUENCER_EXP_NUM_TRIGGER_OUTS 8
#define MUTE_BUTTON_STYLE CountModulaLEDPushButton

#define ROW_POSITIONS STD_ROWS8


// set the module name for the theme selection functions
#define THEME_MODULE_NAME GateSequencer16

#include "GateSequencerSrc.hpp"

