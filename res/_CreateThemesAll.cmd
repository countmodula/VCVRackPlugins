@set /P sure="ARE YOU SURE YOU WISH TO GENERATE THEMES FOR ALL MODULES IN THIS FOLDER Y/N? "

@if "%sure%" EQU "Y" goto themeCheck
@if "%sure%" EQU "y" goto themeCheck
@goto end

:themeCheck
@set /P theme="Enter specific theme name or leave blank for all themes: "
@if "%theme%" EQU "" goto all

:specific
C:\Workspace\Tools\ThemeUtility\bin\Debug\ThemeUtility -t:%theme% .\
@goto end

:all
C:\Workspace\Tools\ThemeUtility\bin\Debug\ThemeUtility .\

:end
@pause