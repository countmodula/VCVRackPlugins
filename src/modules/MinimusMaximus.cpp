//----------------------------------------------------------------------------
//	Count Modula - Minimus Maximus Module
//	A Min/Max/Mean processor with gate outputs to indicate which inputs are at
//	the minimum or maximum level
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Polarizer.hpp"

struct MinimusMaximus : Module {
	enum ParamIds {
		BIAS_PARAM,
		BIAS_ON_PARAM,
		MODE_PARAM,
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
		A_MAX_OUTPUT,
		B_MAX_OUTPUT,
		C_MAX_OUTPUT,
		D_MAX_OUTPUT,
		A_MIN_OUTPUT,
		B_MIN_OUTPUT,
		C_MIN_OUTPUT,
		D_MIN_OUTPUT,
		MIN_OUTPUT,
		MAX_OUTPUT,
		AVG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	MinimusMaximus() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// controls
		configParam(BIAS_ON_PARAM, 0.0f, 1.0f, 0.0f, "Bias on/off");
		configParam(BIAS_PARAM, -5.0f, 5.0f, 0.0f, "Bias Amount");
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Output Mode (Uni/Bipolar)");
	}

	void process(const ProcessArgs &args) override {
	
		float tot = 0.0f;
		int count = 0;
		float min = 99999.0f;
		float max = -99999.0f;
		float f = 0.0f;
		
		// calculate max/min amounts and requisites for average
		for (int i = 0; i < 4; i++) {
			bool useBias = (i == 3 && params[BIAS_ON_PARAM].getValue() > 0.5f);
			
			if (inputs[A_INPUT+i].isConnected() || useBias) {
				// grab the value taking the bias setting of the last input into consideration
				f = useBias ? inputs[A_INPUT+i].getNormalVoltage(params[BIAS_PARAM].getValue()) : inputs[A_INPUT+i].getVoltage();
				
				count++;
				tot += f;
				
				if (f > max)
					max = f;
				
				if (f < min)
					min = f;
			}
		}
		
		// determine the comparator output levels
		bool isGate = (params[MODE_PARAM].getValue() < 0.5f);
		
		// cater for possibility of no inputs
		if (count > 0)
			outputs[AVG_OUTPUT].setVoltage(tot /(float)(count));
		else {
			// no inputs - set outputs to zero
			outputs[AVG_OUTPUT].setVoltage(0.0f);
			max = min = 0.0f;
		}
		
		// set the min/max outputs
		outputs[MIN_OUTPUT].setVoltage(min);
		outputs[MAX_OUTPUT].setVoltage(max);

		// set the logic outputs
		bool isMin, isMax;
		for (int i = 0; i < 4; i++) {
			isMin = (inputs[A_INPUT + i].isConnected() && (inputs[A_INPUT + i].getVoltage() == min));
			isMax = (inputs[A_INPUT + i].isConnected() && (inputs[A_INPUT + i].getVoltage() == max));
			
			outputs[A_MIN_OUTPUT + i].setVoltage(isGate ? boolToGate(isMin) : boolToAudio(isMin));
			outputs[A_MAX_OUTPUT + i].setVoltage(isGate ? boolToGate(isMax) : boolToAudio(isMax));
		}
	}

};

struct MinimusMaximusWidget : ModuleWidget {
MinimusMaximusWidget(MinimusMaximus *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MinimusMaximus.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// controls
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, MinimusMaximus::BIAS_ON_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, MinimusMaximus::BIAS_PARAM));
		addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, MinimusMaximus::MODE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, MinimusMaximus::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, MinimusMaximus::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, MinimusMaximus::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, MinimusMaximus::D_INPUT));
		
		// logical outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, MinimusMaximus::A_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, MinimusMaximus::A_MAX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, MinimusMaximus::B_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, MinimusMaximus::B_MAX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, MinimusMaximus::C_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, MinimusMaximus::C_MAX_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, MinimusMaximus::D_MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, MinimusMaximus::D_MAX_OUTPUT));
		
		// mathematical outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, MinimusMaximus::AVG_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, MinimusMaximus::MIN_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, MinimusMaximus::MAX_OUTPUT));
	}
};

Model *modelMinimusMaximus = createModel<MinimusMaximus, MinimusMaximusWidget>("MinimusMaximus");
