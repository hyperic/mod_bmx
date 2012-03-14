# Microsoft Developer Studio Generated NMAKE File, Based on mod_bmx.dsp
!IF "$(CFG)" == ""
CFG=mod_bmx - Win32 Release
!MESSAGE No configuration specified. Defaulting to mod_bmx - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "mod_bmx - Win32 Release" && "$(CFG)" != "mod_bmx - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "mod_bmx - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\mod_bmx.so"


CLEAN :
	-@erase "$(INTDIR)\mod_bmx.obj"
	-@erase "$(INTDIR)\mod_bmx.res"
	-@erase "$(INTDIR)\mod_bmx_src.idb"
	-@erase "$(INTDIR)\mod_bmx_src.pdb"
	-@erase "$(OUTDIR)\mod_bmx.exp"
	-@erase "$(OUTDIR)\mod_bmx.lib"
	-@erase "$(OUTDIR)\mod_bmx.pdb"
	-@erase "$(OUTDIR)\mod_bmx.so"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /Zi /O2 /Oy- /I "../../include" /I "../../srclib/apr/include" /I "../../srclib/apr-util/include" /I "$(APACHE2_HOME)/include" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "BMX_DECLARE_EXPORT" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\mod_bmx_src" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mod_bmx.res" /i "../../srclib/apr/include" /i "$(APACHE2_HOME)/include" /d "NDEBUG" /d BIN_NAME="mod_bmx.so" /d LONG_NAME="Core bmx_module for Apache" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mod_bmx.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=libhttpd.lib libaprutil-1.lib libapr-1.lib kernel32.lib /nologo /base:"0x42780000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\mod_bmx.pdb" /debug /out:"$(OUTDIR)\mod_bmx.so" /implib:"$(OUTDIR)\mod_bmx.lib" /libpath:"..\..\Release" /libpath:"..\..\srclib\apr\Release" /libpath:"..\..\srclib\apr-util\Release" /libpath:"$(APACHE2_HOME)/lib" /opt:ref 
LINK32_OBJS= \
	"$(INTDIR)\mod_bmx.obj" \
	"$(INTDIR)\mod_bmx.res"

"$(OUTDIR)\mod_bmx.so" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Release\mod_bmx.so
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\mod_bmx.so"
   if exist .\Release\mod_bmx.so.manifest mt.exe -manifest .\Release\mod_bmx.so.manifest -outputresource:.\Release\mod_bmx.so;2
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ELSEIF  "$(CFG)" == "mod_bmx - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\mod_bmx.so"


CLEAN :
	-@erase "$(INTDIR)\mod_bmx.obj"
	-@erase "$(INTDIR)\mod_bmx.res"
	-@erase "$(INTDIR)\mod_bmx_src.idb"
	-@erase "$(INTDIR)\mod_bmx_src.pdb"
	-@erase "$(OUTDIR)\mod_bmx.exp"
	-@erase "$(OUTDIR)\mod_bmx.lib"
	-@erase "$(OUTDIR)\mod_bmx.pdb"
	-@erase "$(OUTDIR)\mod_bmx.so"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /GX /Zi /Od /I "../../include" /I "../../srclib/apr/include" /I "../../srclib/apr-util/include" /I "$(APACHE2_HOME)/include" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "BMX_DECLARE_EXPORT" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\mod_bmx_src" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MTL=midl.exe
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /win32 
RSC=rc.exe
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\mod_bmx.res" /i "../../srclib/apr/include" /i "$(APACHE2_HOME)/include" /d "_DEBUG" /d BIN_NAME="mod_bmx.so" /d LONG_NAME="Core bmx_module for Apache" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\mod_bmx.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=libhttpd.lib libaprutil-1.lib libapr-1.lib kernel32.lib /nologo /base:"0x42780000" /subsystem:windows /dll /incremental:no /pdb:"$(OUTDIR)\mod_bmx.pdb" /debug /out:"$(OUTDIR)\mod_bmx.so" /implib:"$(OUTDIR)\mod_bmx.lib" /libpath:"..\..\Debug" /libpath:"..\..\srclib\apr\Debug" /libpath:"..\..\srclib\apr-util\Debug" /libpath:"$(APACHE2_HOME)/lib" 
LINK32_OBJS= \
	"$(INTDIR)\mod_bmx.obj" \
	"$(INTDIR)\mod_bmx.res"

"$(OUTDIR)\mod_bmx.so" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

TargetPath=.\Debug\mod_bmx.so
SOURCE="$(InputPath)"
PostBuild_Desc=Embed .manifest
DS_POSTBUILD_DEP=$(INTDIR)\postbld.dep

ALL : $(DS_POSTBUILD_DEP)

# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

$(DS_POSTBUILD_DEP) : "$(OUTDIR)\mod_bmx.so"
   if exist .\Debug\mod_bmx.so.manifest mt.exe -manifest .\Debug\mod_bmx.so.manifest -outputresource:.\Debug\mod_bmx.so;2
	echo Helper for Post-build step > "$(DS_POSTBUILD_DEP)"

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("mod_bmx.dep")
!INCLUDE "mod_bmx.dep"
!ELSE 
!MESSAGE Warning: cannot find "mod_bmx.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "mod_bmx - Win32 Release" || "$(CFG)" == "mod_bmx - Win32 Debug"
SOURCE=.\mod_bmx.c

"$(INTDIR)\mod_bmx.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mod_bmx.rc

"$(INTDIR)\mod_bmx.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)



!ENDIF 

