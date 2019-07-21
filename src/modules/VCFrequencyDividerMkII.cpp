//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Voltage Controlled Divider Module MkII
//	A voltage controlled frequency divider (divide by 1 - approx 20)
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/FrequencyDivider.hpp"
#include "../inc/Utility.hpp"

struct VCFrequencyDividerMkII : Module {
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
	FrequencyDividerOld legacyDivider;
	
	bool antiAlias = false;
	bool legacyMode = false;
	float legacyCVMap[21] = {	0.25f,
								0.75f,
								1.25f,
								1.75f,
								2.25f,
								2.75f,
								3.25f,
								3.75f,
								4.25f,
								4.75f,
								5.25f,
								5.75f,
								6.25f,
								6.75f,
								7.25f,
								7.75f,
								8.25f,
								8.75f,
								9.25f,
								9.75f,
								10.25f };

	VCFrequencyDividerMkII() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		// scale 10V CV up to 20
		configParam(CV_PARAM, -2.0f, 2.0f, 0.0f, "CV Amount", " %", 0.0f, 50.0f, 0.0f);
		configParam(MANUAL_PARAM, 1.0f, 21.0f, 0.0f, "Divide by");
	}

	void onReset() override {
		divider.reset();
		legacyDivider.reset();
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("2.0"));
		json_object_set_new(root, "antiAlias", json_boolean(antiAlias));
		json_object_set_new(root, "legacyMode", json_boolean(legacyMode));
	
		return root;
	}

	void dataFromJson(json_t *root) override {
	
		json_t *legacy = json_object_get(root, "legacyMode");
		if (legacy)
			legacyMode = json_boolean_value(legacy);
	
		json_t *aa = json_object_get(root, "antiAlias");
		if (aa)
			antiAlias = json_boolean_value(aa);
	}	
	
	void process(const ProcessArgs &args) override {

		bool phase = false;
		if (legacyMode) {
			legacyDivider.setMaxN(20);
			legacyDivider.setCountMode(COUNT_DN);
			
			float div =  legacyCVMap[clamp((int)(params[MANUAL_PARAM].getValue()), 1, 21) - 1];
			float divCV = params[CV_PARAM].getValue() * inputs[CV_INPUT].getNormalVoltage(0.0f);
			
			legacyDivider.setN(div + divCV);
			legacyDivider.process(inputs[DIV_INPUT].getNormalVoltage(0.0f));
			phase = legacyDivider.phase;			
		}
		else {
			divider.setMaxN(21);
			divider.setCountMode(COUNT_DN);
			
			float div = params[MANUAL_PARAM].getValue();
			float divCV = params[CV_PARAM].getValue() * inputs[CV_INPUT].getNormalVoltage(0.0f);
		
			divider.setN((int)(div + divCV));
			divider.process(inputs[DIV_INPUT].getNormalVoltage(0.0f));
			phase = divider.phase;
		}
	
		outputs[DIVB_OUTPUT].setVoltage(boolToAudio(phase));
		outputs[DIVU_OUTPUT].setVoltage(boolToGate(phase));
	}

};

struct VCFrequencyDividerMkIIWidget : ModuleWidget {
VCFrequencyDividerMkIIWidget(VCFrequencyDividerMkII *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VCFrequencyDividerMkII.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, VCFrequencyDividerMkII::CV_PARAM));
		addParam(createParamCentered<CountModulaRotarySwitchWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, VCFrequencyDividerMkII::MANUAL_PARAM));
		
		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, VCFrequencyDividerMkII::CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, VCFrequencyDividerMkII::DIV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, VCFrequencyDividerMkII::DIVB_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, VCFrequencyDividerMkII::DIVU_OUTPUT));
	}
	
	struct LegacyModeMenuItem : MenuItem {
		VCFrequencyDividerMkII *module;
		void onAction(const event::Action &e) override {
			module->legacyMode = !module->legacyMode;
			module->onReset();
		}
	};	
	
	void appendContextMenu(Menu *menu) override {
		VCFrequencyDividerMkII *module = dynamic_cast<VCFrequencyDividerMkII*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuLabel());
		
		// pretty heading
		MenuLabel *settingsLabel = new MenuLabel();
		settingsLabel->text = "VC Frequency Divider MkII Settings";
		menu->addChild(settingsLabel);		
		
		// add legacy mode menu item
		LegacyModeMenuItem *legacyMenuItem = createMenuItem<LegacyModeMenuItem>("Legacy Mode", CHECKMARK(module->legacyMode));
		legacyMenuItem->module = module;
		menu->addChild(legacyMenuItem);
	}	
};

Model *modelVCFrequencyDividerMkII = createModel<VCFrequencyDividerMkII, VCFrequencyDividerMkIIWidget>("VCFrequencyDividerMkII");
