//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Standard Sequencer Channel Engine
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
struct STRUCT_NAME : Module {

	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(GATE_OUTPUTS, SEQ_NUM_STEPS),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(STEP_LIGHTS, SEQ_NUM_STEPS),
		NUM_LIGHTS
	};
	
	int count;
	float moduleVersion = 0.0f;
	
	// Expander details
	SequencerChannelMessage leftMessages[2][1];	// messages from left module (master)
	SequencerChannelMessage rightMessages[2][1]; // messages to right module (expander)
	SequencerChannelMessage *messagesFromMaster;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	STRUCT_NAME() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
		
		// from left module (master)
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];		
		
		// to right module (expander)
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];	
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_real(1.0f));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}
	
	void dataFromJson(json_t* root) override {
		
		// grab the theme details
		#include "../themes/dataFromJson.hpp"		
		
		json_t *ver = json_object_get(root, "moduleVersion");

		if (ver)
			moduleVersion = json_number_value(ver);
	}

	void process(const ProcessArgs &args) override {

		bool running = false;
		count = 0;

		// grab the detail from the left hand module if we have one
		messagesFromMaster = 0; 
		if (leftExpander.module) {
			if (isExpanderModule(leftExpander.module) || isExpandableModule(leftExpander.module)) {
					
				messagesFromMaster = (SequencerChannelMessage *)(leftExpander.consumerMessage);

				count = messagesFromMaster->counter;
				running = messagesFromMaster->runningState;
			}
		}
		
		// process the step switches, cv and set the length/active step lights etc
		for (int c = 0; c < SEQ_NUM_STEPS; c++) {
			// set step and length lights here
			bool stepActive = (c + 1 == count) && running;

			// now we can set the outputs and lights
			outputs[GATE_OUTPUTS + c].setVoltage(boolToGate(stepActive));
			lights[STEP_LIGHTS + c].setBrightness(boolToLight(stepActive));			
		}
		
		// finally set up the details for any secondary expander
		if (rightExpander.module) {
			if (isExpanderModule(rightExpander.module)) {
				
				SequencerChannelMessage *messageToExpander = (SequencerChannelMessage*)(rightExpander.module->leftExpander.producerMessage);
				
				if (messagesFromMaster) {
					int ch = 0;
					if (messagesFromMaster->hasMaster)
						ch = messagesFromMaster->channel;
						
					messageToExpander->set(messagesFromMaster->counter, messagesFromMaster->length, messagesFromMaster->clockState, messagesFromMaster->runningState, ch, messagesFromMaster->hasMaster);
				}
				else {
					messageToExpander->initialise();
				}

				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}			
	}
};

struct WIDGET_NAME : ModuleWidget {
	WIDGET_NAME(STRUCT_NAME *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" PANEL_FILE)));

		// screws
		#include "../components/stdScrews.hpp"	

		// step lights
		for (int s = 0; s < SEQ_NUM_STEPS; s++) {
			int col = s > 7 ? STD_COL3 : STD_COL1;
			addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[col], STD_ROWS8[STD_ROW1 + (s % 8)]), module, STRUCT_NAME::GATE_OUTPUTS + s));
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[col] + 12 , STD_ROWS8[STD_ROW1 + (s % 8)] - 19), module, STRUCT_NAME::STEP_LIGHTS + s));
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"
	
	void appendContextMenu(Menu *menu) override {
		STRUCT_NAME *module = dynamic_cast<STRUCT_NAME*>(this->module);
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

Model *MODEL_NAME = createModel<STRUCT_NAME, WIDGET_NAME>(MODULE_NAME);
