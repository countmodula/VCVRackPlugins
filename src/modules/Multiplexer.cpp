//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - XOR Logic Gate Module
//	A 1-8 8-1 multiplexer module / 1 to 8 router / 8 to 1 switcher
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Multiplexer
#define PANEL_FILE "Multiplexer.svg"

struct Multiplexer : Module {
	enum ParamIds {
		ROUTER_LENGTH_PARAM,
		SELECTOR_LENGTH_PARAM,
		HOLD_PARAM,
		NORMAL_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		ROUTER_CLOCK_INPUT,
		SELECTOR_CLOCK_INPUT,
		ROUTER_RESET_INPUT,
		SELECTOR_RESET_INPUT,
		ROUTER_LENGTH_INPUT,
		SELECTOR_LENGTH_INPUT,
		ROUTER_INPUT,
		ENUMS(SELECTOR_INPUTS, 8),
		NUM_INPUTS
	};	
	enum OutputIds {
		SELECTOR_OUTPUT,
		ENUMS(ROUTER_OUTPUTS, 8),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(ROUTER_LIGHTS, 8),
		ENUMS(SELECTOR_LIGHTS, 8),
		NUM_LIGHTS
	};

	enum NormallingMode {
		INPUT_MODE,
		ZERO_MODE,
		ASSOC_MODE,
		MULTI_MODE
	};
	
	enum sampleMode {
		TRACKANDHOLD_MODE,
		TRACKONLY_MODE,
		SAMPLEANDHOLD_MODE
	};
	
	int routerIndex = -1;
	int selectorIndex = -1;
	
	GateProcessor gateRouterClock;
	GateProcessor gateSelectorClock;
	GateProcessor gateRouterReset;
	GateProcessor gateSelectorReset;
	
	bool linkedClock = true;
	
	int routerLength = 7;
	int selectorLength = 7;
	int normallingMode = 1;
	int holdMode = 1;
	
	float routerOutputs[8];

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Multiplexer() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configSwitch(ROUTER_LENGTH_PARAM, 1.0f, 7.0f, 7.0f, "Number of router steps (Sends)", {"2", "3", "4", "5", "6", "7", "8"});
		configSwitch(HOLD_PARAM, 0.0f, 2.0f, 1.0f, "Router sample mode",{"Track & Hold", "Through", "Sample & Hold"});
		
		configSwitch(SELECTOR_LENGTH_PARAM, 1.0f, 8.0f, 8.0f, "Number of selector steps (Receives)",  {"2", "3", "4", "5", "6", "7", "8", "Link to Router"});
		configSwitch(NORMAL_PARAM, 1.0f, 4.0f, 1.0f, "Selector normalling mode", {"Router input", "0 Volts", "Associated router output", "Selected router output"});

		for (int i = 0; i < 8; i++) {
			configInput(SELECTOR_INPUTS + i, rack::string::f("Selector recieve %d", i + 1));
			configOutput(ROUTER_OUTPUTS + i, rack::string::f("Router send %d", i + 1));
		}
		
		configInput(ROUTER_CLOCK_INPUT, "Selector clock");
		configInput(SELECTOR_CLOCK_INPUT, "Router clock");
		configInput(ROUTER_RESET_INPUT, "Selector reset");
		configInput(SELECTOR_RESET_INPUT, "Router reset");
		configInput(ROUTER_LENGTH_INPUT, "Selector length CV");
		configInput(SELECTOR_LENGTH_INPUT, "Router length CV");
		
		configInput(ROUTER_INPUT, "Router");
		configOutput(SELECTOR_OUTPUT, "Selector");
		
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
		routerIndex = -1;
		selectorIndex = -1;
		routerLength = 7;
		selectorLength = 7;
		
