//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 8 Step Sequencer Channel
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	16

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME SequencerChannel16
#define WIDGET_NAME SequencerChannel16Widget
#define MODULE_NAME "SequencerChannel16"
#define PANEL_FILE "SequencerChannel16.svg"
#define MODEL_NAME	modelSequencerChannel16

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerChannel16

#include "SequencerChannelSrc.hpp"