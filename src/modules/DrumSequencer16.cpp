//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 12 Channel Drum Sequencer Module
//	Copyright (C) 2023  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME GateSequencer16b
#define WIDGET_NAME GateSequencer16bWidget
#define MODULE_NAME "GateSequencer16b"
#define PANEL_FILE "GateSequencer16b.svg"
#define MODEL_NAME	modelGateSequencer16b

#define GATESEQ_NUM_ROWS	12
#define GATESEQ_NUM_STEPS	16

#define MUTE_BUTTON_STYLE CountModulaLEDPushButtonMini

// set the module name for the theme selection functions
#define THEME_MODULE_NAME GateSequencer16b

// 12 row panel positions
const float ROW_POSITIONS[12] = {
	43.0,
	69.72727,
	96.45454,
	123.18181,
	149.90908,
	176.63635,
	203.36362,
	230.09089,
	256.81816,
	283.54543,
	310.2727,
	337.0
};

#include "GateSequencerSrc.hpp"

