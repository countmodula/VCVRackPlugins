//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Manual Gate Module
//	A simple manual gate generator with a nice big button offering gate, latch
//	extended gate and trigger outputs 
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/PulseModifier.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME ManualGate
#define PANEL_FILE "ManualGate.svg"

struct ManualGate : Module {
	enum ParamIds {
		GATE_PARAM,
		LENGTH_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	
	enum OutputIds {
		GATE_OUTPUT,
		IGATE_OUTPUT,
		EXTENDED_OUTPUT,
		IEXT_OUTPUT,
		TRIG_OUTPUT,
		LATCH_OUTPUT,
		ILATCH_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LATCH_LIGHT,
		NUM_LIGHTS
	};
	
	GateProcessor gate;
	dsp::PulseGenerator  pgTrig;
	PulseModifier pmGate;
	
	bool latch = false;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	ManualGate() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(LENGTH_PARAM, 0.0f, 10.0f, 0.0f, "Output gate length");
		configParam(GATE_PARAM, 0.0f, 1.0f, 0.0f, "Gate on");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		latch = false;
		gate.reset();
		pgTrig.reset();
		pmGate.reset();
	}	
	
	json_t* dataToJson() override {
		json_t* root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		json_object_set_new(root, "Latch", json_boolean(latch));
			
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}	
	

	void dataFromJson(json_t* root) override {
		json_t* jsonLatch = json_object_get(root, "Latch");
		if (jsonLatch)
			latch = json_is_true(jsonLatch);
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"

	}	
	
	void process(const ProcessArgs &args) override {
		
		gate.set(params[GATE_PARAM].getValue() * 10.0f);
		
		if (gate.leadingEdge()) {
			latch = !latch;
		
			// fire off a trigger pulse
			pgTrig.trigger(1e-3f);
		}
		
		
		pmGate.set(params[LENGTH_PARAM].getValue());
		
		if (gate.high()) {
			// reset sets the output low and disables the timer
			pmGate.restart();
		}
		
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(pgTrig.process(args.sampleTime)));
		
		
		outputs[GATE_OUTPUT].setVoltage(gate.value());
		outputs[IGATE_OUTPUT].setVoltage(gate.notValue());
		
		outputs[LATCH_OUTPUT].setVoltage(boolToGate(latch));
		outputs[ILATCH_OUTPUT].setVoltage(boolToGate(!latch));

		bool ext = (gate.high() || pmGate.process(args.sampleTime));
		outputs[EXTENDED_OUTPUT].setVoltage(boolToGate(ext));
		outputs[IEXT_OUTPUT].setVoltage(boolToGate(!ext));
		
		lights[LATCH_LIGHT].setSmoothBrightness(boolToLight(latch), args.sampleTime);
	}
};

struct ManualGateWidget : ModuleWidget {
	ManualGateWidget(ManualGate *module) {	
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/ManualGate.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		// knobs
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, ManualGate::LENGTH_PARAM));

	
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, ManualGate::GATE_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, ManualGate::IGATE_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, ManualGate::LATCH_OUTPUT));		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, ManualGate::ILATCH_OUTPUT));		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, ManualGate::EXTENDED_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, ManualGate::IEXT_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, ManualGate::TRIG_OUTPUT));	
		
		// lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW2]), module, ManualGate::LATCH_LIGHT));

		// Mega manual button - non-standard position
		addParam(createParamCentered<CountModulaPBSwitchMegaMomentary>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW7]), module, ManualGate::GATE_PARAM));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		ManualGate *module = dynamic_cast<ManualGate*>(this->module);
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

Model *modelManualGate = createModel<ManualGate, ManualGateWidget>("ManualGate");
