//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 16 Step Sequencer Channel
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	16 

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME Sequencer16
#define WIDGET_NAME Sequencer16Widget
#define MODULE_NAME "Sequencer16"
#define PANEL_FILE "Sequencer16.svg"
#define MODEL_NAME	modelSequencer16

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Sequencer16

#include "SequencerSrc.hpp"