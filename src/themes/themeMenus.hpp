//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - theme menus
// common menus for selecting themes
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------


menu->addChild(createMenuLabel("Theme"));

// add main theme menu item
ThemeMenu *themeMenuItem = createMenuItem<ThemeMenu>("Set", RIGHT_ARROW);
themeMenuItem->module = module;
menu->addChild(themeMenuItem);

// add the default theme menu item
DefaultThemeMenu *defaultThemeMenuItem = createMenuItem<DefaultThemeMenu>("Set default", RIGHT_ARROW);
defaultThemeMenuItem->module = module;
menu->addChild(defaultThemeMenuItem);

