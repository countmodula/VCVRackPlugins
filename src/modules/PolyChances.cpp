//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - PolyChances (Bernoulli Gate)
//  Copyright (C) 2020  Adam Verspaget
//  Logic portions taken from Branches (Bernoulli Gate) by Andrew Belt
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyChances
#define PANEL_FILE "PolyChances.svg"

struct PolyChances : Module {
	enum ParamIds {
		THRESH_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		PROB_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(PROB_LIGHT, 32),
		NUM_LIGHTS
	};

	GateProcessor gateTriggers[16];
	
	bool latch = false;
	bool toggle = false;
	bool outcome[16];
	bool gate[16] = {};
	bool a[16] = {};
	bool b[16] = {};
	int count = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	PolyChances() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		configParam(THRESH_PARAM, 0.0f, 1.0f, 0.5f, "Output B chance", "%", 0.0f, 100.0f, 0.0f);
		configSwitch(MODE_PARAM, 0.0f, 2.0f, 1.0f, "Mode", {"Toggle", "Normal", "Latch"});
		
		configInput(GATE_INPUT, "Gate");
		configInput(PROB_INPUT, "Probability CV");
		
		configOutput(A_OUTPUT, "A");
		configOutput(B_OUTPUT, "B");
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		for (int i = 0; i < 16; i++) {
			gateTriggers[i].reset();
			outcome[i] = true;
			a[i] = false;
			b[i] = false;
			gate[i] = false;
		}
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();
		json_object_set_new(root, "moduleVersion", json_integer(2));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}

	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
	
	void updateLEDMatrix(float sampleTime) {
		for (int i = 0; i < 16; i++) {
			
			if (a[i] || b[i]) {
				// just flip the lights if we have one or the other
				lights[PROB_LIGHT + (i * 2)].setBrightness(boolToLight(a[i]));
				lights[PROB_LIGHT + (i * 2) + 1].setBrightness(boolToLight(b[i]));
			}
			else {
				// fade the lights if we've got nothing going on
				lights[PROB_LIGHT + (i * 2)].setSmoothBrightness(boolToLight(a[i]), sampleTime);
				lights[PROB_LIGHT + (i * 2) + 1].setSmoothBrightness(boolToLight(b[i]), sampleTime);
			}
		}
	}
	
	void resetLEDMatrix() {
		for (int i = 0; i < 32; i++) {
			lights[PROB_LIGHT + i].setBrightness(0.0f);
		}
	}
	
	void process(const ProcessArgs &args) override {
		
		if (inputs[GATE_INPUT].isConnected()) {

			int numChannels = inputs[GATE_INPUT].getChannels();
			outputs[A_OUTPUT].setChannels(numChannels);
			outputs[B_OUTPUT].setChannels(numChannels);
			
			// what mode are we in?
			switch ((int)(params[MODE_PARAM].getValue())) {
				case 2:
					latch = true;
					toggle = false;
					break;
				case 0:
					latch = false;
					toggle = true;
					break;
				default:
					latch = false;
					toggle = false;
					break;
			}
			
			for (int i = 0; i < 16; i ++) {
				
				if (i < numChannels) {
					// process the gate inputs
					gateTriggers[i].set(inputs[GATE_INPUT].getVoltage(i));

					// determine which outputs we're going to use
					if (gateTriggers[i].leadingEdge()) {
						float r = random::uniform();
						float vProb = inputs[PROB_INPUT].getPolyVoltage(i);
						float threshold = clamp(params[THRESH_PARAM].getValue() + vProb / 10.f, 0.0f, 1.0f);
						
						// toggle mode only changes when the outcome is different to the last outcome
						if (toggle)
							outcome[i] = (outcome[i] != (r < threshold));
						else
							outcome[i] = (r < threshold);						
					}
					
					// in latch mode, the outputs should just flip between themselves based on the outcome rather than following the gate input
					gate[i] = latch || gateTriggers[i].high();
				}
				else
					gate[i] = false;
				
				b[i] = outcome[i] && gate[i];
				a[i] = !outcome[i] && gate[i];
				
				if (i < numChannels) {
					outputs[A_OUTPUT].setVoltage(boolToGate(a[i]), i);
					outputs[B_OUTPUT].setVoltage(boolToGate(b[i]), i);
				}
			}
			if (count == 0)
			 updateLEDMatrix(args.sampleTime);
		}
		else {
			outputs[A_OUTPUT].channels = 0;
			outputs[B_OUTPUT].channels = 0;
			
			if (count == 0)
				resetLEDMatrix();
		}
		
		if (++count > 3)
			count = 0;
	}
};

struct PolyChancesWidget : ModuleWidget {

	std::string panelName;
	
	PolyChancesWidget(PolyChances *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS6[STD_ROW2]), module, PolyChances::THRESH_PARAM));
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS6[STD_ROW3]), module, PolyChances::MODE_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS6[STD_ROW4]), module, PolyChances::GATE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 15, STD_ROWS6[STD_ROW1]), module, PolyChances::PROB_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1] -10, STD_ROWS6[STD_ROW6]), module, PolyChances::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, STD_ROWS6[STD_ROW6]), module, PolyChances::B_OUTPUT));
		
		// outcome lights
		
		// led matrix
		int x = 0, y = 0;
		int startPos = STD_ROWS6[STD_ROW5] - 15;
		for (int s = 0; s < 16; s++) {
			if (x > 3) {
				x = 0;
				y++;
			}
			
			addChild(createLightCentered<SmallLight<CountModulaLightRG>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + (10 * x++), startPos + (10 * y)), module, PolyChances::PROB_LIGHT + (s * 2)));
		}		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyChances *module = dynamic_cast<PolyChances*>(this->module);
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

Model *modelPolyChances = createModel<PolyChances, PolyChancesWidget>("PolyChances");
