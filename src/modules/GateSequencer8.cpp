//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 8 Channel Gate Sequencer Module
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME GateSequencer8
#define WIDGET_NAME GateSequencer8Widget
#define MODULE_NAME "GateSequencer8"
#define PANEL_FILE "GateSequencer8.svg"
#define MODEL_NAME	modelGateSequencer8

#define GATESEQ_NUM_ROWS	8
#define GATESEQ_NUM_STEPS	8

#define SEQUENCER_EXP_NUM_TRIGGER_OUTS 8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME GateSequencer8

#include "GateSequencerSrc.hpp"

