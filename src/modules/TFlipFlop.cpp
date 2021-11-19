//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - T Flip Flop (Latch) Logic Gate Module
//	A dual toggle latch with gate enable
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME TFlipFlop
#define PANEL_FILE "TFlipFlop.svg"

// implements a basic Toggle flip flop
struct TLatch {
	GateProcessor T;
	GateProcessor R;
	GateProcessor E;

	bool stateQ = false; 
	bool stateNQ = true;
	
	float process (float tIn, float resetIn, float enIn) {
		T.set(tIn);
		R.set(resetIn);
		E.set(enIn);

		if (E.high()) {
			if (R.high()) {
				stateQ = false;
				stateNQ = true;
			}
			else if (T.leadingEdge()) {
				stateQ = !stateQ;
				stateNQ = !stateNQ;
			}
		}
		
		return boolToGate(stateQ);
	}
	
	void reset () {
		T.reset();
		R.reset();
		E.reset();
		stateQ = false; 
		stateNQ = true;
	}
	
	// gets the current "Q" output
	float Q() {
		return boolToGate(stateQ);
	}
	
	// gets the current not Q output
	float NQ() {
		return boolToGate(stateNQ);
	}
	
	// gets the current "Q" output light value
	float QLight() {
		return boolToLight(stateQ);
	}
	
	// gets the current not Q output light value
	float NQLight() {
		return boolToLight(stateNQ);
	}
};

struct TFlipFlop : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(T_INPUT, 2),
		ENUMS(RESET_INPUT, 2),
		ENUMS(ENABLE_INPUT, 2),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(Q_OUTPUT, 2),
		ENUMS(NQ_OUTPUT, 2),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STATE_LIGHT, 2),
		NUM_LIGHTS
	};

	TLatch flipflop[2];
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	TFlipFlop() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		for (int i = 0; i < 2; i++) {
			configInput(T_INPUT + i, rack::string::f("Flip flop %d T", i + 1));
			configInput(RESET_INPUT + i, rack::string::f("Flip flop %d Reset", i + 1));
			configInput(ENABLE_INPUT + i, rack::string::f("Flip flop %d Enable", i + 1));
			
			configOutput(Q_OUTPUT + i, rack::string::f("Flip flop %d Q", i + 1));
			configOutput(NQ_OUTPUT + i, rack::string::f("Flip flop %d Not Q", i + 1));
		}

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		flipflop[0].reset();
		flipflop[1].reset();
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// flip flop Q states
		json_t *QStates = json_array();
		for (int i = 0; i < 2; i++) {
			json_array_insert_new(QStates, i, json_boolean(flipflop[i].stateQ));
		}
		json_object_set_new(root, "QStates", QStates);

		
		// flip flop NQ states
		json_t *NQStates = json_array();
		for (int i = 0; i < 2; i++) {
			json_array_insert_new(NQStates, i, json_boolean(flipflop[i].stateNQ));
		}
		json_object_set_new(root, "NQStates", NQStates);
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t *root) override {

		// flip flop Q states
		json_t *QStates = json_object_get(root, "QStates");
		
		if (QStates) {
			for (int i = 0; i < 2; i++) {
				json_t *state = json_array_get(QStates, i);
				
				flipflop[i].stateQ = json_is_true(state);
			}
		}
		
		// flip flop NQ states
		json_t *NQStates = json_object_get(root, "NQStates");
		
		if (NQStates) {
			for (int i = 0; i < 2; i++) {
				json_t *state = json_array_get(NQStates, i);
				
				flipflop[i].stateNQ = json_is_true(state);
			}
		}
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
	}	
	
	
	void process(const ProcessArgs &args) override {
		
		for (int i = 0; i < 2; i++) {
			//perform the latch logic with the given inputs
			flipflop[i].process(inputs[T_INPUT + i].getVoltage(), inputs[RESET_INPUT + i].getVoltage(), inputs[ENABLE_INPUT + i].getNormalVoltage(10.0f));
			
			// set outputs according to latch states
			outputs[Q_OUTPUT + i].setVoltage(flipflop[i].Q());
			lights[STATE_LIGHT + i].setSmoothBrightness(flipflop[i].QLight(), args.sampleTime);
			outputs[NQ_OUTPUT + i].setVoltage(flipflop[i].NQ());
		}
	}
};

