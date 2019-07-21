//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Sample & Hold Module
//	Sample/Track and Hold
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

struct SampleAndHold : Module {
	enum ParamIds {
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SAMPLE_INPUT,
		TRIG_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SAMPLE_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	GateProcessor gateTrig;
	
	SampleAndHold() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		// tracking mode switch
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Sample or Track Mode");
	}
	
	void onReset() override {
		gateTrig.reset();
	}	
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		return root;
	}
	
	void process(const ProcessArgs &args) override {

		// process the trigger input
		gateTrig.set(inputs[TRIG_INPUT].getVoltage());
		
		// determine the mode
		bool trackMode = (params[MODE_PARAM].getValue() > 0.5f);
		
		// determine number of channels
		int n = inputs[SAMPLE_INPUT].getChannels();
		outputs[SAMPLE_OUTPUT].setChannels(n);
		outputs[INV_OUTPUT].setChannels(n);

		// now sample away
		float s;
		for (int c = 0; c < n; c++) {
			
			if ((trackMode && gateTrig.high()) || (!trackMode && gateTrig.leadingEdge())) {
				// track the input
				s = inputs[SAMPLE_INPUT].getVoltage(c);
			}
			else {
				// hold the last sample value
				s = outputs[SAMPLE_OUTPUT].getVoltage(c);
			}
			
			// set the output voltages
			outputs[SAMPLE_INPUT].setVoltage(s, c);
			outputs[INV_OUTPUT].setVoltage(-s, c);
		}
	}
	
};

struct SampleAndHoldWidget : ModuleWidget {
SampleAndHoldWidget(SampleAndHold *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SampleAndHold.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// inputs`
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]),module, SampleAndHold::SAMPLE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]),module, SampleAndHold::TRIG_INPUT));

		// mode switch
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS6(STD_ROW3)), module, SampleAndHold::MODE_PARAM));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, SampleAndHold::SAMPLE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, SampleAndHold::INV_OUTPUT));
	}
};

Model *modelSampleAndHold = createModel<SampleAndHold, SampleAndHoldWidget>("SampleAndHold");
