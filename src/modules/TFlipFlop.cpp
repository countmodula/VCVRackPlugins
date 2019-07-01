//----------------------------------------------------------------------------
//	/^M^\ Count Modula - SR Flip Flop (Latch) Logic Gate Module
//	A dual set/reset latch with gate enable
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

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
};

struct TFlipFlop : Module {
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

	TLatch flipflop[2];
	
	TFlipFlop() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}
	
	void onReset() override {
		flipflop[0].reset();
		flipflop[1].reset();
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

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

struct TFlipFlopWidget : ModuleWidget {
	TFlipFlopWidget(TFlipFlop *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TFlipFlop.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		for (int i = 0; i < 2; i++) {
			int j = i * 3;
			// clock and reset input
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + j]), module, TFlipFlop::S_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2 + j]), module, TFlipFlop::ENABLE_INPUT + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3 + j]), module, TFlipFlop::R_INPUT + i));
			
			// outputs
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + j]), module, TFlipFlop::Q_OUTPUT + i));
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3 + j]), module, TFlipFlop::NQ_OUTPUT + i));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2 + j]), module, TFlipFlop::STATE_LIGHT + i));
		}
	}
};

Model *modelTFlipFlop = createModel<TFlipFlop, TFlipFlopWidget>("TFlipFlop");
