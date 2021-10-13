//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - PolyLogic (Bernoulli Gate)
//  Copyright (C) 2020  Adam Verspaget
//  Logic portions taken from Branches (Bernoulli Gate) by Andrew Belt
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyLogic
#define PANEL_FILE "PolyLogic.svg"

struct PolyLogic : Module {
	enum ParamIds {
		XORMODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		AND_OUTPUT,
		OR_OUTPUT,
		XOR_OUTPUT,
		NAND_OUTPUT,
		NOR_OUTPUT,
		XNOR_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STATE_LIGHT, 32),
		AND_LIGHT,
		OR_LIGHT,
		XOR_LIGHT,
		NAND_LIGHT,
		NOR_LIGHT,
		XNOR_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gateTriggers[16];
	int count = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	PolyLogic() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		configSwitch(XORMODE_PARAM, 0.0f, 1.0, 0.0f, "XOR mode", {"Normal", "1-Hot"});

		configInput(GATE_INPUT, "Polyphonic");

		configOutput(AND_OUTPUT, "AND");
		configOutput(OR_OUTPUT, "OR");
		configOutput(XOR_OUTPUT, "XOR");
		configOutput(NAND_OUTPUT, "NAND");
		configOutput(NOR_OUTPUT, "NOR");
		configOutput(XNOR_OUTPUT, "XNOR");

		outputInfos[AND_OUTPUT]->description = "Monophinic output representing the result of a logical AND across the input channels";
		outputInfos[OR_OUTPUT]->description = "Monophinic output representing the result of a logical OR across the input channels";
		outputInfos[XOR_OUTPUT]->description = "Monophinic output representing the result of a logical XOR across the input channels";
		outputInfos[NAND_OUTPUT]->description = "Monophinic output representing the result of a logical NAND across the input channels";
		outputInfos[NOR_OUTPUT]->description = "Monophinic output representing the result of a logical NOR across the input channels";
		outputInfos[XNOR_OUTPUT]->description = "Monophinic output representing the result of a logical XNOR across the input channels";

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		for (int i = 0; i < 16; i++) {
			gateTriggers[i].reset();
		}
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();
		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void resetLEDMatrix() {
		for (int i = 0; i < 32; i++) {
			lights[STATE_LIGHT + i].setBrightness(0.0f);
		}
	}
	
	void process(const ProcessArgs &args) override {
		
		if (inputs[GATE_INPUT].isConnected()) {
			int numChannels = inputs[GATE_INPUT].getChannels();
			int l = 0;
			
			for (int c = 0; c < 16; c++) {
				
				if (c < numChannels) {
					// process the gate inputs
					gateTriggers[c].set(inputs[GATE_INPUT].getVoltage(c));

					// set active channel light - leave off if th channel is high so we don't mix the colours
					lights[STATE_LIGHT + (c * 2) + 1].setBrightness(boolToLight(gateTriggers[c].low()));
					
					// calculate the logic here
					if (gateTriggers[c].high())
						l++;
				}
				else {
					gateTriggers[c].set(0.0f);
					lights[STATE_LIGHT + (c * 2) + 1].setBrightness(0.0f);
				}
				
				lights[STATE_LIGHT + (c * 2)].setBrightness(boolToLight(gateTriggers[c].high()));
			}
			
			// now set the outputs
			bool bAnd = (l == numChannels);
			bool bOr = (l > 0);
			bool bXor = (params[XORMODE_PARAM].getValue() > 0.5f ? l == 1 : isOdd(l));
			
			outputs[AND_OUTPUT].setVoltage(boolToGate(bAnd));
			outputs[OR_OUTPUT].setVoltage(boolToGate(bOr));
			outputs[XOR_OUTPUT].setVoltage(boolToGate(bXor));
			outputs[NAND_OUTPUT].setVoltage(boolToGate(!bAnd));
			outputs[NOR_OUTPUT].setVoltage(boolToGate(!bOr));
			outputs[XNOR_OUTPUT].setVoltage(boolToGate(!bXor));

			lights[AND_LIGHT].setBrightness(boolToLight(bAnd));
			lights[OR_LIGHT].setBrightness(boolToLight(bOr));
			lights[XOR_LIGHT].setBrightness(boolToLight(bXor));
			lights[NAND_LIGHT].setBrightness(boolToLight(!bAnd));
			lights[NOR_LIGHT].setBrightness(boolToLight(!bOr));
			lights[XNOR_LIGHT].setBrightness(boolToLight(!bXor));
		}
		else {
			outputs[AND_OUTPUT].setVoltage(0.0f);
			outputs[OR_OUTPUT].setVoltage(0.0f);
			outputs[XOR_OUTPUT].setVoltage(0.0f);
			outputs[NAND_OUTPUT].setVoltage(10.0f);
			outputs[NOR_OUTPUT].setVoltage(10.0f);
			outputs[XNOR_OUTPUT].setVoltage(10.0f);
	
			if (count == 0) {
				resetLEDMatrix();
				
				lights[AND_LIGHT].setBrightness(0.0);
				lights[OR_LIGHT].setBrightness(0.0);
				lights[XOR_LIGHT].setBrightness(0.0);
				lights[NAND_LIGHT].setBrightness(0.0);
				lights[NOR_LIGHT].setBrightness(0.0);
				lights[XNOR_LIGHT].setBrightness(0.0);
			}
		}
		
		if (++count > 3)
			count = 0;
	}
};

struct PolyLogicWidget : ModuleWidget {

	std::string panelName;
	
	PolyLogicWidget(PolyLogic *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// params
		
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS5[STD_ROW5]), module, PolyLogic:: XORMODE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW1]), module, PolyLogic::GATE_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2]), module, PolyLogic::AND_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3]), module, PolyLogic::OR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4]), module, PolyLogic::XOR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW2]), module, PolyLogic::NAND_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW3]), module, PolyLogic::NOR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW4]), module, PolyLogic::XNOR_OUTPUT));
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2] - 20), module, PolyLogic::AND_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3] - 20), module, PolyLogic::OR_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4] - 20), module, PolyLogic::XOR_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW2] - 20), module, PolyLogic::NAND_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW3] - 20), module, PolyLogic::NOR_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW4] - 20), module, PolyLogic::XNOR_LIGHT));
		
		
		
		// led matrix
		int x = 0, y = 0;
		int startPos = STD_ROWS5[STD_ROW1] - 15;
		for (int s = 0; s < 16; s++) {
			if (x > 3) {
				x = 0;
				y++;
			}
			
			addChild(createLightCentered<SmallLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15 + (10 * x++), startPos + (10 * y)), module, PolyLogic::STATE_LIGHT + (s * 2)));
		}		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyLogic *module = dynamic_cast<PolyLogic*>(this->module);
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

Model *modelPolyLogic = createModel<PolyLogic, PolyLogicWidget>("PolyLogic");
