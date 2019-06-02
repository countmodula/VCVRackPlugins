//----------------------------------------------------------------------------
//	Count Modula - Analogue Shift Register Module
//	A Dual 4 output/ single 8 output analaogue shift register 
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"

struct ASR {
	static const int MAX_LEN = 16;
	
	GateProcessor trigger;
	float out[MAX_LEN];	
	int num_taps;
		
	void process(float clock, float cv) {
	
		trigger.set(clock);
		
		// only on clock transition from 0 to 1
		if (trigger.leadingEdge()) {
			// shift current values along 1
			for (int i = num_taps-1; i > 0; i--)
				out[i] = out[i-1];
			
			// insert new value at start
			out[0] = cv;
		}
	}
	
	float lastStep() {
		return out[num_taps-1];
	}
	
	void reset() {
		for (int i = 0; i < MAX_LEN; i++)
			out[i] = 0.0f;
	}
	
	ASR() {
		num_taps = MAX_LEN;
		reset();
	}
	
	ASR(int len_) {
		num_taps = MAX_LEN;
		
		if (len_ > 0 && len_ < MAX_LEN) {
			num_taps = len_;
			reset();
		}
	}
};

struct AnalogueShiftRegister : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		CH1_SIGNAL_INPUT,
		CH1_CLOCK_INPUT,
		CH2_SIGNAL_INPUT,
		CH2_CLOCK_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CH1_A_OUTPUT,
		CH1_B_OUTPUT,
		CH1_C_OUTPUT,
		CH1_D_OUTPUT,
		CH2_A_OUTPUT,
		CH2_B_OUTPUT,
		CH2_C_OUTPUT,
		CH2_D_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};
	
	ASR a {4};
	ASR b {4};
	
	AnalogueShiftRegister() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}
	void step() override;
	
	
	void onReset() override {
		a.reset();
		a.reset();
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - toJson, fromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};

void AnalogueShiftRegister::step() {

	float in2 = inputs[CH2_SIGNAL_INPUT].normalize(a.lastStep());
	float in1 = inputs[CH1_SIGNAL_INPUT].value;
	
	float clock1 = inputs[CH1_CLOCK_INPUT].value;
	float clock2 = inputs[CH2_CLOCK_INPUT].normalize(clock1);
	
	a.process(clock1, in1);
	b.process(clock2, in2);
	
	outputs[CH1_A_OUTPUT].value = a.out[0];
	outputs[CH1_B_OUTPUT].value = a.out[1];
	outputs[CH1_C_OUTPUT].value = a.out[2];
	outputs[CH1_D_OUTPUT].value = a.out[3];
	
	outputs[CH2_A_OUTPUT].value = b.out[0];
	outputs[CH2_B_OUTPUT].value = b.out[1];
	outputs[CH2_C_OUTPUT].value = b.out[2];
	outputs[CH2_D_OUTPUT].value = b.out[3];
}

struct AnalogueShiftRegisterWidget : ModuleWidget {
	AnalogueShiftRegisterWidget(AnalogueShiftRegister *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/AnalogueShiftRegister.svg")));

		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		for (int i = 0; i < 2; i++) {
			// clock and cv inputs
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + (i * 4)]), module, AnalogueShiftRegister::CH1_SIGNAL_INPUT + (i * 2)));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2 + (i * 4)]), module, AnalogueShiftRegister::CH1_CLOCK_INPUT + (i * 2)));
				
			// CV Outputs
			for (int j = 0; j < 4; j++) {
				addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[(STD_ROW1 + j)  + (i * 4)]), module, AnalogueShiftRegister::CH1_A_OUTPUT + j +(i * 4)));
			}
		}
	}
};

// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per plugin, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
Model *modelAnalogueShiftRegister = Model::create<AnalogueShiftRegister, AnalogueShiftRegisterWidget>("Count Modula", "AnalogueShiftRegister", "Analogue Shift Register", SAMPLE_AND_HOLD_TAG, SEQUENCER_TAG);
