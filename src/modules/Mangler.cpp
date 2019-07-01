//----------------------------------------------------------------------------
//	/^M^\ Count Modula - Bit Crusher Module
//	Bit and sample rate reducer
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/ClockOscillator.hpp"

struct Mangler : Module {
	enum ParamIds {
		INPUT_LEVEL_PARAM,
		SLICE_CV_PARAM,
		CRUSH_CV_PARAM,
		SLICE_PARAM,
		CRUSH_PARAM,
		RANGE_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		SLICE_CV_INPUT,
		CRUSH_CV_INPUT,
		SIGNAL_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		SIGNAL_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	ClockOscillator osc;
	bool currentState = false;
	float bitDepth, bitDepthCV, numBits, bitSize, out, newBits, inputLevel;
	float sr, srCV, sRate;
	bool bipolar; 

	Mangler() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(INPUT_LEVEL_PARAM,0.0f, 1.5f, 1.0f, "Input level", " %", 0.0f, 100.0f, 0.0f);
		configParam(SLICE_CV_PARAM, -1.0f, 1.0f, 0.0f, "Slice CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(CRUSH_CV_PARAM, -1.0f, 1.0f, 0.0f, "Crush CV amount", " %", 0.0f, 100.0f, 0.0f);
		configParam(SLICE_PARAM, 1.0f, 12.0f, 12.0f, "Slice amount");
		configParam(CRUSH_PARAM, 1.0f, 64.0f, 64.0f, "Crush amount");
		configParam(RANGE_PARAM, 0.0f, 1.0f, 1.0f, "Range");
		configParam(MODE_PARAM, 0.0f, 2.0f, 1.0f, "Mangle mode");
	}
	
	void process(const ProcessArgs &args) override {
			
		bool doSlice =  false, doCrush = false;
		switch ((int)(params[MODE_PARAM].getValue())) {
			case 0:
				// bit crush only
				doCrush = true;
				break;
			case 1:
				// both sample rate and bit crush
				doSlice = true;
				doCrush = true;
				break;
			case 2:
				// sample rate only
				doSlice = true;
				break;
		}
		
		if (doSlice) {
			// grab the sample rate 
			sr = params[SLICE_PARAM].getValue();
			srCV = inputs[SLICE_CV_INPUT].getVoltage() * params[SLICE_CV_PARAM].getValue();
			sRate = clamp (sr + srCV, 0.0f, 12.0f) + 2.5f;
			
			// now set the sample clock rate and step it along
			osc.setPitchHigh(sRate);
			osc.step(args.sampleTime);
		}
		
		// now we can set about mangling the input signal - process only on the edges of the clock or every step if not slicing
		if (currentState != osc.high() || !doSlice) {
			
			if (doCrush) {
				// grab bit depth reduction value
				bitDepth = ceil(params[CRUSH_PARAM].getValue());
				
				// grab the bit depth reduction CV amount - scale it such that 10v = full scale.
				bitDepthCV = ceil(inputs[CRUSH_CV_INPUT].getVoltage() * params[CRUSH_CV_PARAM].getValue() * 6.4f);
				
				// apply the CV and ensure we still have sensible values
				bitDepth -= bitDepthCV;
				bitDepth = clamp(bitDepth, 1.0, 64.0);		
						
				// calculate the number of actual steps for full scale
				bipolar = (params[RANGE_PARAM].getValue() > 0.5f );
				bitSize = (bipolar ? 5.0f : 10.0f) / bitDepth;
				bitSize = 10.0f / bitDepth;
			}
			
			inputLevel = params[INPUT_LEVEL_PARAM].getValue();

			// determine number of poly channels to process
			int numChannels = inputs[SIGNAL_INPUT].getChannels();
			outputs[SIGNAL_OUTPUT].setChannels(numChannels);

			// process each channel in turn
			for (int c = 0; c < numChannels; c++) { 
				// grab the input signal and offset if we need to
				out = inputs[SIGNAL_INPUT].getVoltage(c) * inputLevel;	
				if (bipolar)
					out = clamp(out, -5.5f, 5.5f) + 5.0f;
				else
					out = clamp(out, 0.0f, 10.5f);
					
				if (doCrush) {
					// determine how many bits we actually have - round down to nearest integer
					newBits = round(fabs(out)/bitSize);
					
					// recalculate the value using the number of bits
					out = newBits * bitSize * (out < 0.0f ? -1.0f : 1.0f);
				}

				// remove the offset if we need to
				if (bipolar)
					out -= 5.0;
				
				outputs[SIGNAL_OUTPUT].setVoltage(out, c);
			}
		}
		
		currentState = osc.high();
	}
};

struct ManglerWidget : ModuleWidget {
	ManglerWidget(Mangler *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mangler.svg")));

		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<CountModulaScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<CountModulaScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));	
		
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, Mangler::SIGNAL_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, Mangler::SLICE_CV_INPUT));
		addInput(createInputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, Mangler::CRUSH_CV_INPUT));

		addParam(createParamCentered<CountModulaKnobRed>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1]), module, Mangler::INPUT_LEVEL_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW2]), module, Mangler::SLICE_PARAM));
		addParam(createParamCentered<CountModulaKnobGreen>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW3]), module, Mangler::SLICE_CV_PARAM));

		addParam(createParamCentered<CountModulaToggle2P>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, Mangler::RANGE_PARAM));

		addParam(createParamCentered<CountModulaRotarySwitchWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW5]), module, Mangler::CRUSH_PARAM));
		addParam(createParamCentered<CountModulaKnobWhite>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW6]), module, Mangler::CRUSH_CV_PARAM));

		addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL2], STD_ROWS6[STD_ROW4]), module, Mangler::MODE_PARAM));

		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, Mangler::SIGNAL_OUTPUT));
	}
};

Model *modelMangler = createModel<Mangler, ManglerWidget>("Mangler");
