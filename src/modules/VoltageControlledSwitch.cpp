//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Voltage Controlled Switch Module
//	A 2 in/1 out 1 in/2 out voltage controlled switch
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct VoltageControlledSwitch : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		A_INPUT,
		B1_INPUT,
		B2_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A1_OUTPUT,
		A2_OUTPUT,
		B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		A1_LIGHT,
		A2_LIGHT,
		B1_LIGHT,
		B2_LIGHT,
		NUM_LIGHTS
	};

	dsp::SchmittTrigger stSwitch;
	
	VoltageControlledSwitch() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void onReset() override {
		stSwitch.reset();
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		return root;
	}
	
	void process(const ProcessArgs &args) override {
		
		float cv = inputs[CV_INPUT].getNormalVoltage(0.0f);
		
		// not using standard gate processor here due to different trigger levels
		stSwitch.process(rescale(cv, 3.2f, 3.6f, 0.0f, 1.0f));

		int nA = inputs[A_INPUT].getChannels();
		int nB1 = inputs[B1_INPUT].getChannels();
		int nB2 = inputs[B2_INPUT].getChannels();
		
		if (stSwitch.isHigh()) {
			// IN A -> OUT A2
			outputs[A1_OUTPUT].setChannels(nA);
			outputs[A2_OUTPUT].setChannels(nA);
			for (int c = 0; c < nA; c++) {
				outputs[A1_OUTPUT].setVoltage(0.0f, c);
				outputs[A2_OUTPUT].setVoltage(inputs[A_INPUT].getVoltage(c), c);
			}
			
			// IN B2 -> OUT B
			outputs[B_OUTPUT].setChannels(nB2);
			for (int c = 0; c < nB2; c++)
				outputs[B_OUTPUT].setVoltage(inputs[B2_INPUT].getVoltage(c), c);
			
			lights[A1_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[A2_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
			lights[B1_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[B2_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
		}
		else {
			// IN A -> OUT A1
			outputs[A1_OUTPUT].setChannels(nA);
			outputs[A2_OUTPUT].setChannels(nA);
			for (int c = 0; c < nA; c++) {
				outputs[A1_OUTPUT].setVoltage(inputs[A_INPUT].getVoltage(c), c);
				outputs[A2_OUTPUT].setVoltage(0.0f, c);
			}
			
			// IN B1 -> OUT B
			outputs[B_OUTPUT].setChannels(nB1);
			for (int c = 0; c < nB2; c++)
				outputs[B_OUTPUT].setVoltage(inputs[B1_INPUT].getVoltage(c), c);
			
			lights[A1_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
			lights[A2_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
			lights[B1_LIGHT].setSmoothBrightness(1.0f, args.sampleTime);
			lights[B2_LIGHT].setSmoothBrightness(0.0f, args.sampleTime);
		}
	}
};


struct VoltageControlledSwitchWidget : ModuleWidget {
VoltageControlledSwitchWidget(VoltageControlledSwitch *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VoltageControlledSwitch.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]), module, VoltageControlledSwitch::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]), module, VoltageControlledSwitch::A_INPUT));
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, VoltageControlledSwitch::B1_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]), module, VoltageControlledSwitch::B2_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]), module, VoltageControlledSwitch::A1_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]), module, VoltageControlledSwitch::A2_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, VoltageControlledSwitch::B_OUTPUT));

		// lights
		int ledColumn = 2 + ((STD_COLUMN_POSITIONS[STD_COL1] + STD_COLUMN_POSITIONS[STD_COL2])/ 2);
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW2) + 4), module, VoltageControlledSwitch::A1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW3) + 4), module, VoltageControlledSwitch::A2_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW4) + 5), module, VoltageControlledSwitch::B1_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(ledColumn, STD_HALF_ROWS7(STD_ROW5) + 5), module, VoltageControlledSwitch::B2_LIGHT));
	}
};

Model *modelVoltageControlledSwitch = createModel<VoltageControlledSwitch, VoltageControlledSwitchWidget>("VoltageControlledSwitch");
