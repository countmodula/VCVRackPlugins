//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Super Sample & Hold Module
//	Sample/Track/Pass and Hold with probability
//  Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SampleAndHold2
#define PANEL_FILE "SampleAndHold2.svg"

struct SampleAndHold2 : Module {
	enum ParamIds {
		MODE_PARAM,
		PROB_PARAM,
		PROB_CV_PARAM,
		LEVEL_PARM,
		OFFSET_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SAMPLE_INPUT,
		TRIG_INPUT,
		MODE_INPUT,
		PROB_INPUT,
		OFFSET_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SAMPLE_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		TRACK_LIGHT,
		SAMPLE_LIGHT,
		PASS_LIGHT,
		NUM_LIGHTS
	};

	enum Modes {
		SAMPLE,
		TRACK,
		PASS
	};
	
	GateProcessor gateTrig[16];
	int processCount = 8;
	int trackMode = SAMPLE;
	float probability = 100.0f;
	float probabilityCV = 100.0f;
	bool doSample[16] = {};
	bool forceSample = true;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SampleAndHold2() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		// tracking mode switch
		configSwitch(MODE_PARAM, 0.0f, 2.0f, 0.0f, "Hold mode",{"Sample & Hold", "Through", "Track & Hold"});

		// knobs
		configParam(PROB_PARAM, 0.0f, 1.0f, 1.0f, "Probability", " %", 0.0f, 100.0f, 0.0f);
		configParam(PROB_CV_PARAM, -1.0f, 1.0f, 0.0f, "Probability CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(LEVEL_PARM, 0.0f, 1.0f, 1.0f, "Input level", " %", 0.0f, 100.0f, 0.0f);
		configParam(OFFSET_PARAM, -1.0f, 1.0f, 0.0f, "Offset amount", " V", 0.0f, 10.0f, 0.0f);
		
		configInput(SAMPLE_INPUT, "Signal");
		configInput(TRIG_INPUT, "Trigger");
		configInput(MODE_INPUT, "Hold mode CV");
		configInput(PROB_INPUT, "Probability CV");
		configInput(OFFSET_INPUT, "Offset CV");
		
		configOutput(SAMPLE_OUTPUT, "Sampled signal");
		configOutput(INV_OUTPUT, "Inverted sampled signal");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		processCount = 8;
		trackMode = SAMPLE;
		probability = 1.0f;
		probabilityCV = 1.0f;
		forceSample = true;
	}
	
	void onReset() override {
		for(int i = 0; i < 16; i++) {
			gateTrig[i].reset();
			doSample[i] = false;
		}
			
		processCount = 8;
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(3));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"
		
		json_t *samp = json_array();
		for(int i = 0; i < 16; i++) {
			json_array_insert_new(samp, i, json_boolean(doSample[i]));
		}

		json_object_set_new(root, "sample", samp);
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
		
		json_t *samp = json_object_get(root, "sample");
		
