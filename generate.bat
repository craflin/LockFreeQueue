@echo off

if not exist Build\Debug\.mare\mare.exe call Ext\mare\compile.bat --buildDir=Build/Debug/.mare --outputDir=Build/Debug/.mare --sourceDir=Ext/mare/src
if not "%1"=="" (Build\Debug\.mare\mare.exe %*) else Build\Debug\.mare\mare.exe --vcxproj=2013
