//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 16 Step Sequencer Gate Expander
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	16

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME SequencerGates16
#define WIDGET_NAME SequencerGates16Widget
#define MODULE_NAME "SequencerGates16"
#define PANEL_FILE "SequencerGates16.svg"
#define MODEL_NAME	modelSequencerGates16

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerGates16

#include "SequencerGatesSrc.hpp"