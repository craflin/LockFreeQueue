@echo off

if not exist Build\mare\mare.exe call Ext\mare\build.bat --buildDir=Build/mare --outputDir=Build/mare --sourceDir=Ext/mare/src
if not "%1"=="" (Build\mare\mare.exe %*) else Build\mare\mare.exe --vcxproj=2013

