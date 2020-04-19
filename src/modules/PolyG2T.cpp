//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - PolyG2T Module
//	Gate To Trigger Module
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Inverter.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME PolyG2T
#define PANEL_FILE "PolyG2T.svg"

struct PolyG2T : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		GATE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		INV_OUTPUT,
		START_OUTPUT,
		END_OUTPUT,
		EDGE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(GATE_LIGHT, 16),
		ENUMS(START_LIGHT, 16),
		ENUMS(END_LIGHT, 16),
		ENUMS(EDGE_LIGHT, 16),
		NUM_LIGHTS
	};

	GateProcessor gpGate[16];
	dsp::PulseGenerator pgStart[16];
	dsp::PulseGenerator pgEnd[16];

	bool gate = false, sTrig = false, eTrig = false;
	int counter = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	PolyG2T() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
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
	
	void onReset() override {
		for(int i = 0; i < 16; i++) {
			gpGate[i].reset();
			pgStart[i].reset();
			pgEnd[i].reset();
		}
		
		resetLEDMatrices();
	}
	
	void resetLEDMatrices() {
		for (int i = 0; i < 16 ; i++) {
			lights[GATE_LIGHT + i].setBrightness(0.0f);
			lights[START_LIGHT + i].setBrightness(0.0f);
			lights[END_LIGHT + i].setBrightness(0.0f);
			lights[EDGE_LIGHT + i].setBrightness(0.0f);
		}
	}
	
	void process(const ProcessArgs &args) override {
		
		if (inputs[GATE_INPUT].isConnected()) {
			
			// how many channels do we have?
			int numChans = inputs[GATE_INPUT].getChannels();
			outputs[GATE_OUTPUT].setChannels(numChans);
			outputs[INV_OUTPUT].setChannels(numChans);
			outputs[START_OUTPUT].setChannels(numChans);
			outputs[END_OUTPUT].setChannels(numChans);
			outputs[EDGE_OUTPUT	].setChannels(numChans);		
			
			// process each channel
			for (int c = 0; c < 16; c ++) {
				// process the input
				gpGate[c].set(inputs[GATE_INPUT].getPolyVoltage(c));
				gate = gpGate[c].high();
				
				// leading edge - fire the start trigger
				if (gpGate[c].leadingEdge()) {
					pgStart[c].trigger(1e-3f);
					sTrig = true;
				}
				else {
					sTrig = pgStart[c].remaining > 0.0f;
					pgStart[c].process(args.sampleTime);
				}

				// trailing edge - fire the end trigger
				if (gpGate[c].trailingEdge()) {
					pgEnd[c].trigger(1e-3f);
					eTrig = true;
				}
				else {
					eTrig = pgEnd[c].remaining > 0.0f;
					pgEnd[c].process(args.sampleTime);
				}
				
				// process the trigger outputs
				if (c < numChans) {
					outputs[GATE_OUTPUT].setVoltage(boolToGate(gate), c); 
					outputs[INV_OUTPUT].setVoltage(boolToGate(!gate), c);
					outputs[START_OUTPUT].setVoltage(boolToGate(sTrig), c);
					outputs[END_OUTPUT].setVoltage(boolToGate(eTrig), c);
					outputs[EDGE_OUTPUT].setVoltage(boolToGate(sTrig || eTrig), c);
				}
				
				// and finally the lights
				if (counter == 0) {
					float elapsed = args.sampleTime * 2.0;
					lights[GATE_LIGHT + c].setBrightness(boolToLight(gate));
					lights[START_LIGHT + c].setSmoothBrightness(boolToLight(sTrig), elapsed);
					lights[END_LIGHT + c].setSmoothBrightness(boolToLight(eTrig), elapsed);
					lights[EDGE_LIGHT + c].setSmoothBrightness(boolToLight(sTrig || eTrig), elapsed);
				}
			}
		}
		else {
			outputs[GATE_OUTPUT].channels = 0;
			outputs[INV_OUTPUT].channels = 0;
			outputs[START_OUTPUT].channels = 0;
			outputs[END_OUTPUT].channels = 0;
			outputs[EDGE_OUTPUT].channels = 0;
			
			if (counter == 0)
				resetLEDMatrices();
		}
		
		if (++counter > 3)
			counter = 0;
	}
};

struct PolyG2TWidget : ModuleWidget {
	PolyG2TWidget(PolyG2T *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/PolyG2T.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW1]), module, PolyG2T::GATE_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, PolyG2T::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, PolyG2T::INV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, PolyG2T::START_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, PolyG2T::EDGE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, PolyG2T::END_OUTPUT));
		
		// lights
		addLEDMatrix(PolyG2T::GATE_LIGHT, STD_COL2, STD_ROW2, 0);
		addLEDMatrix(PolyG2T::START_LIGHT, STD_COL3, STD_ROW4, -4);
		addLEDMatrix(PolyG2T::EDGE_LIGHT, STD_COL3, STD_ROW5, -4);
		addLEDMatrix(PolyG2T::END_LIGHT, STD_COL3, STD_ROW6, -4);
	}
	
	void addLEDMatrix(int lightID, int colPos, int rowPos, int rowOffset) { 
		// led matrix
		int x = 0, y = 0;
		int startPos = STD_ROWS6[rowPos] - 15 + rowOffset;
		for (int s = 0; s < 16; s++) {
			if (x > 3) {
				x = 0;
				y++;
			}
			
			addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[colPos] - 15 + (10 * x++), startPos + (10 * y)), module, lightID + s));
		}	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		PolyG2T *module = dynamic_cast<PolyG2T*>(this->module);
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

Model *modelPolyG2T = createModel<PolyG2T, PolyG2TWidget>("PolyG2T");
