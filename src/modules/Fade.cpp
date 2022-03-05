//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Recording Fade Module
//	A fade-in/fade-out and trigger controller for the VCV Rack Rec module.
//	Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../components/CountModulaLEDDisplay.hpp"
#include "../inc/GateProcessor.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/Utility.hpp"

#include "../inc/FadeExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Fade
#define OLD_PANEL_FILE "FadeV1.svg"
#define PANEL_FILE "Fade.svg"

struct Fade : Module {
	enum ParamIds {
		FADE_PARAM,
		IN_PARAM,
		OUT_PARAM,
		MON_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		L_INPUT,
		R_INPUT,
		CTRL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		L_OUTPUT,
		R_OUTPUT,
		GATE_OUTPUT,
		TRIG_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		L_LIGHT,
		R_LIGHT,
		GATE_LIGHT,
		TRIG_LIGHT,
		FADEIN_LIGHT,
		FADEOUT_LIGHT,
		FADE_PARAM_LIGHT,
		MON_PARAM_LIGHT,
		MODE_G_LIGHT,
		MODE_T_LIGHT,
		NUM_LIGHTS
	};

	enum Stages {
		ATTACK_STAGE,
		ON_STAGE,
		DECAY_STAGE,
		OFF_STAGE
	};
	
	enum ControlModes {
		GATE_MODE,
		STARTSTOP_MODE,
		NUM_CONTROL_MODES
	};
	
	float mute = 0.0f;
	float lastMute = 0.0f;
	float time = 0.0f;
	int stage = OFF_STAGE;
	bool prevRunning = false;
	bool running = false;
	bool gate = false;
	dsp::PulseGenerator  pgTrig;
	LagProcessor monitorSlew;
	GateProcessor gpControl;
	
	float recordTime = 0.0f;
	int hours = 0, minutes = 0, seconds = 0;
	int sDisplay = 0, mDisplay = 0, hDisplay = 0;
	int controlMode = 0;
	int processCount = 9;
	
	float fadeIn = 3.0f;
	float fadeOut = 3.0f;	
	float fadeButton = 0.0f;	
	bool monitor = false;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	FadeExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	
	Fade() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

		configParam(IN_PARAM, 0.1f, 10.0f, 3.0f, "Fade-in time", " S");
		configParam(OUT_PARAM, 0.1f, 10.0f, 3.0f, "Fade-out time", " S");
		configSwitch(FADE_PARAM, 0.0f, 1.0f, 0.0f, "Start/stop", {"Stopped", "Running"});
		configSwitch(MON_PARAM, 0.0f, 1.0f, 0.0f, "Monitor", {"Off", "Monitoring"});

		configInput(L_INPUT, "Left/mono");
		configInput(R_INPUT, "Right");
		configInput(CTRL_INPUT, "Control");
		inputInfos[CTRL_INPUT]->description = "Accepts a gate or start/stop triggers to start and stop the fading action";
		
		configOutput(L_OUTPUT, "Left");
		configOutput(R_OUTPUT, "Right");
		configOutput(GATE_OUTPUT, "Running gate");
		configOutput(TRIG_OUTPUT, "Running start/end trigger");
		outputInfos[GATE_OUTPUT]->description = "Outputs a high gate for the duration of the run time";
		outputInfos[TRIG_OUTPUT]->description = "Outputs trigger pulses at the start of fade-in and end of fade-out";

		configBypass(L_INPUT, L_OUTPUT);
		configBypass(R_INPUT, R_OUTPUT);
		
