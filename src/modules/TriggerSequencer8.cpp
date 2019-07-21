//----------------------------------------------------------------------------
//	/^M^\ Count Modula - 8 step Trigger Sequencer Module
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SequencerExpanderMessage.hpp"

#define STRUCT_NAME TriggerSequencer8
#define WIDGET_NAME TriggerSequencer8Widget
#define MODULE_NAME "TriggerSequencer8"
#define PANEL_FILE "res/TriggerSequencer8.svg"
#define MODEL_NAME	modelTriggerSequencer8

#define TRIGSEQ_NUM_ROWS	4
#define TRIGSEQ_NUM_STEPS	8


#include "TriggerSequencerSrc.hpp"