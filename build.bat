@echo off

if not defined VSINSTALLDIR (
    call "c:\program files\microsoft visual studio\2022\community\vc\auxiliary\build\vcvarsall.bat" x64
)

set rootDir=%cd%

set compilerFlags= -MTd -nologo -fp:fast -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -D_CRT_SECURE_NO_WARNINGS -FC -Z7

set includeDirs= /I %rootDir%\include\glm\

set linkDirs= /LIBPATH:%rootDir%/include/glm/

set linkerFlags= /SUBSYSTEM:WINDOWS -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

IF NOT EXIST .\build mkdir .\build
pushd .\build

REM C:\Users\Marcm\Downloads\imgui-1.91.5\imgui-1.91.5\examples\example_sdl3_opengl3
REM if not exist ImGui.lib (
REM    cl  -FoImGui.obj %includeDirs% /link %linkDirs% %linkerFlags% /c
REM    lib ImGui.obj
REM )

cl %compilerFlags% ..\\win_main.cpp /Fe.\jf.exe %includeDirs% /link %linkDirs% %linkerFlags%

popd