		processCount = 9;
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		mute = 0.0f;
		lastMute = 0.0f;
		time = 0.0f;
		stage = OFF_STAGE;
		running = false;
		pgTrig.reset();
		monitorSlew.reset();
		gpControl.reset();
		gate = false;
		hours = minutes = seconds = 0;
		sDisplay = mDisplay = hDisplay = 0;
	}	

	json_t* dataToJson() override {
		json_t* root = json_object();
		json_object_set_new(root, "moduleVersion", json_integer(2));
		json_object_set_new(root, "controlMode", json_integer(controlMode));

		// add the theme details
		#include "../themes/dataToJson.hpp"		

		return root;
	}	

	void dataFromJson(json_t* root) override {
		
		controlMode = 0;
		json_t *cm = json_object_get(root, "controlMode");
		if (cm)
			controlMode = json_integer_value(cm);

		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}
		
	void process(const ProcessArgs &args) override {

		if (++processCount > 8) {
			processCount = 0;
			fadeIn = params[IN_PARAM].getValue();
			fadeOut = params[OUT_PARAM].getValue();
			monitor = (params[MON_PARAM].getValue() > 0.5f);
			fadeButton = params[FADE_PARAM].getValue();
		}

		// process monitor pass through button
		if (running) {
			// disable monitor function if already running
			monitor =  false;
			params[MON_PARAM].setValue(0.0f);
		}
		
		
		if (inputs[CTRL_INPUT].isConnected()) {
			// process the control input
			gpControl.set(inputs[CTRL_INPUT].getVoltage());

			if (controlMode == STARTSTOP_MODE) {
				if (gpControl.leadingEdge())
					gate ^= true;
			}
			else {
				if (gpControl.leadingEdge())
					gate = true;
				else if (gpControl.trailingEdge())
					gate = false;
			}
			
			// disable fade if monitoring
			if (monitor) {
				gate = running = false;
				params[FADE_PARAM].setValue(0.0f);			}
			else {
				params[FADE_PARAM].setValue(boolToLight(gate));
			}
		}
		else {
			// process fade start/stop button
			gate = (fadeButton > 0.5f);
			
			// disable fade if monitoring
			if (monitor) {
				gate = running = false;
				params[FADE_PARAM].setValue(0.0f);
			}
		}
		
		
		// only process if we're gated or already running
		if (gate || running) {
			running = true;
			
			time = time + args.sampleTime;
			
			// process the recording timer
			if (!prevRunning) {
				recordTime = 0.0f;
				hours = minutes = seconds = 0;
				
				// update the display values
				sDisplay = mDisplay = hDisplay = 0;
			}
			else	
				recordTime = recordTime + args.sampleTime;

			// update the hour/minute/second counters
			if (recordTime >= 1.0f) {
				recordTime = recordTime - 1.0f;

				if (hours < 99 || minutes < 59 || seconds < 59) {
					if (++seconds > 59)
					{
						seconds = 0;
						
						if(++minutes > 59) {
							minutes = 0;
							hours++;
						}
					}
				}

				// and update the display values
				sDisplay = seconds;
				mDisplay = minutes;
				hDisplay = hours;
			}
			
			// switch to next stage if required
			switch (stage) {
				case ATTACK_STAGE:
					if (time > fadeIn) {
						stage++;
						time = 0.0f;
						lastMute = mute;
					}
					break;
				case ON_STAGE:
					if (!gate) {
						stage++;
						time = 0.0f;
						lastMute = 1.0f;
					}
					break;
				case DECAY_STAGE:
					if (time > fadeOut) {
						stage++;
						time = 0.0f;
						lastMute = mute;
					}
					break;
				case OFF_STAGE:
						time = 0.0f;
						if (gate) {
							stage = ATTACK_STAGE;
						}
						else {
							running = false;
						}
					break;
			}
			
			// determine the envelope value
			float t;
			switch (stage) {
				case ATTACK_STAGE:
					t = fminf(time/fadeIn, 1.0f);
					mute = lastMute + ((1.0f - lastMute) * t);
					break;
				case ON_STAGE:
					mute = 1.0f;
					break;				
				case DECAY_STAGE:
					mute = 1.0f - fminf(time/fadeOut, 1.0f);
					break;
				case OFF_STAGE:
					mute = 0.0f;
					break;
			}			
		}
		else {
			// are we monitoring?
			mute = monitor ? 1.0f : 0.0f;
			mute = monitorSlew.process(mute, 1.0f, 0.1f, 0.1f, args.sampleTime);
		}
		
		// process the signals
		if (inputs[L_INPUT].isConnected()) {
			int numChannelsL = inputs[L_INPUT].getChannels();
			outputs[L_OUTPUT].setChannels(numChannelsL);
			for (int c = 0; c < numChannelsL; c++)
				outputs[L_OUTPUT].setVoltage(inputs[L_INPUT].getVoltage(c) * mute, c);
		}
		else {
			outputs[L_OUTPUT].setChannels(1);
			outputs[L_OUTPUT].setVoltage(0.0f);
		}
		
		if (inputs[R_INPUT].isConnected()) {
			int numChannelsR = inputs[R_INPUT].getChannels();
			outputs[R_OUTPUT].setChannels(numChannelsR);
			for (int c = 0; c < numChannelsR; c++)
				outputs[R_OUTPUT].setVoltage(inputs[R_INPUT].getVoltage(c) * mute, c);
		}
		else {
			outputs[R_OUTPUT].setChannels(1);
			outputs[R_OUTPUT].setVoltage(0.0f);
		}
			
		// kick off the trigger if we need to (on both start and end of the gate)
		bool trig = false;
		if(prevRunning != running) {
			pgTrig.trigger(1e-3f);
			trig = true;
		}
		else 
			trig = pgTrig.process(args.sampleTime);
		
		// process the gate/triggers/lights etc
		outputs[GATE_OUTPUT].setVoltage(boolToGate(running));
		outputs[TRIG_OUTPUT].setVoltage(boolToGate(trig));
		
		if (processCount == 0) {
			if (controlMode > 0) {
				lights[MODE_G_LIGHT].setBrightness(0.0f);
				lights[MODE_T_LIGHT].setBrightness(1.0f);
			}
			else {
				lights[MODE_G_LIGHT].setBrightness(1.0f);
				lights[MODE_T_LIGHT].setBrightness(0.0f);
			}

			lights[L_LIGHT].setBrightness(inputs[L_INPUT].isConnected() ? mute : 0.0f);
			lights[R_LIGHT].setBrightness(inputs[R_INPUT].isConnected() ? mute : 0.0f);
			lights[GATE_LIGHT].setBrightness(boolToLight(running));
			
			// nice feedback for the fading stages
			lights[FADEIN_LIGHT].setBrightness(boolToLight(stage == ATTACK_STAGE));
			lights[FADEOUT_LIGHT].setBrightness(boolToLight(stage == DECAY_STAGE));
		}
		
		lights[TRIG_LIGHT].setSmoothBrightness(boolToLight(trig), args.sampleTime);
		
		// save for next time
		prevRunning = running;
		
		// set up details for the expander
		if (rightExpander.module) {
			if (rightExpander.module->model == modelFadeExpander) {
				
				FadeExpanderMessage *messageToExpander = (FadeExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);

				// set any potential expander module values
				messageToExpander->envelope = mute * 10.0f;
				messageToExpander->run = gate;
				messageToExpander->fadeIn = (stage == ATTACK_STAGE);
				messageToExpander->fadeOut = (stage == DECAY_STAGE);
				
				rightExpander.module->leftExpander.messageFlipRequested = true;
			}
		}	
	}
};

