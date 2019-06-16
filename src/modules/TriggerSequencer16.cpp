//----------------------------------------------------------------------------
//	Count Modula - 16 Step Trigger Sequencer Module
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

#define STRUCT_NAME TriggerSequencer16
#define WIDGET_NAME TriggerSequencer16Widget
#define MODULE_NAME "TriggerSequencer16"
#define PANEL_FILE "res/TriggerSequencer16.svg"
#define MODEL_NAME	modelTriggerSequencer16

#define TRIGSEQ_NUM_ROWS	4
#define TRIGSEQ_NUM_STEPS	16 

#include "TriggerSequencerSrc.hpp"