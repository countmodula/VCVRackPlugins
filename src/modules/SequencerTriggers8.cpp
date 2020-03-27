//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 8 Step Sequencer Trigger Expander
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	8

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME SequencerTriggers8
#define WIDGET_NAME SequencerTriggers8Widget
#define MODULE_NAME "SequencerTriggers8"
#define PANEL_FILE "SequencerTriggers8.svg"
#define MODEL_NAME	modelSequencerTriggers8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerTriggers8

#define TRIGGER_OUTPUTS

#include "SequencerGatesSrc.hpp"