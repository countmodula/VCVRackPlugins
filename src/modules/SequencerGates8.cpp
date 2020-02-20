//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 8 Step Sequencer Gate Expander
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	8

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME SequencerGates8
#define WIDGET_NAME SequencerGates8Widget
#define MODULE_NAME "SequencerGates8"
#define PANEL_FILE "SequencerGates8.svg"
#define MODEL_NAME	modelSequencerGates8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerGates8

#include "SequencerGatesSrc.hpp"