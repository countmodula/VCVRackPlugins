//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - theme selection step() functionality
// common  functionality for handling of setting the selected theme

//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

int cTheme = module? module->currentTheme : getDefaultTheme(false);

switch (cTheme) {
	case 1:	// Moonlight
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Moonlight/" + panelName)));
		break;
	case 2: // Absinthe
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Absinthe/" + panelName)));
		break;
	case 3: // Raven
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Raven/" + panelName)));
		break;
	case 4: // Sanguine
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Sanguine/" + panelName)));
		break;
	case 5: // Blue Moon
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BlueMoon/" + panelName)));
		break;
	case 6: // Trick or Treat
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/TrickOrTreat/" + panelName)));
		break;
	default:
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" + panelName)));
		break;
}
	
