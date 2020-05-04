//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Analogue Shift Register Module
//	A Dual 4 output/ single 8 output analaogue shift register 
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME AnalogueShiftRegister
#define PANEL_FILE "AnalogueShiftRegister.svg"

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
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	AnalogueShiftRegister() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		a.reset();
		b.reset();
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_integer(2));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"
			
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}		
	
	void process(const ProcessArgs &args) override {

		float in2 = inputs[CH2_SIGNAL_INPUT].getNormalVoltage(a.lastStep());
		float in1 = inputs[CH1_SIGNAL_INPUT].getVoltage();
		
		float clock1 = inputs[CH1_CLOCK_INPUT].getVoltage();
		float clock2 = inputs[CH2_CLOCK_INPUT].getNormalVoltage(clock1);
		
		a.process(clock1, in1);
		b.process(clock2, in2);
		
		outputs[CH1_A_OUTPUT].setVoltage(a.out[0]);
		outputs[CH1_B_OUTPUT].setVoltage(a.out[1]);
		outputs[CH1_C_OUTPUT].setVoltage(a.out[2]);
		outputs[CH1_D_OUTPUT].setVoltage(a.out[3]);
		
		outputs[CH2_A_OUTPUT].setVoltage(b.out[0]);
		outputs[CH2_B_OUTPUT].setVoltage(b.out[1]);
		outputs[CH2_C_OUTPUT].setVoltage(b.out[2]);
		outputs[CH2_D_OUTPUT].setVoltage(b.out[3]);
	}	
};

struct AnalogueShiftRegisterWidget : ModuleWidget {
	AnalogueShiftRegisterWidget(AnalogueShiftRegister *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AnalogueShiftRegister.svg")));

		// screws
		#include "../components/stdScrews.hpp"	

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
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	void appendContextMenu(Menu *menu) override {
		AnalogueShiftRegister *module = dynamic_cast<AnalogueShiftRegister*>(this->module);
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

Model *modelAnalogueShiftRegister = createModel<AnalogueShiftRegister, AnalogueShiftRegisterWidget>("AnalogueShiftRegister");
