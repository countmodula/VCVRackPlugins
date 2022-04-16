//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 8 Step Sequencer Module
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	8

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME Sequencer8
#define WIDGET_NAME Sequencer8Widget
#define MODULE_NAME "Sequencer8"
#define PANEL_FILE "Sequencer8.svg"
#define MODEL_NAME	modelSequencer8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Sequencer8

#include "SequencerSrc.hpp"