struct TFlipFlopWidget : ModuleWidget {

	std::string panelName;
	
	TFlipFlopWidget(TFlipFlop *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		for (int i = 0; i < 2; i++) {
			int j = i * 3;
			// clock and reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + j]), module, TFlipFlop::T_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 + j]), module, TFlipFlop::ENABLE_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + j]), module, TFlipFlop::RESET_INPUT + i));
			
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, TFlipFlop::Q_OUTPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3 + j]), module, TFlipFlop::NQ_OUTPUT + i));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2 + j]), module, TFlipFlop::STATE_LIGHT + i));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		TFlipFlop *module = dynamic_cast<TFlipFlop*>(this->module);
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

// reset the module name for the theme selection functions
#undef THEME_MODULE_NAME
#undef PANEL_FILE
#define THEME_MODULE_NAME SingleTFlipFlop
#define PANEL_FILE "TFF.svg"

struct SingleTFlipFlop : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		T_INPUT,
		RESET_INPUT,
		ENABLE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		Q_OUTPUT,
		NQ_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		Q_LIGHT,
		NQ_LIGHT,
		NUM_LIGHTS
	};

	TLatch flipflop;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	SingleTFlipFlop() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configInput(T_INPUT, "T");
		configInput(RESET_INPUT, "Reset");
		configInput(ENABLE_INPUT, "Enable");

		configOutput(Q_OUTPUT, "Q");
		configOutput(NQ_OUTPUT, "Not Q");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		flipflop.reset();
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// flip flop states
		json_object_set_new(root, "QState", json_boolean(flipflop.stateQ));
		json_object_set_new(root, "NQState", json_boolean(flipflop.stateNQ));

		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t *root) override {

		// flip flop states
		json_t *QState = json_object_get(root, "QState");
		json_t *NQState = json_object_get(root, "NQState");
	
		if (QState)
			flipflop.stateQ = json_boolean_value(QState);

		if (NQState)
			flipflop.stateNQ =  json_boolean_value(NQState);
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
	}	
	
	
	void process(const ProcessArgs &args) override {

		//perform the latch logic
		flipflop.process(inputs[T_INPUT].getVoltage(), inputs[RESET_INPUT].getVoltage(), inputs[ENABLE_INPUT].getNormalVoltage(10.0f));
		
		// set outputs according to latch states
		outputs[Q_OUTPUT].setVoltage(flipflop.Q());
		lights[Q_LIGHT].setSmoothBrightness(flipflop.QLight(), args.sampleTime);
		outputs[NQ_OUTPUT].setVoltage(flipflop.NQ());
		lights[NQ_LIGHT].setSmoothBrightness(flipflop.NQLight(), args.sampleTime);
	}
};

struct SingleTFlipFlopWidget : ModuleWidget {

	std::string panelName;
	
	SingleTFlipFlopWidget(SingleTFlipFlop *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW1]), module, SingleTFlipFlop::T_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2]), module, SingleTFlipFlop::ENABLE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3]), module, SingleTFlipFlop::RESET_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4]), module, SingleTFlipFlop::Q_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW5]), module, SingleTFlipFlop::NQ_OUTPUT));
		
		// lights
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 12.5, STD_ROWS5[STD_ROW4] - 20), module, SingleTFlipFlop::Q_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 12.5, STD_ROWS5[STD_ROW5] - 20), module, SingleTFlipFlop::NQ_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SingleTFlipFlop *module = dynamic_cast<SingleTFlipFlop*>(this->module);
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


Model *modelTFlipFlop = createModel<TFlipFlop, TFlipFlopWidget>("TFlipFlop");
Model *modelSingleTFlipFlop = createModel<SingleTFlipFlop, SingleTFlipFlopWidget>("SingleTFlipFlop");
