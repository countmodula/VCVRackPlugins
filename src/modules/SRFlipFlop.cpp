//----------------------------------------------------------------------------
//	Count Modula - SR Flip Flop (Latch) Logic Gate Module
//	A dual set/reset latch with gate enable
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

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
	
	SRFlipFlop() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	void onReset() override {
		flipflop[0].reset();
		flipflop[1].reset();
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void SRFlipFlop::step() {
	
	for (int i = 0; i < 2; i++) {
		//perform the latch logic with the given inputs
		flipflop[i].process(inputs[S_INPUT + i].value, inputs[R_INPUT + i].value, inputs[ENABLE_INPUT + i].normalize(10.0f));
		
		// set outputs according to latch states
		outputs[Q_OUTPUT + i].value = flipflop[i].Q();
		lights[STATE_LIGHT + i].setBrightnessSmooth(flipflop[i].Q());
		outputs[NQ_OUTPUT + i].value = flipflop[i].NQ();
	}
}

struct SRFlipFlopWidget : ModuleWidget {
	SRFlipFlopWidget(SRFlipFlop *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/SRFlipFlop.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(Widget::create<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
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
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelSRFlipFlop = Model::create<SRFlipFlop, SRFlipFlopWidget>("Count Modula", "SRFlipFlop", "SR Flip Flop", LOGIC_TAG);