		for(int i = 0; i < 16; i++) {
			if (samp) {
				json_t *v = json_array_get(samp, i);
				if (v)
					doSample[i] = json_boolean_value(v);
			}
		}
	}	
	
	void process(const ProcessArgs &args) override {

		// determine the mode - input takes precedence over switch
		if (++processCount > 8) {
			processCount = 0;

			int prevTrackMode = trackMode;
			
			trackMode = SAMPLE;
			if (inputs[MODE_INPUT].isConnected()) {
				trackMode =  (int)(clamp(inputs[MODE_INPUT].getVoltage(), 0.0f, 5.0f)) / 2;
			}
			else
				trackMode = (int)(params[MODE_PARAM].getValue());
				
			lights[PASS_LIGHT].setBrightness(boolToLight(trackMode == PASS));
			lights[TRACK_LIGHT].setBrightness(boolToLight(trackMode == TRACK));
			lights[SAMPLE_LIGHT].setBrightness(boolToLight(trackMode == SAMPLE));
			
			probability = params[PROB_PARAM].getValue();
			probabilityCV = params[PROB_CV_PARAM].getValue();
			
			if (prevTrackMode != trackMode)
				forceSample = true;
		}

		// do the sample and hold
		int numTrigs = clamp(inputs[TRIG_INPUT].getChannels(), 0, 16);
		if (numTrigs > 0)
		{
			float level = params[LEVEL_PARM].getValue();
			float offset = params[OFFSET_PARAM].getValue();
			
			// determine number of output channels
			// if num trigs is 1 it is applied to all input channels and number of output channels = number of input channels
			// if num trigs is > 1 and input channels is >= num trigs then number of output channels = number of input channels
			// if num trigs is > 1 and input channels is < num trigs then number of output channels = number of trig channels and randoms are used for the missing channels
			int inputChannels = clamp(inputs[SAMPLE_INPUT].getChannels(), 0, numTrigs > 1 ? numTrigs : 16);
			int channelsToUse = std::max(numTrigs, inputChannels);
			
			outputs[SAMPLE_OUTPUT].setChannels(channelsToUse);
			outputs[INV_OUTPUT].setChannels(channelsToUse);

			// handle single trigger here
			int t = 0;
			if (numTrigs == 1) {
				gateTrig[0].set(inputs[TRIG_INPUT].getVoltage());
				t = 0;
			}
			
			// now sample away
			float s;
			bool getProb = inputs[PROB_INPUT].isConnected();
			float threshold = probability;
			
			for (int c = 0; c < 16; c++) {
				if (c < channelsToUse) {
					if (numTrigs > 1) {
						gateTrig[c].set(inputs[TRIG_INPUT].getVoltage(c));
						t = c;
					}

					// probability check goes here
					if (forceSample || gateTrig[t].anyEdge()) {
						float r = random::uniform();
						if(getProb)
							threshold = clamp(probability + (inputs[PROB_INPUT].getPolyVoltage(c) * probabilityCV / 10.f), 0.f, 1.f);

						if (r < threshold) {
							switch (trackMode) {
								case TRACK:
									doSample[c] = gateTrig[t].high();
									break;
								case SAMPLE:
									doSample[c] = gateTrig[t].leadingEdge();
									break;
								case PASS:
									doSample[c] = !gateTrig[t].high();
									break;
							}
						}
					}

					if (doSample[c]) {
						// track, pass  or sample the input
						float offsetVoltage = offset * inputs[OFFSET_INPUT].getNormalPolyVoltage(10.0f, c);
						float v = c >= inputChannels ? random::uniform() * 10.0f - 5.0f : inputs[SAMPLE_INPUT].getVoltage(c);
						// todo: saturate rather than clamp
						s = clamp (v * level + offsetVoltage, -12.0f, 12.0f);
						
						if (trackMode == SAMPLE)
							doSample[c] = false;
					}
					else {
						// hold the last sample value
						s = outputs[SAMPLE_OUTPUT].getVoltage(c);
					}
					
					// set the output voltages
					outputs[SAMPLE_OUTPUT].setVoltage(s, c);
					outputs[INV_OUTPUT].setVoltage(-s, c);
				}
				else {
					doSample[c] = trackMode == PASS;
					gateTrig[c].set(0.0f);
				}
			}
			
			forceSample = false;
		}
	}
	
};

struct SampleAndHold2Widget : ModuleWidget {

	std::string panelName;
	
	SampleAndHold2Widget(SampleAndHold2 *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]),module, SampleAndHold2::TRIG_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]),module, SampleAndHold2::SAMPLE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]),module, SampleAndHold2::MODE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]),module, SampleAndHold2::PROB_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]),module, SampleAndHold2::OFFSET_INPUT));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, SampleAndHold2::SAMPLE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, SampleAndHold2::INV_OUTPUT));

		// mode switch
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, SampleAndHold2::MODE_PARAM));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 19, STD_ROWS6[STD_ROW2] - 18), module, SampleAndHold2::PASS_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 19, STD_ROWS6[STD_ROW2]), module, SampleAndHold2::TRACK_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 19, STD_ROWS6[STD_ROW2] + 18), module, SampleAndHold2::SAMPLE_LIGHT));

		// Knobs
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, SampleAndHold2::PROB_PARAM));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, SampleAndHold2::PROB_CV_PARAM));
		addParam(createParamCentered<Potentiometer<VioletKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, SampleAndHold2::LEVEL_PARM));
		addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, SampleAndHold2::OFFSET_PARAM));
	}
	
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SampleAndHold2 *module = dynamic_cast<SampleAndHold2*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *modelSampleAndHold2 = createModel<SampleAndHold2, SampleAndHold2Widget>("SampleAndHold2");
