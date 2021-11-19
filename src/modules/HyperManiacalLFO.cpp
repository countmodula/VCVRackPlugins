//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - Hyper Maniacal LFO Module
//	Oscillator section based on the VCV VCO by Andrew Belt
//	Copyright (C) 2020  Adam Verspaget
//----------------------------------------------------------------------------
#include "../CountModula.hpp"
#include "../inc/SlewLimiter.hpp"
#include "../inc/Utility.hpp"
#include "../inc/HyperManiacalLFOExpanderMessage.hpp"
#include "../inc/MegalomaniacControllerMessage.hpp"

// set the module name for the theme selection functions
#define THEME_MODULE_NAME HyperManiacalLFO
#define PANEL_FILE "HyperManiacalLFO.svg"

using simd::float_4;

template <typename T>
T sin2pi_pade_05_5_4(T x) {
	x -= 0.5f;
	return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
	       / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <int OVERSAMPLE, int QUALITY, typename T>
struct VoltageControlledOscillator {

	bool unipolar = false;
	
	// For optimizing in serial code
	int channels = 0;

	T phase = 0.f;
	T freq;
	T pulseWidth = 0.5f;
	T syncDirection = 1.f;

	dsp::TRCFilter<T> sqrFilter;

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sqrMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sawMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> triMinBlep;
	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sinMinBlep;

	T sqrValue = 0.f;
	T sawValue = 0.f;
	T triValue = 0.f;
	T sinValue = 0.f;

	void setPhase(T newPhase) {
		phase = simd::clamp(newPhase, -1.0f, 1.0f);
	}
	
	void setPitch(T pitch) {
		freq = dsp::approxExp2_taylor5(pitch + 30) / 1073741824;
	}

	void setPulseWidth(T pulseWidth) {
		const float pwMin = 0.01f;
		this->pulseWidth = simd::clamp(pulseWidth, pwMin, 1.0f - pwMin);
	}

	void process(float deltaTime) {
		// Advance phase
		T deltaPhase = simd::fmin(freq * deltaTime, 0.5f);
		
		// Reset back to forward
		syncDirection = 1.0f;

		phase += deltaPhase;
		// Wrap phase
		phase -= simd::floor(phase);

		// Jump sqr when crossing 0, or 1 if backwards
		T wrapPhase = (syncDirection == -1.f) & 1.f;
		T wrapCrossing = (wrapPhase - (phase - deltaPhase)) / deltaPhase;
		int wrapMask = simd::movemask((0 < wrapCrossing) & (wrapCrossing <= 1.f));
		if (wrapMask) {
			for (int i = 0; i < channels; i++) {
				if (wrapMask & (1 << i)) {
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = wrapCrossing[i] - 1.f;
					T x = mask & (2.f * syncDirection);
					sqrMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Jump sqr when crossing `pulseWidth`
		T pulseCrossing = (pulseWidth - (phase - deltaPhase)) / deltaPhase;
		int pulseMask = simd::movemask((0 < pulseCrossing) & (pulseCrossing <= 1.f));
		if (pulseMask) {
			for (int i = 0; i < channels; i++) {
				if (pulseMask & (1 << i)) {
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = pulseCrossing[i] - 1.f;
					T x = mask & (-2.f * syncDirection);
					sqrMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Jump saw when crossing 0.5
		T halfCrossing = (0.5f - (phase - deltaPhase)) / deltaPhase;
		int halfMask = simd::movemask((0 < halfCrossing) & (halfCrossing <= 1.f));
		if (halfMask) {
			for (int i = 0; i < channels; i++) {
				if (halfMask & (1 << i)) {
					T mask = simd::movemaskInverse<T>(1 << i);
					float p = halfCrossing[i] - 1.f;
					T x = mask & (-2.f * syncDirection);
					sawMinBlep.insertDiscontinuity(p, x);
				}
			}
		}

		// Square
		sqrValue = sqr(phase);
		sqrValue += sqrMinBlep.process();

		// Saw
		sawValue = saw(phase);
		sawValue += sawMinBlep.process();

		// Tri
		triValue = tri(phase);
		triValue += triMinBlep.process();

		// Sin
		sinValue = sin(phase);
		sinValue += sinMinBlep.process();
	}

	T sin(T phase) {
		T v = sin2pi_pade_05_5_4(phase);

		return v;
	}
	T sin() {
		
		if (unipolar)
			return (sinValue + 1.0f) *1.2f;
		else	
			return sinValue * 1.2f;
	}

	T tri(T phase) {
		T v = 1 - 4 * simd::fmin(simd::fabs(phase - 0.25f), simd::fabs(phase - 1.25f));

		return v;
	}
	T tri() {
		if (unipolar)
			return (triValue + 1.0f) * 1.2f;
		else	
			return triValue * 1.2f;
	}

	T saw(T phase) {
		T x = phase + 0.5f;
		x -= simd::trunc(x);
		T v = 2.0f * x - 1.0f;

		return v;
	}
	T saw() {
		if (unipolar)
			return (sawValue + 1.0f) * 1.2f;
		else
			return sawValue * 1.2f;
	}

	T sqr(T phase) {
		T v = simd::ifelse(phase < pulseWidth, 1.0f, -1.0f);
		return v;
	}
	
	T sqr() {
		if (unipolar)
			return (sqrValue + 1.0f) * 1.2f;
		else
			return sqrValue * 1.2f;
	}

	T light() {
		return simd::sin(2 * T(M_PI) * phase);
	}
};

struct HyperManiacalLFO : Module {
	enum ParamIds {
		ENUMS(FREQ_PARAMS, 6),
		ENUMS(RANGE_SW_PARAMS, 6),
		ENUMS(WAVE_SEL_PARAMS, 6),
		GLIDE_PARAM,
		GLIDE_SH_PARAM,
		LEVEL_PARAM,
		MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		LFO_OUTPUT,
		INV_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(LFO_LIGHTS, 6),
		MODE_PARAM_LIGHT,
		NUM_LIGHTS
	};

	// add the variables we'll use when managing themes
	#include "../themes/variables.hpp"

	HyperManiacalLFOExpanderMessage rightMessages[2][1]; 	// messages to right module (expander)
	MegalomaniacControllerMessage leftMessages[2][1];		// messages from left module (controller)
	
	HyperManiacalLFOExpanderMessage dummyExpndrMessage;
	MegalomaniacControllerMessage	dummyCntrlrMessage;
	
	VoltageControlledOscillator<8, 8, float_4> lfos[2];
	
	LagProcessor slew;
	
	HyperManiacalLFO() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		
		configParam(GLIDE_SH_PARAM, 0.0f, 1.0f, 1.0f, "Glide shape");
		configParam(GLIDE_PARAM, 0.0f, 1.0f, 0.0f, "Glide rate");
		
		std::string oscName;
		for (int i = 0; i < 6; i++) {
			oscName = "Oscillator " + std::to_string(i + 1);
			configParam(FREQ_PARAMS + i, 0.0f, 8.0f, (float)(i), oscName + " rate");
			configSwitch(RANGE_SW_PARAMS + i, 0.0f, 2.0f, 1.0f, oscName + " range", {"Ultra low", "Low", "High"});
			configSwitch(WAVE_SEL_PARAMS + i, 0.0f, 4.0f, 3.0f, oscName + " wave", {"Off", "Sine", "Saw", "Triangle", "Square"});
		}

		configParam(LEVEL_PARAM, 0.0f, 1.0f, 1.0f, "Output level");
		configSwitch(MODE_PARAM, 0.0f, 1.0f, 0.0f, "Output mode", {"Unipolar", "Bipolar"});
	
		configOutput(LFO_OUTPUT, "Maniacal LFO");
		configOutput(INV_OUTPUT, "Inverted maniacal LFO");
	
		for (int i = 0; i < 2; i++) {		
			float r[] = {random::uniform(), random::uniform(), random::uniform(), random::uniform()};
			float_4 newPhase = float_4::load(&r[0]);
			lfos[i].setPhase(newPhase);
		}
		
		// expander
		rightExpander.producerMessage = rightMessages[0];
		rightExpander.consumerMessage = rightMessages[1];			

		// controller
		leftExpander.producerMessage = leftMessages[0];
		leftExpander.consumerMessage = leftMessages[1];
		
		// set the theme from the current default value
		#include "../themes/setDefaultTheme.hpp"
	}

	void onReset() override {
		for (int i = 0; i < 2; i++) {		

			float r[] = {random::uniform(), random::uniform(), random::uniform(), random::uniform()};
			float_4 newPhase = float_4::load(&r[0]);
			lfos[i].setPhase(newPhase);
		}
		
		slew.reset();
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

		// set up details for the expander
		HyperManiacalLFOExpanderMessage *messageToExpander;
		if (rightExpander.module && isExpanderModule(rightExpander.module))
			messageToExpander = (HyperManiacalLFOExpanderMessage*)(rightExpander.module->leftExpander.producerMessage);
		else
			messageToExpander = &dummyExpndrMessage;

		messageToExpander->unipolar = params[MODE_PARAM].getValue() < 0.5f;

		// grab details from the contoller if present
		MegalomaniacControllerMessage *messageFromController;
		if (leftExpander.module && isControllerModule(leftExpander.module))
			messageFromController = (MegalomaniacControllerMessage*)(leftExpander.consumerMessage);
		else
			messageFromController = &dummyCntrlrMessage;

		float osc_pitch[8] = {};
		for (int i = 0; i < 6; i++) {
			osc_pitch[i] = params[FREQ_PARAMS + i].getValue() + messageFromController->fmValue[i];
			
			// determine the range - need to reverse the order from the controller
			int range = 3 - messageFromController->selectedRange[i];
			if (range > 2)
				range = (int)(params[RANGE_SW_PARAMS + i].getValue());
			
			switch (range) {
				case 2: // high 45 Hz - 11.6kHz
					osc_pitch[i] += 5.5;
					break;
				case 0: // ultra low 0.002 Hz - 0.5 Hz (2-512 second period)
					osc_pitch[i] -= 9.0;
					break;
				default:	// low 0.5 Hz - 116 Hz (0.07 - 2 second period)
					osc_pitch[i] -= 1.0;
					break;
			}
		}
		
		for (int i = 0; i < 6; i+=4) {
			auto* lfo = &lfos[i / 4];
			
			lfo->channels = std::min(6 - i, 4);
			lfo->unipolar = messageToExpander->unipolar;
			
			float_4 pitch = float_4::load(&osc_pitch[i]);

			lfo->setPitch(pitch);
			lfo->process(args.sampleTime);
	
			lfo->sin().store(&(messageToExpander->sin[i]));
			lfo->saw().store(&(messageToExpander->saw[i]));
			lfo->tri().store(&(messageToExpander->tri[i]));
			lfo->sqr().store(&(messageToExpander->sqr[i]));
			
			lfo->light().store(&(messageToExpander->lig[i]));
			
			lfo->freq.store(&(messageToExpander->frq[i]));
		}
		
		float lvl = params[LEVEL_PARAM].getValue();
		float oscValues = 0.0f;

		for (int i = 0; i < 6; i++) {
			
			// determine the waveform selection
			int wave = messageFromController->selectedWaveform[i] - 1;
			if (wave < 0)
				wave = (int)(params[WAVE_SEL_PARAMS + i].getValue());
			
			// if we have a controller, apply the mix levels
			float mix = messageFromController-> mixLevel[i];
			switch (wave) {
				case 1:
					oscValues += (messageToExpander->sin[i] * lvl * mix);
					break;
				case 2:
					oscValues += (messageToExpander->saw[i] * lvl * mix);
					break;
				case 3:
					oscValues += (messageToExpander->tri[i] * lvl * mix);
					break;
				case 4:
					oscValues += (messageToExpander->sqr[i] * lvl * mix);
					break;
				default:
					break;
			}
			
			
			lights[LFO_LIGHTS + i].setSmoothBrightness(messageToExpander->lig[i], args.sampleTime);
		}

		float g = params[GLIDE_PARAM].getValue();
		if (g > 0.0f) {
			float s = params[GLIDE_SH_PARAM].getValue();
			oscValues = slew.process(oscValues, s, g, g, args.sampleTime);
		}

		// TODO: saturation rather than clipping
		oscValues = clamp(oscValues, (messageToExpander->unipolar ? 0.0f : -11.2f), 11.2f);
		
		outputs[LFO_OUTPUT].setVoltage(oscValues);
		outputs[INV_OUTPUT].setVoltage(-oscValues);
		
		// set up details for the expander
		if (rightExpander.module && isExpanderModule(rightExpander.module))
			rightExpander.module->leftExpander.messageFlipRequested = true;
	}
};

struct HyperManiacalLFOWidget : ModuleWidget {
	
	std::string panelName;
	
	HyperManiacalLFOWidget(HyperManiacalLFO *module) {
		setModule(module);
		panelName = PANEL_FILE;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));

		// screws
		#include "../components/stdScrews.hpp"	

		// LFO knobs
		{
			int i = 0, j = 0;

			// LFO 1
			addParam(createParamCentered<Potentiometer<RedKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW1]), module, HyperManiacalLFO::FREQ_PARAMS + i++));
			addParam(createParamCentered<RotarySwitch<OperatingAngle145<RedKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW1]), module, HyperManiacalLFO::WAVE_SEL_PARAMS + j++));

			// LFO 2
			addParam(createParamCentered<Potentiometer<OrangeKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW2]), module, HyperManiacalLFO::FREQ_PARAMS + i++));
			addParam(createParamCentered<RotarySwitch<OperatingAngle145<OrangeKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW2]), module, HyperManiacalLFO::WAVE_SEL_PARAMS + j++));

			// LFO 3
			addParam(createParamCentered<Potentiometer<YellowKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW3]), module, HyperManiacalLFO::FREQ_PARAMS + i++));
			addParam(createParamCentered<RotarySwitch<OperatingAngle145<YellowKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW3]), module, HyperManiacalLFO::WAVE_SEL_PARAMS + j++));

			// LFO 4
			addParam(createParamCentered<Potentiometer<GreenKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW4]), module, HyperManiacalLFO::FREQ_PARAMS + i++));
			addParam(createParamCentered<RotarySwitch<OperatingAngle145<GreenKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW4]), module, HyperManiacalLFO::WAVE_SEL_PARAMS + j++));

			// LFO 5
			addParam(createParamCentered<Potentiometer<BlueKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW5]), module, HyperManiacalLFO::FREQ_PARAMS + i++));
			addParam(createParamCentered<RotarySwitch<OperatingAngle145<BlueKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW5]), module, HyperManiacalLFO::WAVE_SEL_PARAMS + j++));

			// LFO 6
			addParam(createParamCentered<Potentiometer<VioletKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL1], STD_ROWS6[STD_ROW6]), module, HyperManiacalLFO::FREQ_PARAMS + i++));
			addParam(createParamCentered<RotarySwitch<OperatingAngle145<VioletKnob>>>(Vec(STD_COLUMN_POSITIONS[STD_COL5], STD_ROWS6[STD_ROW6]), module, HyperManiacalLFO::WAVE_SEL_PARAMS + j++));
		}
		
		// Other LFO related bits
		for (int i = 0; i < 6; i++) {
			addParam(createParamCentered<CountModulaToggle3P>(Vec(STD_COLUMN_POSITIONS[STD_COL3], STD_ROWS6[STD_ROW1 + i]), module, HyperManiacalLFO::RANGE_SW_PARAMS + i));
			
			// lights
			addChild(createLightCentered<MediumLight<RedLight>>(Vec(STD_COLUMN_POSITIONS[STD_COL2] - 5, STD_ROWS6[STD_ROW1 + i] - 20), module, HyperManiacalLFO::LFO_LIGHTS + i));
		}
		
		// mode switch
		addParam(createParamCentered<CountModulaLEDPushButton<CountModulaPBLight<GreenLight>>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW3]), module, HyperManiacalLFO::MODE_PARAM, HyperManiacalLFO::MODE_PARAM_LIGHT));
		
		// glide controls switch
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW1]), module, HyperManiacalLFO::GLIDE_PARAM));
		addParam(createParamCentered<Potentiometer<WhiteKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW2]), module, HyperManiacalLFO::GLIDE_SH_PARAM));

		// level
		addParam(createParamCentered<Potentiometer<GreyKnob>>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW4]), module, HyperManiacalLFO::LEVEL_PARAM));
		
		// outputs
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW5]), module, HyperManiacalLFO::LFO_OUTPUT));
		addOutput(createOutputCentered<CountModulaJack>(Vec(STD_COLUMN_POSITIONS[STD_COL7], STD_ROWS6[STD_ROW6]), module, HyperManiacalLFO::INV_OUTPUT));
	}
	
	// include the theme menu item struct we'll when we add the theme menu items
	#include "../themes/ThemeMenuItem.hpp"

	void appendContextMenu(Menu *menu) override {
		HyperManiacalLFO *module = dynamic_cast<HyperManiacalLFO*>(this->module);
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

Model *modelHyperManiacalLFO = createModel<HyperManiacalLFO, HyperManiacalLFOWidget>("HyperManiacalLFO");
