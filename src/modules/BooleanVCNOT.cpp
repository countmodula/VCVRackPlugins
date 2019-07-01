//----------------------------------------------------------------------------
//	/^M^\ Count Modula - VC NOT Logic Gate Module
//	A Voltage controlled logical inverter
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/GateProcessor.hpp"

struct BooleanVCNOT : Module {
	enum ParamIds {
		ENUMS(ENABLE_PARAM, 2),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(LOGIC_INPUT, 2),
		ENUMS(ENABLE_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(INV_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Inverter inverter[2];
	
	BooleanVCNOT() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}
	
	void onReset() override {
		for (int i = 0; i < 2; i++)
			inverter[i].reset();
	}
	
	void process(const ProcessArgs &args) override {
		//perform the logic
		for (int i = 0; i < 2; i++) {
			float out =  inverter[i].process(inputs[LOGIC_INPUT + i].getVoltage(), inputs[ENABLE_INPUT + i].getNormalVoltage(10.0f));
			outputs[INV_OUTPUT + i].setVoltage(out);
		}
	}	
	
};

struct BooleanVCNOTWidget : ModuleWidget {
	BooleanVCNOTWidget(BooleanVCNOT *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BooleanVCNOT.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		for (int i = 0; i < 2; i++) {
			// logic and enable inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + (i * 3)]),module, BooleanVCNOT::LOGIC_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 + (i * 3)]),module, BooleanVCNOT::ENABLE_INPUT + i));
		
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + (i * 3)]), module, BooleanVCNOT::INV_OUTPUT + i));
		}
	}
};

Model *modelBooleanVCNOT = createModel<BooleanVCNOT, BooleanVCNOTWidget>("BooleanVCNOT");
