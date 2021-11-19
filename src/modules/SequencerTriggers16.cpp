//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 16 Step Sequencer Gate Expander
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	16

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME SequencerTriggers16
#define WIDGET_NAME SequencerTriggers16Widget
#define MODULE_NAME "SequencerTriggers16"
#define PANEL_FILE "SequencerTriggers16.svg"
#define MODEL_NAME	modelSequencerTriggers16

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerTriggers16

#define TRIGGER_OUTPUTS

#include "SequencerGatesSrc.hpp"