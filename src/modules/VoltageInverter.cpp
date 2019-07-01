//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Voltage Inverter Module
//	A basic quad voltage inverter
//----------------------------------------------------------------------------
#include "../CountModula.hpp"

struct VoltageInverter : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	float out = 0.0f;
	VoltageInverter() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override {
		int inputToUse = 0;
		for (int i = 0; i < 4; i++) {
			// determine which input we want to actually use
			if (inputs[A_INPUT + i].isConnected())
				inputToUse = i;
				
			// grab output value normallised to the previous input value
			int n = inputs[A_INPUT + inputToUse].getChannels();
			outputs[A_OUTPUT + i].setChannels(n);
			for (int c = 0; c < n; c++)
				outputs[A_OUTPUT + i].setVoltage(-inputs[A_INPUT + inputToUse].getVoltage(c), c);
		}
	}
};

struct VoltageInverterWidget : ModuleWidget {
	VoltageInverterWidget(VoltageInverter *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VoltageInverter.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		for (int i = 0; i < 4; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (i * 2)]), module, VoltageInverter::A_INPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (i * 2)]), module, VoltageInverter::A_OUTPUT + (i)));
		}
	}
};

Model *modelVoltageInverter = createModel<VoltageInverter, VoltageInverterWidget>("VoltageInverter");
