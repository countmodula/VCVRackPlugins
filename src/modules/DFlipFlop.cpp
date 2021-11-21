//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - D Flip Flop Logic Gate Module
//	A D type flip flop with gate enable
//	Copyright (C) 2021  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SingleDFlipFlop
#define PANEL_FILE "DFF.svg"

// implements a basic D type flip flop
struct DLatch {
	GateProcessor D;
	GateProcessor C;
	GateProcessor E;

	bool stateQ = false; 
	bool stateNQ = true;

	float process (float dIn, float clockIn, float enIn) {
		D.set(dIn);
		C.set(clockIn);
		E.set(enIn);

		if (E.high()) {
			if (C.leadingEdge()) {
				stateQ = D.high();
				stateNQ = !stateQ;
			}
		}

		return boolToGate(stateQ);
	}

	void reset () {
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

struct SingleDFlipFlop : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		D_INPUT,
		CLOCK_INPUT,
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

	DLatch flipflop;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	SingleDFlipFlop() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configInput(D_INPUT, "D");
		configInput(CLOCK_INPUT, "Clock");
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
		flipflop.process(inputs[D_INPUT].getVoltage(), inputs[CLOCK_INPUT].getVoltage(), inputs[ENABLE_INPUT].getNormalVoltage(10.0f));
		
		// set outputs according to latch states
		outputs[Q_OUTPUT].setVoltage(flipflop.Q());
		lights[Q_LIGHT].setSmoothBrightness(flipflop.QLight(), args.sampleTime);
		outputs[NQ_OUTPUT].setVoltage(flipflop.NQ());
		lights[NQ_LIGHT].setSmoothBrightness(flipflop.NQLight(), args.sampleTime);
	}
};

struct SingleDFlipFlopWidget : ModuleWidget {

	std::string panelName;
	
	SingleDFlipFlopWidget(SingleDFlipFlop *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW1]), module, SingleDFlipFlop::D_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2]), module, SingleDFlipFlop::ENABLE_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3]), module, SingleDFlipFlop::CLOCK_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4]), module, SingleDFlipFlop::Q_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW5]), module, SingleDFlipFlop::NQ_OUTPUT));
		
		// lights
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 12.5, STD_ROWS5[STD_ROW4] - 20), module, SingleDFlipFlop::Q_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 12.5, STD_ROWS5[STD_ROW5] - 20), module, SingleDFlipFlop::NQ_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SingleDFlipFlop *module = dynamic_cast<SingleDFlipFlop*>(this->module);
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


Model *modelSingleDFlipFlop = createModel<SingleDFlipFlop, SingleDFlipFlopWidget>("SingleDFlipFlop");
