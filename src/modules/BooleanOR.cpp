//----------------------------------------------------------------------------
//	Count Modula - OR Logic Gate Module
//	A 4 input OR gate logic module with inverted output
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/GateProcessor.hpp"


struct OrGate {
	GateProcessor a;
	GateProcessor b;
	GateProcessor c;
	GateProcessor d;

	bool isHigh = false;
	
	float process (float aIn, float bIn, float cIn, float dIn) {
		a.set(aIn);
		b.set(bIn);
		c.set(cIn);
		d.set(dIn);

		// process the OR function
		isHigh = a.high()|| b.high() || c.high() || d.high();
		
		return boolToGate(isHigh);
	}
	
	void reset() {
		a.reset();
		b.reset();
		c.reset();
		d.reset();
		
		isHigh = false;
	}	
};

struct BooleanOR : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		A_INPUT,
		B_INPUT,
		C_INPUT,
		D_INPUT,
		I_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		OR_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	OrGate gate;
	Inverter inverter;
	
	BooleanOR() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	
	void step() override;

	void onReset() override {
		gate.reset();
		inverter.reset();
	}
	
	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void BooleanOR::step() {
	// grab and normalise the inputs
	float inA = inputs[A_INPUT].normalize(0.0f);
	float inB = inputs[B_INPUT].normalize(inA);
	float inC = inputs[C_INPUT].normalize(inB);
	float inD = inputs[D_INPUT].normalize(inC);
	
	//perform the logic
	float out =  gate.process(inA, inB, inC, inD);
	outputs[OR_OUTPUT].value = out;
	
	float notOut = inverter.process(inputs[I_INPUT].normalize(out));
	outputs[INV_OUTPUT].value = notOut;
}

struct BooleanORWidget : ModuleWidget {
BooleanORWidget(BooleanOR *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/BooleanOR.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]),module, BooleanOR::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]),module, BooleanOR::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]),module, BooleanOR::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]),module, BooleanOR::D_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]),module, BooleanOR::I_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, BooleanOR::OR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, BooleanOR::INV_OUTPUT));
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelBooleanOR = Model::create<BooleanOR, BooleanORWidget>("Count Modula", "BooleanOR", "Logical OR Gate", LOGIC_TAG);
