//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - G2T Module
//	Gate To Trigger Module
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/Inverter.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME G2T
#define PANEL_FILE "G2T.svg"

struct G2T : Module {
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
		GATE_LIGHT,
		START_LIGHT,
		END_LIGHT,
		EDGE_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gate;
	dsp::PulseGenerator pgStart;
	dsp::PulseGenerator pgEnd;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	G2T() {
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
		gate.reset();
		pgStart.reset();
		pgEnd.reset();
	}
	
	void process(const ProcessArgs &args) override {
		
		// process the input
		gate.set(inputs[GATE_INPUT].getVoltage());

			// leading edge - fire the start trigger
		if (gate.leadingEdge())
			pgStart.trigger(1e-3f);

		// find leading/trailing edge
		if (gate.trailingEdge())
			pgEnd.trigger(1e-3f);
		
		// process the gate outputs
		if (gate.high()) {
			outputs[GATE_OUTPUT].setVoltage(10.0f); 
			outputs[INV_OUTPUT].setVoltage(0.0);
			lights[GATE_LIGHT].setBrightness(10.0f);
		}
		else {
			outputs[GATE_OUTPUT].setVoltage(0.0f); 
			outputs[INV_OUTPUT].setVoltage(10.0); 
			lights[GATE_LIGHT].setBrightness(0.0f);
		}
		
		bool sTrig = pgStart.remaining > 0.0f;
		bool eTrig = pgEnd.remaining > 0.0f;
		pgStart.process(args.sampleTime);
		pgEnd.process(args.sampleTime);
		
		// process the start trigger
		outputs[START_OUTPUT].setVoltage(boolToGate(sTrig));
		lights[START_LIGHT].setSmoothBrightness(boolToLight(sTrig), args.sampleTime);

		// process the end trigger
		outputs[END_OUTPUT].setVoltage(boolToGate(eTrig));
		lights[END_LIGHT].setSmoothBrightness(boolToLight(eTrig), args.sampleTime);
		
		// process the end trigger
		outputs[EDGE_OUTPUT].setVoltage(boolToGate(sTrig || eTrig));
		lights[EDGE_LIGHT].setSmoothBrightness(boolToLight(sTrig || eTrig), args.sampleTime);
		
	}
};

struct G2TWidget : ModuleWidget {

	std::string panelName;
	
	G2TWidget(G2T *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, G2T::GATE_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, G2T::GATE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, G2T::INV_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, G2T::START_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, G2T::EDGE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, G2T::END_OUTPUT));
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW2]), module, G2T::GATE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW4]), module, G2T::START_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW5]), module, G2T::EDGE_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW6]), module, G2T::END_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		G2T *module = dynamic_cast<G2T*>(this->module);
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

Model *modelG2T = createModel<G2T, G2TWidget>("G2T");
