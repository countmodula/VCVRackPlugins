//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin.
//----------------------------------------------------------------------------
#include "CountModula.hpp"

Plugin *pluginInstance;

void init(Plugin *p) {
	pluginInstance = p;

	// Add all Models defined throughout the plugin
	p->addModel(modelAnalogueShiftRegister);
	p->addModel(modelAttenuator);	
	p->addModel(modelBinarySequencer);
	p->addModel(modelBurstGenerator);
	p->addModel(modelBooleanAND);
	p->addModel(modelBooleanOR);
	p->addModel(modelBooleanVCNOT);
	p->addModel(modelBooleanXOR);
	p->addModel(modelComparator);
	p->addModel(modelCVSpreader);
	p->addModel(modelEventArranger);
	p->addModel(modelG2T);
	p->addModel(modelGateDelay);
	p->addModel(modelGateModifier);
	p->addModel(modelManualCV);
	p->addModel(modelManualGate);
	p->addModel(modelMatrixMixer);
	p->addModel(modelMinimusMaximus);
	p->addModel(modelMixer);
	p->addModel(modelMorphShaper);
	p->addModel(modelMultiplexer);
	p->addModel(modelMute);
	p->addModel(modelMuteIple);
	p->addModel(modelGateDelayMT);
	p->addModel(modelPolyrhythmicGenerator);
	p->addModel(modelRectifier);
	p->addModel(modelSampleAndHold);	
	p->addModel(modelShepardGenerator);
	p->addModel(modelSRFlipFlop);
	p->addModel(modelStepSequencer8);
	p->addModel(modelTFlipFlop);
	p->addModel(modelTriggerSequencer8);
	p->addModel(modelTriggerSequencer16);
	p->addModel(modelVCFrequencyDivider);
	p->addModel(modelVCPolarizer);
	p->addModel(modelVoltageControlledSwitch);
	p->addModel(modelVoltageInverter);
	p->addModel(modelMangler);
	p->addModel(modelBasicSequencer8);
	p->addModel(modelSequencerExpanderCV8);	
	p->addModel(modelSequencerExpanderOut8);
	p->addModel(modelSequencerExpanderTrig8);
	p->addModel(modelSubHarmonicGenerator);
	p->addModel(modelPolyrhythmicGeneratorMkII);
	p->addModel(modelVCFrequencyDividerMkII);
	p->addModel(modelGatedComparator);
	p->addModel(modelSlopeDetector);
	p->addModel(modelSequencerExpanderRM8);
	p->addModel(modelSequencerExpanderLog8);
	// p->addModel(modelVCGateDivider);
	// p->addModel(modelOscilloscope);
	p->addModel(modelStartupDelay);
	p->addModel(modelRackEarLeft);
	p->addModel(modelRackEarRight);
	p->addModel(modelBlank4HP);
	p->addModel(modelBlank8HP);
	p->addModel(modelBlank12HP);
	p->addModel(modelBlank16HP);
	
	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}


// save the given global count modula settings`
void saveSettings(json_t *rootJ) {
	std::string settingsFilename = asset::user("CountModula.json");
	
	FILE *file = fopen(settingsFilename.c_str(), "w");
	
	if (file) {
		json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		fclose(file);
	}
}

// read the global count modula settings
json_t * readSettings() {
	std::string settingsFilename = asset::user("CountModula.json");
	FILE *file = fopen(settingsFilename.c_str(), "r");
	
	if (!file) {
		return json_object();
	}
	
	json_error_t error;
	json_t *rootJ = json_loadf(file, 0, &error);
	
	fclose(file);
	return rootJ;
}

// read the default theme value from the global count modula settings file
int readDefaultTheme() {
	int theme = 0; // default to the standard theme
	
	// read the settings file
	json_t *rootJ = readSettings();
	
	// get the default theme value
	json_t* jsonTheme = json_object_get(rootJ, "DefaultTheme");
	if (jsonTheme)
		theme = json_integer_value(jsonTheme);

	// houskeeping
	json_decref(rootJ);
	
	return theme;
}

// save the given theme value in the global count modula settings file
void saveDefaultTheme(int theme) {
	// read the settings file
	json_t *rootJ = readSettings();
	
	// set the default theme value
	json_object_set_new(rootJ, "DefaultTheme", json_integer(theme));

	// save the updated data
	saveSettings(rootJ);
	
	// houskeeping
	json_decref(rootJ);
}