struct FadeWidget : ModuleWidget {

	std::string panelName;
	CountModulaLEDDisplayMini2 *hDisplay;
	CountModulaLEDDisplayMini2 *mDisplay;
	CountModulaLEDDisplayMini2 *sDisplay;
	
	FadeWidget(Fade *module) {
		setModule(module);
		
		panelName = PANEL_FILE;

		// set panel based on current default
		#include "../themes/setPanel.hpp"

		// screws
		#include "../components/stdScrews.hpp"	
		
		// lights
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW1] - 19), module, Fade::L_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW2] - 19), module, Fade::R_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW3] - 19), module, Fade::GATE_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS8[STD_ROW4] - 19), module, Fade::TRIG_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 16, STD_ROWS6[STD_ROW4] - 25), module, Fade::FADEIN_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 16, STD_ROWS6[STD_ROW4] - 25), module, Fade::FADEOUT_LIGHT));
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 18, STD_ROWS8[STD_ROW4] - 26.5f), module, Fade::MODE_G_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 18, STD_ROWS8[STD_ROW4] - 12.5f), module, Fade::MODE_T_LIGHT));
	
		// inputs	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1]), module, Fade::L_INPUT));	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW2]), module, Fade::R_INPUT));	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW4]), module, Fade::CTRL_INPUT));	

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW1]), module, Fade::L_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW2]), module, Fade::R_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW3]), module, Fade::GATE_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS8[STD_ROW4]), module, Fade::TRIG_OUTPUT));	

		// controls
		addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, Fade::IN_PARAM));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, Fade::OUT_PARAM ));
		
		// Mega button - non-standard position
		addParam(createParamCentered<CountModulaLEDPushButtonMega<CountModulaPBLight<RedLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS7[STD_ROW7] - 5), module, Fade::FADE_PARAM, Fade::FADE_PARAM_LIGHT));
		
		//monitor button
		addParam(createParamCentered<CountModulaLEDPushButtonMini<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW3] - 4), module, Fade::MON_PARAM, Fade::MON_PARAM_LIGHT));
		
		// hour/minute/second displays
		hDisplay = new CountModulaLEDDisplayMini2();
		hDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5] - 10));
		hDisplay->text = "00";
		addChild(hDisplay);

		mDisplay = new CountModulaLEDDisplayMini2();
		mDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW5] - 10));
		mDisplay->text = "00";
		addChild(mDisplay);

		sDisplay = new CountModulaLEDDisplayMini2();
		sDisplay->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5] - 10));
		sDisplay->text = "00";
		addChild(sDisplay);
		
	}
	
	//---------------------------------------------------------------------------------------------
	// Control input mode menu
	//---------------------------------------------------------------------------------------------
	// control input mode menu item
	struct ControlModeMenuItem : MenuItem {
		Fade *module;
		int mode;

		void onAction(const event::Action &e) override {
			module->controlMode = mode;
		}
	};
	
	// control input mode menu
	struct ControlModeMenu : MenuItem {
		Fade *module;

		const char *menuLabels[Fade::NUM_CONTROL_MODES] = {"Gate", "Start/Stop triggers" };

		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			for (int i = 0; i < Fade::NUM_CONTROL_MODES; i++) {
				ControlModeMenuItem *modeMenuItem = createMenuItem<ControlModeMenuItem>(menuLabels[i], CHECKMARK(module->controlMode == i));
				modeMenuItem->module = module;
				modeMenuItem->mode = i;
				menu->addChild(modeMenuItem);
			}

			return menu;
		}
	};

	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Fade *module = dynamic_cast<Fade*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"
		
		// add the output mode menu
		ControlModeMenu *modeMenu = createMenuItem<ControlModeMenu>("Control Input Mode", RIGHT_ARROW);
		modeMenu->module = module;
		menu->addChild(modeMenu);
		
	}	
	
	void step() override {
		if (module) {
			Fade *m = (Fade *)module;
			hDisplay->text = rack::string::f("%02d", m->hDisplay);
			mDisplay->text = rack::string::f("%02d", m->mDisplay);
			sDisplay->text = rack::string::f("%02d", m->sDisplay);
			
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};

Model *modelFade = createModel<Fade, FadeWidget>("Fade");
