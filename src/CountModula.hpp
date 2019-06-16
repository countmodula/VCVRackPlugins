#include "rack.hpp"
using namespace rack;


// Forward-declare the Plugin
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelComparator;
extern Model *modelBinarySequencer;
extern Model *modelVCPolarizer;
extern Model *modelMatrixMixer;
extern Model *modelBooleanAND;
extern Model *modelBooleanOR;
extern Model *modelBooleanXOR;
extern Model *modelCVSpreader;
extern Model *modelAnalogueShiftRegister;
extern Model *modelVoltageInverter;
extern Model *modelVCFrequencyDivider;
extern Model *modelEventArranger;
extern Model *modelAttenuator;
extern Model *modelG2T;
extern Model *modelMinimusMaximus;
extern Model *modelVoltageControlledSwitch;
extern Model *modelMultiplexer;
extern Model *modelGateDelay;
extern Model *modelMorphShaper;
extern Model *modelShepardGenerator;
extern Model *modelSRFlipFlop;
extern Model *modelManualCV;
extern Model *modelManualGate;
extern Model *modelGateDelayMT;
extern Model *modelMuteIple;
extern Model *modelPolyrhythmicGenerator;
extern Model *modelBooleanVCNOT;
extern Model *modelBurstGenerator;
extern Model *modelTriggerSequencer16;
extern Model *modelTriggerSequencer8;
extern Model *modelMixer;

#include "components/CountModulaComponents.hpp"
#include "components/StdComponentPositions.hpp"


