//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - 16 Step Trigger Sequencer Module
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME TriggerSequencer16
#define WIDGET_NAME TriggerSequencer16Widget
#define MODULE_NAME "TriggerSequencer16"
#define PANEL_FILE "TriggerSequencer16.svg"
#define MODEL_NAME	modelTriggerSequencer16

#define TRIGSEQ_NUM_ROWS	4
#define TRIGSEQ_NUM_STEPS	16 

// set the module name for the theme selection functions
#define THEME_MODULE_NAME TriggerSequencer16

#include "TriggerSequencerSrc.hpp"