//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - theme selection step() functionality
// common  functionality for handling of setting the selected theme

//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------
 
int cTheme = ((THEME_MODULE_NAME*)module)->currentTheme;
int pTheme = ((THEME_MODULE_NAME*)module)->prevTheme;

if (cTheme != pTheme) {
	switch (cTheme) {
		case 1:	// Moonlight
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Moonlight/" + panelName)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // white
			break;
		case 2: // Absinthe
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Absinthe/" + panelName)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0x00, 0x00, 0x00); // black
			break;
		case 3: // Raven
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Raven/" + panelName)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // white
			break;
		case 4: // Sanguine
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Sanguine/" + panelName)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // white
			break;
		case 5: // Blue Moon
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueMoon/" + panelName)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // white
			break;
		case 6: // Trick or Treat
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TrickOrTreat/" + panelName)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0x00, 0x00, 0x00); // black
			break;
		default:
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0x00, 0x00, 0x00); // black
			break;
	}
	
	((THEME_MODULE_NAME*)module)->prevTheme = cTheme;
}
