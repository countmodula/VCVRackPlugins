//----------------------------------------------------------------------------
//	/^M^\ Count Modula Plugin for VCV Rack - theme menus
// common menus for selecting themes
//  Copyright (C) 2019  Adam Verspaget
//----------------------------------------------------------------------------

// add main theme menu item
ThemeMenu *themeMenuItem = createMenuItem<ThemeMenu>("Theme", RIGHT_ARROW);
themeMenuItem->module = module;
menu->addChild(themeMenuItem);

// add the default theme menu item
DefaultThemeMenu *defaultThemeMenuItem = createMenuItem<DefaultThemeMenu>("Default Theme", RIGHT_ARROW);
defaultThemeMenuItem->module = module;
menu->addChild(defaultThemeMenuItem);
