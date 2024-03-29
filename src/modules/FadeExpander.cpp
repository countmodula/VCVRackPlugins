//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Recording Fade Module Expander
//	Expander for the fade-in/fade-out module.
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"

#include "../inc/FadeExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME FadeExpander
#define PANEL_FILE "FadeExpander.svg"

struct FadeExpander : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENV_OUTPUT,
		GATE_OUTPUT,
		TRIG_OUTPUT,
		FI_OUTPUT,
		FO_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		GATE_LIGHT,
		TRIG_LIGHT,
		FI_LIGHT,
		FO_LIGHT,
		NUM_LIGHTS
	};

	dsp::PulseGenerator  pgTrig;
	bool prevRun = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	// Expander details
	FadeExpanderMessage leftMessages[2][1];	// messages from left module (master)
	FadeExpanderMessage *messagesFromMaster;
	bool leftModuleAvailable = false; 	
	
	FadeExpander() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configOutput(ENV_OUTPUT, "Envelope");
		configOutput(INV_OUTPUT, "Inverted envelope");
		configOutput(GATE_OUTPUT, "Run");
		configOutput(TRIG_OUTPUT, "Start/stop");
		configOutput(FI_OUTPUT, "Fading in");
		configOutput(FO_OUTPUT, "Fading out");

		outputInfos[GATE_OUTPUT]->description = "Outputs a gate signal that goes high at the start of fade-in and low at the start of fade out.";
		outputInfos[TRIG_OUTPUT]->description = "Outputs trigger pulses at the start of fade-in and start of fade-out";
		outputInfos[FI_OUTPUT]->description = "Gate signal that is high for the duration of fade-in";
		outputInfos[FO_OUTPUT]->description = "Gate signal that is high for the duration of fade-out";

		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];			
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		pgTrig.reset();
	}	
	
	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "moduleVersion", json_integer(1));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		

		return root;
	}	

	void dataFromJson(json_t* root) override {
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"	
	}
		
	void process(const ProcessArgs &args) override {
		
		float envelope = 0.0f;
		bool run = false;
		bool fadeIn = false;
		bool fadeOut = false;
	
		leftModuleAvailable = false;
		if (leftExpander.module) {
			if (leftExpander.module->model == modelFade) {
					
				leftModuleAvailable = true;
				messagesFromMaster = (FadeExpanderMessage *)(leftExpander.consumerMessage);				
				
				envelope = messagesFromMaster->envelope; 
				run = messagesFromMaster->run; 
				fadeIn = messagesFromMaster->fadeIn; 
				fadeOut = messagesFromMaster->fadeOut; 
			}
		}			
		
		if (run != prevRun)
			pgTrig.trigger(1e-3f);
		
		outputs[ENV_OUTPUT].setVoltage(envelope);
		outputs[INV_OUTPUT].setVoltage(-envelope);
		
		bool trig = pgTrig.remaining > 0.0f;
		pgTrig.process(args.sampleTime);
		outputs[GATE_OUTPUT].setVoltage(boolToGate(run));
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(trig));

		outputs[FI_OUTPUT].setVoltage(boolToGate(fadeIn));
		outputs[FO_OUTPUT].setVoltage(boolToGate(fadeOut));
		
	
		lights[GATE_LIGHT].setBrightness(boolToLight(run));
		lights[TRIG_LIGHT].setSmoothBrightness(boolToLight(trig), args.sampleTime);

		lights[FI_LIGHT].setSmoothBrightness(boolToLight(fadeIn), args.sampleTime);
		lights[FO_LIGHT].setSmoothBrightness(boolToLight(fadeOut), args.sampleTime);
		
		prevRun = run;
	}
};

struct FadeExpanderWidget : ModuleWidget {

	std::string panelName;
	
	FadeExpanderWidget(FadeExpander *module) {
		setModule(module);
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	

		// lights
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW3] - 19), module, FadeExpander::GATE_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW4] - 19), module, FadeExpander::TRIG_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW5] - 19), module, FadeExpander::FI_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW6] - 19), module, FadeExpander::FO_LIGHT));
	
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, FadeExpander::ENV_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, FadeExpander::INV_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, FadeExpander::GATE_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, FadeExpander::TRIG_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, FadeExpander::FI_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, FadeExpander::FO_OUTPUT));	
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		FadeExpander *module = dynamic_cast<FadeExpander*>(this->module);
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

Model *modelFadeExpander = createModel<FadeExpander, FadeExpanderWidget>("FadeExpander");