		gateRouterClock.reset();
		gateSelectorClock.reset();
		gateRouterReset.reset();
		gateSelectorReset.reset();	
	}
	
	void process(const ProcessArgs &args) override {
		
		// flag if the router clock also clocks the selector
		linkedClock = !inputs[SELECTOR_CLOCK_INPUT].isConnected();
		
		// what hold and normalling modes are selected?
		normallingMode = ((int)(params[NORMAL_PARAM].getValue())) - 1;
		holdMode = (int)(params[HOLD_PARAM].getValue());
		bool doSample = holdMode != SAMPLEANDHOLD_MODE;
		
		// determine the sequence routerLength
		routerLength = (int)(params[ROUTER_LENGTH_PARAM].getValue());
		selectorLength = (int)(params[SELECTOR_LENGTH_PARAM].getValue());
		float cvRouterLength = clamp(inputs[ROUTER_LENGTH_INPUT].getVoltage(), 0.0f, 10.0f);
		float cvSelectorLength = clamp(inputs[SELECTOR_LENGTH_INPUT].getVoltage(), 0.0f, 10.0f);
		
		// cv overrides switch position;
		if(inputs[ROUTER_LENGTH_INPUT].isConnected()) {
			// scale cv input to 1-7
			routerLength = (int)(1.0f + (cvRouterLength * 0.6f));
		}

		if(inputs[SELECTOR_LENGTH_INPUT].isConnected()) {
			// scale cv input to 1-7
			selectorLength = (int)(1.0f + (cvSelectorLength * 0.6f));
		}

		// sync to router has been selected
		if (selectorLength > 7)
			selectorLength = routerLength;
		
		// handle the reset inputs
		float routerReset = inputs[ROUTER_RESET_INPUT].getNormalVoltage(0.0f);
		float selectoReset = inputs[SELECTOR_RESET_INPUT].getNormalVoltage(routerReset);
		gateRouterReset.set(routerReset);
		gateSelectorReset.set(selectoReset);
		
		// grab the clock input values
		float routerClock = inputs[ROUTER_CLOCK_INPUT].getVoltage();
		float selectorClock = linkedClock ? routerClock : inputs[SELECTOR_CLOCK_INPUT].getVoltage();
		gateRouterClock.set(routerClock);
		gateSelectorClock.set(selectorClock);
		
		// router reset/clock logic
		if (gateRouterReset.high()) {
			// stop all outputs and reset to start
			routerIndex = -1;
		}
		else {
			// advance router clock if required
			if (gateRouterClock.leadingEdge()) {
				routerIndex = (routerIndex < routerLength ? routerIndex + 1 : 0);
				doSample = true;	
			}
		}
		
		// selector reset/clock logic
		if (gateSelectorReset.high()) {
			// stop all outputs and reset to start
			selectorIndex = -1;
		}
		else {
			if ((linkedClock && gateRouterClock.leadingEdge()) || (!linkedClock && gateSelectorClock.leadingEdge())) {
				// advance selector clock if required
				selectorIndex = (selectorIndex < selectorLength ? selectorIndex + 1 : 0);
			}	
		}

		// grab the send input  value	
		float sendIn = inputs[ROUTER_INPUT].getVoltage();
		
		// now send the input to the appropriate router output and the appropriate selector input to the output
		for (int i = 0; i < 8; i ++) {
			switch (holdMode) {
				case SAMPLEANDHOLD_MODE:
					if (doSample) {						
						routerOutputs[i] = (i == routerIndex ? sendIn : routerOutputs[i]);
						outputs[ROUTER_OUTPUTS + i].setVoltage(i == routerIndex ? sendIn : routerOutputs[i]);
					}
					break;
				case TRACKANDHOLD_MODE:
						routerOutputs[i] = (i == routerIndex ? sendIn : routerOutputs[i]);
						outputs[ROUTER_OUTPUTS + i].setVoltage(i == routerIndex ? sendIn : routerOutputs[i]);
					break;
				case TRACKONLY_MODE:
				default:
					routerOutputs[i] = (i == routerIndex ? sendIn : 0.0f);
					outputs[ROUTER_OUTPUTS + i].setVoltage(i == routerIndex ? sendIn : 0.0f);
					break;
			}
			lights[ROUTER_LIGHTS + i].setBrightness(i == routerIndex ? 1.0f : 0.0f);
		}
		
		for (int i = 0; i < 8; i ++) {
			// receives
			if (i == selectorIndex) {
				float normalVal = 0.0f;
				switch (normallingMode) {
					case MULTI_MODE:
						normalVal = routerOutputs[routerIndex];
						break;
					case ASSOC_MODE:
						normalVal = routerOutputs[i];
						break;
					case INPUT_MODE:
						normalVal = sendIn;
						break;
					default:
					case ZERO_MODE:
						normalVal = 0.0f;
						break;
				}
				
				if (inputs[SELECTOR_INPUTS + i].isConnected())
					outputs[SELECTOR_OUTPUT].setVoltage(inputs[SELECTOR_INPUTS + i].getVoltage());
				else
					outputs[SELECTOR_OUTPUT].setVoltage(normalVal);

				lights[SELECTOR_LIGHTS + i].setBrightness(1.0f );
			}
			else			
				lights[SELECTOR_LIGHTS + i].setBrightness(0.0f );
		}
	}
};

struct MultiplexerWidget : ModuleWidget {

	std::string panelName;
	
	MultiplexerWidget(Multiplexer *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		//--------------------------------------------------------
		// router section
		//--------------------------------------------------------
		addParam(createParamCentered<RotarySwitch<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW7)), module, Multiplexer::ROUTER_LENGTH_PARAM));
		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_HALF_ROWS8(STD_ROW2)), module, Multiplexer::HOLD_PARAM));
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, Multiplexer::ROUTER_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, Multiplexer::ROUTER_CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW5]), module, Multiplexer::ROUTER_RESET_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW6]), module, Multiplexer::ROUTER_LENGTH_INPUT));
			
		for (int i = 0; i < 8; i++) {
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::ROUTER_LIGHTS + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::ROUTER_OUTPUTS + i));
		}
	
		//--------------------------------------------------------
		// selector section
		//--------------------------------------------------------
		for (int i = 0; i < 8; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::SELECTOR_INPUTS + i));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS8[STD_ROW1 + i]), module, Multiplexer::SELECTOR_LIGHTS + i));
		}

		addParam(createParamCentered<RotarySwitch<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_HALF_ROWS8(STD_ROW2)), module, Multiplexer::SELECTOR_LENGTH_PARAM));
		addParam(createParamCentered<RotarySwitch<OperatingAngle145<YellowKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_HALF_ROWS8(STD_ROW6)), module, Multiplexer::NORMAL_PARAM));
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW1]), module, Multiplexer::SELECTOR_LENGTH_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW4]), module, Multiplexer::SELECTOR_CLOCK_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW5]), module, Multiplexer::SELECTOR_RESET_INPUT));
		
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9], STD_ROWS8[STD_ROW8]), module, Multiplexer::SELECTOR_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Multiplexer *module = dynamic_cast<Multiplexer*>(this->module);
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

Model *modelMultiplexer = createModel<Multiplexer, MultiplexerWidget>("Multiplexer");
