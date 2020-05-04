//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Voltage Scaler
//  Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME VoltageScaler
#define PANEL_FILE "VoltageScaler.svg"

struct VoltageScaler : Module {
	enum ParamIds {
		MIN_PARAM,
		MAX_PARAM,
		UPPER_PARAM,
		LOWER_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		UPPER_INPUT,
		LOWER_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		CV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		UNDER_LIGHT,
		OVER_LIGHT,
		NUM_LIGHTS
	};

	
	float inMin, inMax, inDiff;
	float limitA, limitB, diff, cvOut, cvIn;
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"
		
	VoltageScaler() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		configParam(MIN_PARAM, -10.0f, 10.0f, 0.0f, "Input minimum", " V");
		configParam(MAX_PARAM, -10.0f, 10.0f, 10.0f, "Input maximum", " V");

		configParam(UPPER_PARAM, -1.0f, 1.0f, 0.0f, "Output limit A", " V", 0.0f, 10.0f, 0.0f);
		configParam(LOWER_PARAM, -1.0f, 1.0f, 0.0f, "Output limit B", " V", 0.0f, 10.0f, 0.0f);

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

		// params are common to all channels
		inMin = params[MIN_PARAM].getValue();
		inMax = clamp(params[MAX_PARAM].getValue(), inMin, 10.0f);
		limitA = clamp(inputs[UPPER_INPUT].getNormalVoltage(10.0f), -10.0f, 10.0f)  * params[UPPER_PARAM].getValue();
		limitB = clamp(inputs[LOWER_INPUT].getNormalVoltage(10.0f), -10.0f, 10.0f) * params[LOWER_PARAM].getValue();

		// calc the differences
		inDiff = fabs(inMax - inMin);
		diff = fabs(limitB - limitA);

		// apply the same scaling to all channels presented to the input
		int numChans = inputs[CV_INPUT].getChannels();
		bool over = false;
		bool under = false;
		if (numChans > 0)
		{
			outputs[CV_OUTPUT].setChannels(numChans);
			
			for (int c = 0; c < numChans; c++)
			{
				cvIn = inputs[CV_INPUT].getVoltage(c);
				over |= (cvIn > inMax);
				under |= (cvIn < inMin);
				
				if (diff > 0.0001f && inDiff > 0.00001f) {
					cvOut = rescale(clamp(cvIn, inMin, inMax), inMin, inMax, 0.0f, diff);
				}
				else
					cvOut = 0.0f;

				outputs[CV_OUTPUT].setVoltage(fminf(limitA, limitB) + cvOut, c);
			}
		}
		else
			outputs[CV_OUTPUT].setVoltage(0.0f);
		
		// indicate if we've had an under or over input
		lights[UNDER_LIGHT].setSmoothBrightness(boolToLight(under), args.sampleTime);
		lights[OVER_LIGHT].setSmoothBrightness(boolToLight(over), args.sampleTime);
	}
};

struct VoltageScalerWidget : ModuleWidget {
	VoltageScalerWidget(VoltageScaler *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/VoltageScaler.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
		
		// knobs
		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW1]), module, VoltageScaler::MIN_PARAM));
		addParam(createParamCentered<CountModulaKnobYellow>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW2]), module, VoltageScaler::MAX_PARAM));
		addParam(createParamCentered<CountModulaKnobOrange>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW3]), module, VoltageScaler::UPPER_PARAM));
		addParam(createParamCentered<CountModulaKnobOrange>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW4]), module, VoltageScaler::LOWER_PARAM));
		
		// inputs
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3]), module, VoltageScaler::UPPER_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4]), module, VoltageScaler::LOWER_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW5]), module, VoltageScaler::CV_INPUT));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW5]), module, VoltageScaler::CV_OUTPUT));
		
		// error lights
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW1]), module, VoltageScaler::UNDER_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW2]), module, VoltageScaler::OVER_LIGHT));
	}

	struct PresetMenuItem : MenuItem {
		VoltageScalerWidget *widget;
		float inMin, inMax, limitA, limitB;
		std::string name;
		
		void setValues(std::string undoText, float iMin, float iMax, float limA, float limB) {
			name = undoText;
			inMin = iMin;
			inMax = iMax;
			limitA = limA;
			limitB = limB;
		}
		
		void onAction(const event::Action &e) override {

			// history - current settings
			history::ModuleChange *h = new history::ModuleChange;
			h->name = name;
			h->moduleId = widget->module->id;
			h->oldModuleJ = widget->toJson();
		
			widget->getParam(VoltageScaler::MIN_PARAM)->paramQuantity->setValue(inMin);
			widget->getParam(VoltageScaler::MAX_PARAM)->paramQuantity->setValue(inMax);
			widget->getParam(VoltageScaler::UPPER_PARAM)->paramQuantity->setValue(limitA);
			widget->getParam(VoltageScaler::LOWER_PARAM)->paramQuantity->setValue(limitB);

			// history - new settings
			h->newModuleJ = widget->toJson();
			APP->history->push(h);	
		}
	};

	struct PresetMenu : MenuItem {
		VoltageScalerWidget *widget;
	
		Menu *createChildMenu() override {
			Menu *menu = new Menu;

			// scale ±5 to 0-10
			PresetMenuItem *presetMenuItem1 = createMenuItem<PresetMenuItem>("Scale ±5V to 0-10V");
			presetMenuItem1->setValues("scale ±5V to 0-10V", -5.0f, 5.0f, 0.0f, 1.0f);
			presetMenuItem1->widget = widget;
			menu->addChild(presetMenuItem1);
			
			// scale 0-10 to ±5
			PresetMenuItem *presetMenuItem2 = createMenuItem<PresetMenuItem>("Scale 0-10V to ±5V");
			presetMenuItem2->setValues("scale 0-10V to ±5V", 0.0f, 10.0f, -0.5f, 0.5f);
			presetMenuItem2->widget = widget;
			menu->addChild(presetMenuItem2);

			return menu;
		}
	};
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		VoltageScaler *module = dynamic_cast<VoltageScaler*>(this->module);
		assert(module);

		// blank separator
		menu->addChild(new MenuSeparator());
		
		// add the theme menu items
		#include "../themes/themeMenus.hpp"

		// blank separator
		menu->addChild(new MenuSeparator());

		// preset menu
		PresetMenu *presetMenuItem = createMenuItem<PresetMenu>("Preset", RIGHT_ARROW);
		presetMenuItem->widget = this;
		menu->addChild(presetMenuItem);
	}	
	
	void step() override {
		if (module) {
			// process any change of theme
			#include "../themes/step.hpp"
		}
		
		Widget::step();
	}	
};	

Model *modelVoltageScaler = createModel<VoltageScaler, VoltageScalerWidget>("VoltageScaler");
