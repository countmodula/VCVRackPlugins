//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Manual Gate Module
//	A simple manual gate generator with a nice big button offering gate, latch
//	extended gate and trigger outputs 
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Mute
#define PANEL_FILE "Mute.svg"

struct Mute : Module {
	enum ParamIds {
		MUTE_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		L_INPUT,
		R_INPUT,
		MUTE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		L_OUTPUT,
		R_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		MUTE_LIGHT,
		NUM_LIGHTS
	};

	GateProcessor gate;
	LagProcessor slew;
	bool latch = false;

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	Mute() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Hard/Soft Mute");
		configParam(MUTE_PARAM, 0.0f, 1.0f, 0.0f, "Mute");

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		latch = false;
		gate.reset();
		slew.reset();
	}	
	
	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		json_object_set_new(root, "Latch", json_boolean(latch));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		

		return root;
	}	

	void dataFromJson(json_t* root) override {
		json_t* jsonLatch = json_object_get(root, "Latch");
		
		if (jsonLatch) {
			latch = json_is_true(jsonLatch);
		}
	
		// grab the theme details
		#include "../themes/dataFromJson.hpp"	
	}
		
	void process(const ProcessArgs &args) override {
		
		// set the mute state based on the input or the mute button. input takes precedence
		if (inputs[MUTE_INPUT].isConnected()) {
			gate.set(inputs[MUTE_INPUT].getVoltage());
			latch = gate.high();
		}
		else {
			gate.set(params[MUTE_PARAM].getValue() * 10.0f);
			if (gate.leadingEdge()) {
				latch = !latch;
			}
		}
		
		// calculate the mute factor
		float mute = (latch ? 0.0f : 1.0f);
		if (params[MODE_PARAM].getValue() > 0.5) {
			// soft mode - apply some slew to soften the switch
			mute = slew.process(mute, 1.0f, 0.1f, 0.1f);
		}
		else {
			// hard mode - keep slew in sync but don't use it
			slew.process(mute, 1.0f, 0.01f, 0.01f);
		}

		lights[MUTE_LIGHT].setSmoothBrightness(boolToLight(latch), args.sampleTime);
		
		// process the signals
		int numChannelsL = inputs[L_INPUT].getChannels();
		outputs[L_OUTPUT].setChannels(numChannelsL);
		for (int c = 0; c < numChannelsL; c++)
			outputs[L_OUTPUT].setVoltage(inputs[L_INPUT].getVoltage(c) * mute, c);
		
		int numChannelsR = inputs[R_INPUT].getChannels();
		outputs[R_OUTPUT].setChannels(numChannelsR);
		for (int c = 0; c < numChannelsR; c++)
			outputs[R_OUTPUT].setVoltage(inputs[R_INPUT].getVoltage(c) * mute, c);
	}
};

struct MuteWidget : ModuleWidget {
	MuteWidget(Mute *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mute.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// inputs	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, Mute::L_INPUT));	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, Mute::R_INPUT));	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, Mute::MUTE_INPUT));	

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, Mute::L_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, Mute::R_OUTPUT));	

		// switches
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, Mute::MODE_PARAM));

		// lights
		addChild(createLightCentered<LargeLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW4]), module, Mute::MUTE_LIGHT));

		// Mega mute button - non-standard position
		addParam(createParamCentered<CountModulaPBSwitchMegaMomentaryUnlit>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS8[STD_ROW7]), module, Mute::MUTE_PARAM));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Mute *module = dynamic_cast<Mute*>(this->module);
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

Model *modelMute = createModel<Mute, MuteWidget>("Mute");
