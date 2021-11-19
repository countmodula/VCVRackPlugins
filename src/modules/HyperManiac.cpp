//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Hyper Maniacal LFO Control Expander Module
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/Utility.hpp"
#include "../inc/MegalomaniacControllerMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Megalomaniac
#define PANEL_FILE "Megalomaniac.svg"

struct Megalomaniac : Module {
	enum ParamIds {
		ENUMS(RATECV_PARAMS, 6),
		ENUMS(MIX_PARAMS, 6),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(RATECV_INPUTS, 6),
		ENUMS(RANGECV_INPUTS, 6),
		ENUMS(WAVECV_INPUTS, 6),
		ENUMS(MIXCV_INPUTS, 6),
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(RANGE_LIGHTS, 6 * 3),
		ENUMS(WAVE_LIGHTS, 6 * 5),
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"

	MegalomaniacControllerMessage rightMessages[2][1]; // messages to right module (hyper maniacal LFO)

	MegalomaniacControllerMessage dummyCntrlrMessage;
	
	short updateCounter;
	
	Megalomaniac() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		std::string oscName;
		for (int i = 0; i < 6; i++) {
			oscName = "Oscillator " + std::to_string(i + 1);
			configParam(RATECV_PARAMS + i, -1.0f, 1.0f, 0.0f, oscName + " rate CV Amount");
			configParam(MIX_PARAMS + i, 0.0f, 1.0f, 1.0f, oscName + " mix");
			
			configInput(RATECV_INPUTS + i, oscName + " rate CV");
			configInput(RANGECV_INPUTS + i, oscName + " range CV");
			configInput(WAVECV_INPUTS + i, oscName + " wave select CV");
			configInput(MIXCV_INPUTS + i, oscName + " mix CV");
		}

		updateCounter = 512;

		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
	}

	json_t *dataToJson() override {
		json_t *root = json_object();
		
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

		// set up details for the LFO module
		MegalomaniacControllerMessage *messageToModule;
		if (rightExpander.module && isControllableModule(rightExpander.module))
			messageToModule = (MegalomaniacControllerMessage*)(rightExpander.module->leftExpander.producerMessage);
		else
			messageToModule = &dummyCntrlrMessage;
		
		for (int i = 0; i < 6; i++) {
			// wave select 0-2 = off, 2 -4 = Sin etc.
			if (inputs[WAVECV_INPUTS + i].isConnected())
				messageToModule->selectedWaveform[i] = 1 + (int)(clamp(inputs[WAVECV_INPUTS + i].getVoltage()/2.0f, 0.0f, 4.0f));
			else
				messageToModule->selectedWaveform[i] = 0;
				
			// range select 0-2 = U, 2-4 = L, 4-6 = H
			if (inputs[RANGECV_INPUTS +i].isConnected())
				messageToModule->selectedRange[i] = 1 + (int)(clamp(inputs[RANGECV_INPUTS + i].getVoltage()/2.0f, 0.0f, 2.0f));
			else
				messageToModule->selectedRange[i] = 0;
			
			// mix levels
			messageToModule->mixLevel[i] = clamp(inputs[MIXCV_INPUTS + i].getNormalVoltage(10.0), 0.0f, 10.0f) / 10.0f * params[MIX_PARAMS + i].getValue();
			
			// rate values
			messageToModule->fmValue[i] = clamp(inputs[RATECV_INPUTS + i].getVoltage(), -12.0f, 12.0f) * params[RATECV_PARAMS + i].getValue();
		}

		// no need to update the lights every sample
		if (++updateCounter > 512) {
			updateCounter = 0;
			for (int i = 0; i < 6; i ++) {
				for (int j = 0; j < 5; j ++) {
					lights[WAVE_LIGHTS + (i * 5) + j].setBrightness(boolToLight(messageToModule->selectedWaveform[i] == j + 1 ));
					
					if (j < 3)
						lights[RANGE_LIGHTS + (i * 3) + j].setBrightness(boolToLight(messageToModule->selectedRange[i] == j + 1 ));
				}
			}
		}
		
		// set up details for the expander
		if (rightExpander.module && isControllableModule(rightExpander.module))
			rightExpander.module->leftExpander.messageFlipRequested = true;		
	}
};

struct MegalomaniacWidget : ModuleWidget {
	
	std::string panelName;
	
	MegalomaniacWidget(Megalomaniac *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// LFO knobs
		{
			int i = 0, j = 0;

			// LFO 1
			addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 15, STD_ROWS6[STD_ROW1]), module, Megalomaniac::RATECV_PARAMS + i++));
			addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW1]), module, Megalomaniac::MIX_PARAMS + j++));

			// LFO 2
			addParam(createParamCentered<Potentiometer<OrangeKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 15, STD_ROWS6[STD_ROW2]), module, Megalomaniac::RATECV_PARAMS + i++));
			addParam(createParamCentered<Potentiometer<OrangeKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW2]), module, Megalomaniac::MIX_PARAMS + j++));

			// LFO 3
			addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 15, STD_ROWS6[STD_ROW3]), module, Megalomaniac::RATECV_PARAMS + i++));
			addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW3]), module, Megalomaniac::MIX_PARAMS + j++));

			// LFO 4
			addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 15, STD_ROWS6[STD_ROW4]), module, Megalomaniac::RATECV_PARAMS + i++));
			addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW4]), module, Megalomaniac::MIX_PARAMS + j++));

			// LFO 5
			addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 15, STD_ROWS6[STD_ROW5]), module, Megalomaniac::RATECV_PARAMS + i++));
			addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW5]), module, Megalomaniac::MIX_PARAMS + j++));

			// LFO 6
			addParam(createParamCentered<Potentiometer<VioletKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] - 15, STD_ROWS6[STD_ROW6]), module, Megalomaniac::RATECV_PARAMS + i++));
			addParam(createParamCentered<Potentiometer<VioletKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL5] + 15, STD_ROWS6[STD_ROW6]), module, Megalomaniac::MIX_PARAMS + j++));
		}
		
		// Other LFO related bits
		for (int i = 0; i < 6; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1 + i]), module, Megalomaniac::RATECV_INPUTS + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS6[STD_ROW1 + i]), module, Megalomaniac::MIXCV_INPUTS + i));
			
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW1 + i]), module, Megalomaniac::RANGECV_INPUTS + i));
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL9] - 10, STD_ROWS6[STD_ROW1 + i]), module, Megalomaniac::WAVECV_INPUTS + i));
			
			// range lights
			int k = -10;
			for (int j = 0; j < 3; j ++) {
				addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL7] + 20, STD_ROWS6[STD_ROW1 + i] + k), module, Megalomaniac::RANGE_LIGHTS + (i * 3) + j));
				k += 10;
			}
			
			// wave lights
			k = -25;
			for (int j = 0; j < 5; j ++) {
				addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL9] + 10, STD_ROWS6[STD_ROW1 + i] + k), module, Megalomaniac::WAVE_LIGHTS + (i * 5) + j));
				k += 10;
			}
		}
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Megalomaniac *module = dynamic_cast<Megalomaniac*>(this->module);
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

Model *modelMegalomaniac = createModel<Megalomaniac, MegalomaniacWidget>("Megalomaniac");
