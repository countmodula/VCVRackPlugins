//----------------------------------------------------------------------------
//	/^M^\ Count Modula - SR Flip Flop (Latch) Logic Gate Module
//	A dual set/reset latch with gate enable
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME SRFlipFlop
#define PANEL_FILE "SRFlipFlop.svg"

// implements a basic SR flip flop
struct SRLatch {
	GateProcessor S;
	GateProcessor R;
	GateProcessor E;

	bool stateQ = false; 
	bool stateNQ = true;
	
	float process (float sIn, float rIn, float enIn) {
		S.set(sIn);
		R.set(rIn);
		E.set(enIn);

		if (E.high()) {
			if(R.high() && S.high()) {
				// invalid state
				stateQ = stateNQ = true;
			}
			else if (R.high() && S.low()) {
				stateQ = false;
				stateNQ = true;
			}
			else if (R.low() && S.high()) {
				stateQ = true;
				stateNQ = false;
			}
		}
		
		return boolToGate(stateQ);
	}
	
	void reset () {
		S.reset();
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
};

struct SRFlipFlop : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(S_INPUT, 2),
		ENUMS(R_INPUT, 2),
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

	SRLatch flipflop[2];

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	SRFlipFlop() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		flipflop[0].reset();
		flipflop[1].reset();
	}


	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
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
			flipflop[i].process(inputs[S_INPUT + i].getVoltage(), inputs[R_INPUT + i].getVoltage(), inputs[ENABLE_INPUT + i].getNormalVoltage(10.0f));
			
			// set outputs according to latch states
			outputs[Q_OUTPUT + i].setVoltage(flipflop[i].Q());
			lights[STATE_LIGHT + i].setSmoothBrightness(flipflop[i].Q(), args.sampleTime);
			outputs[NQ_OUTPUT + i].setVoltage(flipflop[i].NQ());
		}
	}
};

struct SRFlipFlopWidget : ModuleWidget {
	SRFlipFlopWidget(SRFlipFlop *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SRFlipFlop.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		for (int i = 0; i < 2; i++) {
			int j = i * 3;
			// clock and reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + j]), module, SRFlipFlop::S_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 + j]), module, SRFlipFlop::ENABLE_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + j]), module, SRFlipFlop::R_INPUT + i));
			
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, SRFlipFlop::Q_OUTPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3 + j]), module, SRFlipFlop::NQ_OUTPUT + i));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2 + j]), module, SRFlipFlop::STATE_LIGHT + i));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		SRFlipFlop *module = dynamic_cast<SRFlipFlop*>(this->module);
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

Model *modelSRFlipFlop = createModel<SRFlipFlop, SRFlipFlopWidget>("SRFlipFlop");
