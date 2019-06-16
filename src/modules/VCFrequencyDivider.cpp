//----------------------------------------------------------------------------
//	Count Modula - Voltage Controlled Divider Module
//	A voltage controlled frequency divider (divide by 1 - approx 20)
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/Utility.hpp"

struct VCFrequencyDivider : Module {
	enum ParamIds {
		CV_PARAM,
		MANUAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		DIV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		DIVB_OUTPUT,
		DIVU_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	FrequencyDivider divider;
	
	VCFrequencyDivider() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(CV_PARAM, -1.0f, 1.0f, 0.0f, "CV Level");
		configParam(MANUAL_PARAM, 0.0f, 10.0f, 0.0f, "Manual Level");
	}

	void onReset() override {
		divider.reset();
	}
	
	void process(const ProcessArgs &args) override {

		divider.setMaxN(20);
		divider.setCountMode(COUNT_DN);
		
		float div = params[MANUAL_PARAM].getValue() + (params[CV_PARAM].getValue() * inputs[CV_INPUT].getNormalVoltage(0.0f));
		divider.setN(div);
		divider.process(inputs[DIV_INPUT].getNormalVoltage(0.0f));
		
		outputs[DIVB_OUTPUT].setVoltage(boolToAudio(divider.phase));
		outputs[DIVU_OUTPUT].setVoltage(boolToGate(divider.phase));
	}

};

struct VCFrequencyDividerWidget : ModuleWidget {
VCFrequencyDividerWidget(VCFrequencyDivider *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VCFrequencyDivider.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, VCFrequencyDivider::CV_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, VCFrequencyDivider::MANUAL_PARAM));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, VCFrequencyDivider::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, VCFrequencyDivider::DIV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, VCFrequencyDivider::DIVB_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, VCFrequencyDivider::DIVU_OUTPUT));
	}
};

Model *modelVCFrequencyDivider = createModel<VCFrequencyDivider, VCFrequencyDividerWidget>("VCFrequencyDivider");
