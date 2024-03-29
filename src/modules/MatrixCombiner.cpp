//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Step Sequencer Module
//	A classic 8 step CV/Gate sequencer
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME MatrixCombiner
#define PANEL_FILE "MatrixCombiner.svg"

struct MatrixCombiner : Module {

	enum ParamIds {
		ENUMS(BUS_A_PARAMS, 7),
		ENUMS(BUS_B_PARAMS, 7),
		ENUMS(BUS_C_PARAMS, 7),
		ENUMS(BUS_D_PARAMS, 7),
		ENUMS(BUS_E_PARAMS, 7),
		ENUMS(BUS_F_PARAMS, 7),
		MODE_PARAM,
		NUM_PARAMS
	};
	
	enum InputIds {
		ENUMS(GATE_INPUTS, 7),
		NUM_INPUTS
	};
	
	enum OutputIds {
		A_OUTPUT,
		B_OUTPUT,
		C_OUTPUT,
		D_OUTPUT,
		E_OUTPUT,
		F_OUTPUT,
		NUM_OUTPUTS
	};
	
	enum LightIds {
		A_LIGHT,
		B_LIGHT,
		C_LIGHT,
		D_LIGHT,
		E_LIGHT,
		F_LIGHT,
		ENUMS(BUS_A_PARAM_LIGHTS, 7),
		ENUMS(BUS_B_PARAM_LIGHTS, 7),
		ENUMS(BUS_C_PARAM_LIGHTS, 7),
		ENUMS(BUS_D_PARAM_LIGHTS, 7),
		ENUMS(BUS_E_PARAM_LIGHTS, 7),
		ENUMS(BUS_F_PARAM_LIGHTS, 7),
		NUM_LIGHTS
	};

	GateProcessor gates[7];
	dsp::PulseGenerator pgOut[6];
	
	bool outs[6] = {}; 
	bool prevOuts[6] = {}; 
	bool doTrig = false;;
	bool out = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	MatrixCombiner() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		// step params
		for (int s = 0; s < 7; s++) {
			configSwitch(BUS_A_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Source %d", s +1), {"N/C", "Combined with A"});
			configSwitch(BUS_B_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Source %d", s +1), {"N/C", "Combined with B"});
			configSwitch(BUS_C_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Source %d", s +1), {"N/C", "Combined with C"});
			configSwitch(BUS_D_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Source %d", s +1), {"N/C", "Combined with D"});
			configSwitch(BUS_E_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Source %d", s +1), {"N/C", "Combined with E"});
			configSwitch(BUS_F_PARAMS + s, 0.0f, 1.0f, 0.0f, rack::string::f("Source %d", s +1), {"N/C", "Combined with F"});

			configInput(GATE_INPUTS + s, rack::string::f("Source %d", s +1));
		}
		
		char c = 'A';
		for (int i = 0; i < NUM_OUTPUTS; i++) {
			configOutput(A_OUTPUT + i, rack::string::f("Destination %c", c++));
		}
		
		configSwitch(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Output mode", {"Gates", "Triggers"});
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
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
	
	void onReset() override {
		for (int i = 0; i < 7; i++) {
			gates[i].reset();
		}
		
		for (int i = 0; i < 6; i++) {
			pgOut[i].reset();
			prevOuts[i] = false;
			outs[i] = false;
		}
	}	
	
	void process(const ProcessArgs &args) override {
		
		for (int i = 0; i < 6; i++)
			outs[i] = false;

		for (int i = 0; i < 7; i++) {
		
			gates[i].set(inputs[i].getVoltage());
			
			// determine the output values
			if (gates[i].high()) {
				outs[0] |= (params[BUS_A_PARAMS + i].getValue() > 0.5f);
				outs[1] |= (params[BUS_B_PARAMS + i].getValue() > 0.5f);
				outs[2] |= (params[BUS_C_PARAMS + i].getValue() > 0.5f);
				outs[3] |= (params[BUS_D_PARAMS + i].getValue() > 0.5f);
				outs[4] |= (params[BUS_E_PARAMS + i].getValue() > 0.5f);
				outs[5] |= (params[BUS_F_PARAMS + i].getValue() > 0.5f);
			}
		}
		
		doTrig = (params[MODE_PARAM].getValue() > 0.5f);
	
		// set the outputs and lights accordingly
		for (int i = 0; i < 6; i++) {
			
			// save this so we can override it doing triggers
			out = outs[i];
			
			if (doTrig) {
				// doing triggers, override the value if we need to
				if (!prevOuts[i] && outs[i])
					pgOut[i].trigger(1e-3f);
				else
					out = pgOut[i].process(args.sampleTime);
			}
			else {
				// not doing triggers, reset the trigger pulse timer
				pgOut[i].reset();
			}
			
			outputs[A_OUTPUT + i].setVoltage(boolToGate(out));	
			lights[A_LIGHT + i].setSmoothBrightness(boolToLight(out), args.sampleTime);	
			
			prevOuts[i] = outs[i];
		}
	}
};

struct MatrixCombinerWidget : ModuleWidget {

	std::string panelName;
	
	MatrixCombinerWidget(MatrixCombiner *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs and buttons
		for (int s = 0; s < 7; s++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + s]), module, MatrixCombiner::GATE_INPUTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, STD_ROWS8[STD_ROW1 + s]), module, MatrixCombiner::BUS_A_PARAMS + s, MatrixCombiner::BUS_A_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW1 + s]), module, MatrixCombiner::BUS_B_PARAMS + s, MatrixCombiner::BUS_B_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL4] + 30, STD_ROWS8[STD_ROW1 + s]), module, MatrixCombiner::BUS_C_PARAMS + s, MatrixCombiner::BUS_C_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 40, STD_ROWS8[STD_ROW1 + s]), module, MatrixCombiner::BUS_D_PARAMS + s, MatrixCombiner::BUS_D_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 50, STD_ROWS8[STD_ROW1 + s]), module, MatrixCombiner::BUS_E_PARAMS + s, MatrixCombiner::BUS_E_PARAM_LIGHTS + s));
			addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL7] + 60, STD_ROWS8[STD_ROW1 + s]), module, MatrixCombiner::BUS_F_PARAMS + s, MatrixCombiner::BUS_F_PARAM_LIGHTS + s));
		}
		
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW8]), module, MatrixCombiner::MODE_PARAM));

		// output lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, STD_HALF_ROWS8(STD_ROW7)), module, MatrixCombiner::A_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_HALF_ROWS8(STD_ROW7)), module, MatrixCombiner::B_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL4] + 30, STD_HALF_ROWS8(STD_ROW7)), module, MatrixCombiner::C_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 40, STD_HALF_ROWS8(STD_ROW7)), module, MatrixCombiner::D_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 50, STD_HALF_ROWS8(STD_ROW7)), module, MatrixCombiner::E_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7] + 60, STD_HALF_ROWS8(STD_ROW7)), module, MatrixCombiner::F_LIGHT));

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 10, STD_ROWS8[STD_ROW8]), module, MatrixCombiner::A_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW8]), module, MatrixCombiner::B_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4] + 30, STD_ROWS8[STD_ROW8]), module, MatrixCombiner::C_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 40, STD_ROWS8[STD_ROW8]), module, MatrixCombiner::D_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 50, STD_ROWS8[STD_ROW8]), module, MatrixCombiner::E_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7] + 60, STD_ROWS8[STD_ROW8]), module, MatrixCombiner::F_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		MatrixCombiner *module = dynamic_cast<MatrixCombiner*>(this->module);
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

Model *modelMatrixCombiner = createModel<MatrixCombiner, MatrixCombinerWidget>("MatrixCombiner");
