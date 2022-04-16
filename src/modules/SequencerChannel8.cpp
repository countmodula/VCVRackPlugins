//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard 8 Step Sequencer Channel
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------

#define SEQ_NUM_STEPS	8

#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerChannelMessage.hpp"

#define STRUCT_NAME SequencerChannel8
#define WIDGET_NAME SequencerChannel8Widget
#define MODULE_NAME "SequencerChannel8"
#define PANEL_FILE "SequencerChannel8.svg"
#define MODEL_NAME	modelSequencerChannel8

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SequencerChannel8

#include "SequencerChannelSrc.hpp"