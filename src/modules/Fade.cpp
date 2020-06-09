//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Recording Fade Module
//	A fade-in/fade-out and trigger controller for the VCV Rack Rec module.
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/Utility.hpp"

#include "../inc/FadeExpanderMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME Fade
#define PANEL_FILE "Fade.svg"

struct Fade : Module {
	enum ParamIds {
		FADE_PARAM,
		IN_PARAM,
		OUT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		L_INPUT,
		R_INPUT,
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
		NUM_LIGHTS
	};

	enum Stages {
		ATTACK_STAGE,
		ON_STAGE,
		DECAY_STAGE,
		OFF_STAGE
	};
	
	float mute = 0.0f;
	float lastMute = 0.0f;
	float time = 0.0f;
	int stage = OFF_STAGE;
	bool prevRunning = false;
	bool running = false;
	dsp::PulseGenerator  pgTrig;
	
	float recordTime = 0.0f;
	int hours = 0, minutes = 0, seconds = 0;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
	
	FadeExpanderMessage rightMessages[2][1]; // messages to right module (expander)
	
	CountModulaDisplayMini2 *hDisplay;
	CountModulaDisplayMini2 *mDisplay;
	CountModulaDisplayMini2 *sDisplay;	
	
	Fade() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(IN_PARAM, 0.1f, 10.0f, 3.0f, "Fade-in time", " S");
		configParam(OUT_PARAM, 0.1f, 10.0f, 3.0f, "Fade-out time", " S");
		configParam(FADE_PARAM, 0.0f, 1.0f, 0.0f, "Start/stop");

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
		
		hours = minutes = seconds = 0;
		sDisplay->text = string::f("%02d", seconds);
		mDisplay->text = string::f("%02d", minutes);
		hDisplay->text = string::f("%02d", hours);		
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
		
		// grab the fade time settings
		float fadeIn = params[IN_PARAM].getValue();
		float fadeOut = params[OUT_PARAM].getValue();

		bool gate = (params[FADE_PARAM].getValue() > 0.5f);
		
		// only process if we're gated or already running
		if (gate || running) {
			running = true;
			
			time = time + args.sampleTime;
			
			// // process the recording timer
			if (!prevRunning) {
				recordTime = 0.0f;
				hours = minutes = seconds = 0;
				
				// update the display
				sDisplay->text = string::f("%02d", seconds);
				mDisplay->text = string::f("%02d", minutes);
				hDisplay->text = string::f("%02d", hours);
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

				// and update the display
				sDisplay->text = string::f("%02d", seconds);
				mDisplay->text = string::f("%02d", minutes);
				hDisplay->text = string::f("%02d", hours);
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
		
		// process the signals
		int numChannelsL = inputs[L_INPUT].getChannels();
		outputs[L_OUTPUT].setChannels(numChannelsL);
		for (int c = 0; c < numChannelsL; c++)
			outputs[L_OUTPUT].setVoltage(inputs[L_INPUT].getVoltage(c) * mute, c);
		
		int numChannelsR = inputs[R_INPUT].getChannels();
		outputs[R_OUTPUT].setChannels(numChannelsR);
		for (int c = 0; c < numChannelsR; c++)
			outputs[R_OUTPUT].setVoltage(inputs[R_INPUT].getVoltage(c) * mute, c);
			
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
		
		lights[L_LIGHT].setBrightness(inputs[L_INPUT].isConnected() ? mute : 0.0f);
		lights[R_LIGHT].setBrightness(inputs[R_INPUT].isConnected() ? mute : 0.0f);
		lights[GATE_LIGHT].setBrightness(boolToLight(running));
		lights[TRIG_LIGHT].setSmoothBrightness(boolToLight(trig), args.sampleTime);
		
		// nice feedback for the fading stages
		lights[FADEIN_LIGHT].setBrightness(boolToLight(stage == ATTACK_STAGE));
		lights[FADEOUT_LIGHT].setBrightness(boolToLight(stage == DECAY_STAGE));
		
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
	FadeWidget(Fade *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Fade.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// lights
		addChild(createLightCentered<SmallLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS6[STD_ROW1] - 19), module, Fade::L_LIGHT));
		addChild(createLightCentered<SmallLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS6[STD_ROW2] - 19), module, Fade::R_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] + 20, STD_ROWS6[STD_ROW3] - 19), module, Fade::GATE_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 20, STD_ROWS6[STD_ROW3] - 19), module, Fade::TRIG_LIGHT));

		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1] - 16, STD_ROWS6[STD_ROW4] - 25), module, Fade::FADEIN_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3] + 16, STD_ROWS6[STD_ROW4] - 25), module, Fade::FADEOUT_LIGHT));
	
		// inputs	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, Fade::L_INPUT));	
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, Fade::R_INPUT));	

		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, Fade::L_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, Fade::R_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, Fade::GATE_OUTPUT));	
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, Fade::TRIG_OUTPUT));	

		// controls
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, Fade::IN_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW4]), module, Fade::OUT_PARAM ));
		
		// Mega mute button - non-standard position
		addParam(createParamCentered<CountModulaPBSwitchMega>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS7[STD_ROW7] - 5), module, Fade::FADE_PARAM));
		
		// hour/minute/second displays
		CountModulaDisplayMini2 *hDisp = new CountModulaDisplayMini2();
		hDisp->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5] - 10));
		hDisp->text = "00";
		addChild(hDisp);

		CountModulaDisplayMini2 *mDisp = new CountModulaDisplayMini2();
		mDisp->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW5] - 10));
		mDisp->text = "00";
		addChild(mDisp);

		CountModulaDisplayMini2 *sDisp = new CountModulaDisplayMini2();
		sDisp->setCentredPos(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5] - 10));
		sDisp->text = "00";
		addChild(sDisp);
		
		if (module) {
			module->hDisplay = hDisp;
			module->mDisplay = mDisp;
			module->sDisplay = sDisp;
		}		
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		Fade *module = dynamic_cast<Fade*>(this->module);
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

Model *modelFade = createModel<Fade, FadeWidget>("Fade");
