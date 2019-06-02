//----------------------------------------------------------------------------
//	Count Modula - Morph Shaper
//	A CV controlled morphing controller based on the Doepfer A-144
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "dsp/digital.hpp"

struct MorphShaper : Module {
	enum ParamIds {
		CV_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(MORPH_OUTPUT, 4),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(MORPH_LIGHT, 4),
		NUM_LIGHTS
	};

	float outVal;
	
#ifdef NOT_FULL_SCALE
	// thresholds for increasing output
	float pMap[4][2] = {
		{0.5f, 2.3f},
		{2.3f, 4.1f},
		{4.1f, 5.9f},
		{5.9f, 7.7f}
	};
	
	// thresholds for decreasing output
	float nMap[4][2] = {	
		{2.3f, 4.1f},
		{4.1f, 5.9f},
		{5.9f, 7.7f},
		{7.7f, 9.5f}
	};	
	
	const float span = 1.8f;
	const float scale = 10.0f/span;
	
#else
	// thresholds for increasing output
	float pMap[4][2] = {
		{0.0f, 2.0f},
		{2.0f, 4.0f},
		{4.0f, 6.0f},
		{6.0f, 8.0f}
	};
	
	// thresholds for decreasing output
	float nMap[4][2] = {	
		{2.0f, 4.0f},
		{4.0f, 6.0f},
		{6.0f, 8.0f},
		{8.0f, 10.0f}
	};	
	
	const float span = 2.0f;
	const float scale = 10.0f / span;	
#endif
	
	MorphShaper() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void MorphShaper::step() {
	
	// calculate the current morph control value
	float morphVal = clamp(params[MANUAL_PARAM].value + (inputs[CV_INPUT].value * params[CV_PARAM].value), 0.0f, 10.0f);
		
	// reset all outputs
	for (int i = 0; i < 4; i++)
	{
		if (morphVal >= pMap[i][0] && morphVal < pMap[i][1])
			outVal = morphVal - pMap[i][0];
		else if (morphVal >= nMap[i][0] && morphVal < nMap[i][1])
			outVal = nMap[i][0] + span - morphVal;
		else
			outVal = 0.0f;
		
		outputs[MORPH_OUTPUT + i].value = outVal * scale;
		lights[MORPH_LIGHT + i].setBrightnessSmooth(outVal / 10.0f);
	}
}

struct MorphShaperWidget : ModuleWidget {
MorphShaperWidget(MorphShaper *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/MorphShaper.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, MorphShaper::CV_PARAM, -1.0f, 1.0f, 0.0f));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, MorphShaper::MANUAL_PARAM, 0.0f, 10.0f, 0.0f));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, MorphShaper::CV_INPUT));
		
		// outputs/lights
		for (int i = 0; i < 4 ; i++) {
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + i]), module, MorphShaper::MORPH_OUTPUT +i));	
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3 + i]), module, MorphShaper::MORPH_LIGHT + i));
		}
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelMorphShaper = Model::create<MorphShaper, MorphShaperWidget>("Count Modula", "MorphShaper", "Morph Shaper", UTILITY_TAG, WAVESHAPER_TAG);
