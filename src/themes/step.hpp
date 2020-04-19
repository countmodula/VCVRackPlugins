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
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Moonlight/" PANEL_FILE)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // black
			break;
		case 2: // Absinthe
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Absinthe/" PANEL_FILE)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0x00, 0x00, 0x00); // black
			break;
		case 3: // Raven
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Raven/" PANEL_FILE)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // black
			break;
		case 4: // Sanguine
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Sanguine/" PANEL_FILE)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // black
			break;
		case 5: // Blue Moon
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueMoon/" PANEL_FILE)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0xff, 0xff, 0xff); // black
			break;
		default:
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" PANEL_FILE)));
			((THEME_MODULE_NAME*)module)->bezelColor = nvgRGB(0x00, 0x00, 0x00); // black
			break;
	}
	
	((THEME_MODULE_NAME*)module)->prevTheme = cTheme;
}
