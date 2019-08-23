//----------------------------------------------------------------------------
//	/^M^\ Count Modula - SlopeDetector Module
//	A VCV rack implementation of the CGS Slope Detector designed by Ken Stone
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/Utility.hpp"
#include "../inc/SlewLimiter.hpp"

struct SlopeDetector : Module {
	enum ParamIds {
		SENSE_PARAM,
		RANGE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		RISING_OUTPUT,
		STEADY_OUTPUT,
		FALLING_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RISING_LIGHT,
		STEADY_LIGHT,
		FALLING_LIGHT,
		NUM_LIGHTS
	};

	LagProcessor lag;
	
	float cv = 0.0f, cvLag = 0.0f;
	float lagAmt = 0.0f, range = 0.0f;
	
	int count = 0;
	bool prevRising = false, prevFalling = false;
	bool rising = false, falling = false, steady = true;
	
	SlopeDetector() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(SENSE_PARAM, 0.0f, 1.0f, 0.0f, "Sense");
		configParam(RANGE_PARAM, 0.0f, 1.0f, 0.0f, "Sense range");
	}
	
	json_t *dataToJson() override {
		json_t *root = json_object();

		json_object_set_new(root, "moduleVersion", json_string("1.0"));
		
		return root;
	}
	
	void process(const ProcessArgs &args) override {
		// grab CV input value
		cv = inputs[CV_INPUT].getVoltage();

		// apply lag
		range = (params[RANGE_PARAM].getValue() > 0.5f ? 1.0f : 0.5f);
		lagAmt = (0.1f + params[SENSE_PARAM].getValue()) * range;
		cvLag = lag.process(cv, 1.0f, lagAmt, lagAmt);
		
		// determine if we're rising or falling based on the lagged value
		// applying a bit of hysteresis to avoid double triggering.
		// double triggering will still happen if the response is set too quick and the input signal is slow
		if(fabs(cv - cvLag) > 0.01f) {
			if (cv > cvLag) {
				rising = true;
			}
			else if (prevRising && cv < (cvLag - 0.01f)) {
				rising = false;
			}
			
			if (cv < cvLag) {
				falling = true;
			}
			else if (prevFalling && cv > (cvLag + 0.01f)) {
				falling = false;
			}
		}
		else {
			// any cv input within 0.01 volts of the lagged value is considered steady
			rising = falling = false;
		}
		
		// steady only if neither rising or falling
		steady = !(rising || falling);
		
		// set the outputs
		outputs[RISING_OUTPUT].setVoltage(boolToGate(rising));
		outputs[STEADY_OUTPUT].setVoltage(boolToGate(steady));
		outputs[FALLING_OUTPUT].setVoltage(boolToGate(falling));

		// Set lights
		lights[RISING_LIGHT].setSmoothBrightness(boolToLight(rising), args.sampleTime);
		lights[STEADY_LIGHT].setSmoothBrightness(boolToLight(steady), args.sampleTime);
		lights[FALLING_LIGHT].setSmoothBrightness(boolToLight(falling), args.sampleTime);
		
		// use these to detect change of direction
		prevRising = rising;
		prevFalling = falling;
	}
};

struct SlopeDetectorWidget : ModuleWidget {
	SlopeDetectorWidget(SlopeDetector *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/SlopeDetector.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW1]), module, SlopeDetector::CV_INPUT));

		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW1]), module, SlopeDetector::SENSE_PARAM));
		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW2]), module, SlopeDetector::RANGE_PARAM));

		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW3]), module, SlopeDetector::RISING_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW4]), module, SlopeDetector::STEADY_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS5[STD_ROW5]), module, SlopeDetector::FALLING_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW3]), module, SlopeDetector::RISING_LIGHT));
		addChild(createLightCentered<MediumLight<YellowLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW4]), module, SlopeDetector::STEADY_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS5[STD_ROW5]), module, SlopeDetector::FALLING_LIGHT));
	}
};

Model *modelSlopeDetector = createModel<SlopeDetector, SlopeDetectorWidget>("SlopeDetector");
