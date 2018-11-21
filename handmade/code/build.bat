@echo off

IF NOT EXIST ..\..\build mkdir ..\..\build
pushd ..\..\build
cl -nologo -GR- -EHa- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -FC -Z7 ..\handmade\code\win32_handmade.cpp user32.lib gdi32.lib
popd
