//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Binary Comparator Module
//  Copyright (C) 202  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/GateProcessor.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME BinaryComparator
#define PANEL_FILE "BinaryComparator.svg"

struct BinaryComparator : Module {
	enum ParamIds {
		ENUMS(MUTE_PARAMS, 8),
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(A_INPUTS, 8),
		ENUMS(B_INPUTS, 8),
		NUM_INPUTS
	};
	enum OutputIds {
		LT_OUTPUT,
		LTE_OUTPUT,
		EQ_OUTPUT,
		GTE_OUTPUT,
		GT_OUTPUT,
		NE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		LT_LIGHT,
		LTE_LIGHT,
		EQ_LIGHT,
		GTE_LIGHT,
		GT_LIGHT,
		NE_LIGHT,
		NUM_LIGHTS
	};

	
	GateProcessor gpA[8];
	GateProcessor gpB[8];

	short bitMask;
	short a, b;
	bool lt, eq, gt;	
	
	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"	
	
	BinaryComparator() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	
		for (int i = 0; i < 8 ; i++) {
			configParam(MUTE_PARAMS + i, 0.0f, 1.0f, 0.0f, "Mute");
		}

		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}
	
	void onReset() override {
		for (int i = 0; i < 8 ; i++) {
			gpA[i].reset();
			gpB[i].reset();
		}
	}

	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		// add the theme details
		#include "../themes/dataToJson.hpp"		
		
		return root;
	}	
	
	void dataFromJson(json_t* root) override {
		// grab the theme details
		#include "../themes/dataFromJson.hpp"
	}		
	
	void process(const ProcessArgs &args) override {

		a = b = 0;
		bitMask = 0x01;

		// calculate the numbers
		for (int i = 0; i < 8; i ++) {

			if (gpA[i].set(inputs[A_INPUTS + i].getVoltage()))
				a = a + bitMask;
			
			if (gpB[i].set(inputs[B_INPUTS + i].getNormalVoltage(params[MUTE_PARAMS + i].getValue() * 10.0f)))
				b = b + bitMask;
			
			bitMask = bitMask << 1;
		}

		// now calculate the results
		lt = a < b;
		eq = a == b;
		gt = a > b;
		
		outputs[LT_OUTPUT].setVoltage(boolToGate(lt));
		outputs[LTE_OUTPUT].setVoltage(boolToGate(lt || eq));
		outputs[EQ_OUTPUT].setVoltage(boolToGate(eq));
		outputs[GTE_OUTPUT].setVoltage(boolToGate(eq || gt));
		outputs[GT_OUTPUT].setVoltage(boolToGate(gt));
		outputs[NE_OUTPUT].setVoltage(boolToGate(!eq));
		
		lights[LT_LIGHT].setBrightness(boolToLight(lt));
		lights[LTE_LIGHT].setBrightness(boolToLight(lt || eq));
		lights[EQ_LIGHT].setBrightness(boolToLight(eq));
		lights[GTE_LIGHT].setBrightness(boolToLight(eq || gt));
		lights[GT_LIGHT].setBrightness(boolToLight(gt));
		lights[NE_LIGHT].setBrightness(boolToLight(!eq));
	}
};


struct BinaryComparatorWidget : ModuleWidget {
	BinaryComparatorWidget(BinaryComparator *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BinaryComparator.svg")));

		// screws
		#include "../components/stdScrews.hpp"	
			
		// inputs/buttons
		for (int i = 0; i < 8 ; i++) {
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS8[STD_ROW1 + i]), module, BinaryComparator::A_INPUTS + i));	
			addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL2] + 15, STD_ROWS8[STD_ROW1 + i]), module, BinaryComparator::B_INPUTS + i));	
			addParam(createParamCentered<CountModulaPBSwitch>(Vec(STD_COLUMN_POSITIONS[STD_COL4], STD_ROWS8[STD_ROW1 + i]), module, BinaryComparator::MUTE_PARAMS + i));
		}
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS6[STD_ROW1]), module, BinaryComparator::LT_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS6[STD_ROW2]), module, BinaryComparator::LTE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS6[STD_ROW3]), module, BinaryComparator::EQ_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS6[STD_ROW4]), module, BinaryComparator::GTE_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS6[STD_ROW5]), module, BinaryComparator::GT_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL6], STD_ROWS6[STD_ROW6]), module, BinaryComparator::NE_OUTPUT));
		
		// Lights
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 19, STD_ROWS6[STD_ROW1]), module, BinaryComparator::LT_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 19, STD_ROWS6[STD_ROW2]), module, BinaryComparator::LTE_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 19, STD_ROWS6[STD_ROW3]), module, BinaryComparator::EQ_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 19, STD_ROWS6[STD_ROW4]), module, BinaryComparator::GTE_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 19, STD_ROWS6[STD_ROW5]), module, BinaryComparator::GT_LIGHT));
		addChild(createLightCentered<SmallLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL6] + 19, STD_ROWS6[STD_ROW6]), module, BinaryComparator::NE_LIGHT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		BinaryComparator *module = dynamic_cast<BinaryComparator*>(this->module);
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

Model *modelBinaryComparator = createModel<BinaryComparator, BinaryComparatorWidget>("BinaryComparator");
