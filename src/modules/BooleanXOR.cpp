//----------------------------------------------------------------------------
//	/^M^\ Count Modula - XOR Logic Gate Module
//	A 4 input Exclusive-OR (XOR) gate logic module with inverted output
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/Inverter.hpp"
#include "../inc/GateProcessor.hpp"

struct XorGate {
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

		// process the XOR function
		int i = 0;
		if (a.high()) i++;
		if (b.high()) i++;
		if (c.high()) i++;
		if (d.high()) i++;
		
		isHigh = i == 1;
		
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

struct BooleanXOR : Module {
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
		XOR_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	XorGate gate;
	Inverter inverter;
	
	BooleanXOR() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		return root;
	}
	
	void onReset() override {
		gate.reset();
		inverter.reset();
	}
	
	void process(const ProcessArgs &args) override {
		// grab and normalise the inputs
		float inA = inputs[A_INPUT].getNormalVoltage(0.0f);
		float inB = inputs[B_INPUT].getNormalVoltage(0.0f);
		float inC = inputs[C_INPUT].getNormalVoltage(0.0f);
		float inD = inputs[D_INPUT].getNormalVoltage(0.0f);
		
		//perform the logic
		float out =  gate.process(inA, inB, inC, inD);
		outputs[XOR_OUTPUT].setVoltage(out);
		
		float notOut = inverter.process(inputs[I_INPUT].getNormalVoltage(out));
		outputs[INV_OUTPUT].setVoltage(notOut);
	}
};

struct BooleanXORWidget : ModuleWidget {
BooleanXORWidget(BooleanXOR *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BooleanXOR.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// clock and reset input
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW1]),module, BooleanXOR::A_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW2]),module, BooleanXOR::B_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW3]),module, BooleanXOR::C_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW4]),module, BooleanXOR::D_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW6]),module, BooleanXOR::I_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW5]), module, BooleanXOR::XOR_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS7[STD_ROW7]), module, BooleanXOR::INV_OUTPUT));
	}
};

Model *modelBooleanXOR = createModel<BooleanXOR, BooleanXORWidget>("BooleanXOR");
