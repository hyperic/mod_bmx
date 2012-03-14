# Microsoft Developer Studio Project File - Name="mod_bmx" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=mod_bmx - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "mod_bmx.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "mod_bmx.mak" CFG="mod_bmx - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "mod_bmx - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "mod_bmx - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "mod_bmx - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MD /W3 /O2 /Oy- /Zi /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "BMX_DECLARE_EXPORT" /I "../../include" /I "../../srclib/apr/include" /I "../../srclib/apr-util/include" /I "$(APACHE2_HOME)/include" /Fd"Release\mod_bmx_src" /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG" /d BIN_NAME="mod_bmx.so" /d LONG_NAME="Core bmx_module for Apache" /I "../../srclib/apr/include" /I "$(APACHE2_HOME)/include"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib /nologo /subsystem:windows /dll /machine:I386 /out:"Release/mod_bmx.so"
# ADD LINK32 libhttpd.lib libaprutil-1.lib libapr-1.lib kernel32.lib /nologo /subsystem:windows /dll /debug /incremental:no /machine:I386 /out:"Release/mod_bmx.so" /libpath:"..\..\Release" /libpath:"..\..\srclib\apr\Release" /libpath:"..\..\srclib\apr-util\Release" /libpath:"$(APACHE2_HOME)/lib" /base:"0x46430000" /opt:ref
# Begin Special Build Tool
TargetPath=.\Release\mod_bmx.so
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
PostBuild_Cmds=if exist $(TargetPath).manifest mt.exe -manifest $(TargetPath).manifest -outputresource:$(TargetPath);2
# End Special Build Tool

!ELSEIF  "$(CFG)" == "mod_bmx - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MDd /W3 /GX /Od /Zi /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Od /Zi /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "BMX_DECLARE_EXPORT" /I "../../include" /I "../../srclib/apr/include" /I "../../srclib/apr-util/include" /I "$(APACHE2_HOME)/include" /Fd"Debug\mod_bmx_src" /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG" /d BIN_NAME="mod_bmx.so" /d LONG_NAME="Core bmx_module for Apache" /I "../../srclib/apr/include" /I "$(APACHE2_HOME)/include"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /out:"Debug/mod_bmx.so"
# ADD LINK32 libhttpd.lib libaprutil-1.lib libapr-1.lib kernel32.lib /nologo /subsystem:windows /dll /incremental:no /debug /machine:I386 /libpath:"..\..\Debug" /libpath:"..\..\srclib\apr\Debug" /libpath:"..\..\srclib\apr-util\Debug" /libpath:"$(APACHE2_HOME)/lib" /out:"Debug/mod_bmx.so" /base:"0x46430000"
# Begin Special Build Tool
TargetPath=.\Debug\mod_bmx.so
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
PostBuild_Cmds=if exist $(TargetPath).manifest mt.exe -manifest $(TargetPath).manifest -outputresource:$(TargetPath);2
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "mod_bmx - Win32 Release"
# Name "mod_bmx - Win32 Debug"
# Begin Source File

SOURCE=.\mod_bmx.c
# End Source File
# Begin Source File

SOURCE=.\mod_bmx.rc
# End Source File
# Begin Source File

SOURCE=.\mod_bmx.h
# End Source File
# End Target
# End Project
