//----------------------------------------------------------------------------
//	/^M^\ Count Modula - theme selection step() functionality
// common  functionality for handling of setting the selected theme
//----------------------------------------------------------------------------
 
int cTheme = ((THEME_MODULE_NAME*)module)->currentTheme;
int pTheme = ((THEME_MODULE_NAME*)module)->prevTheme;

if (cTheme != pTheme) {
	switch (cTheme) {
		case 1:	// Moonlight
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Moonlight/" PANEL_FILE)));
			break;
		case 2: // Absinthe
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Absinthe/" PANEL_FILE)));
			break;
		case 3: // Raven
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Raven/" PANEL_FILE)));
			break;
		case 4: // Sanguine
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Sanguine/" PANEL_FILE)));
			break;
		default:
			setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/" PANEL_FILE)));
			break;
	}
	
	((THEME_MODULE_NAME*)module)->prevTheme = cTheme;
}
