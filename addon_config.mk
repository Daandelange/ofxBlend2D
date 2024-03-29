meta:
	ADDON_NAME = ofxBlend2D
	ADDON_DESCRIPTION = OF wrapper for libBlend2d, a multithreaded 2d vector renderer.
	ADDON_AUTHOR = Daan de Lange
	ADDON_TAGS = "Geometry" "Graphics" "Bridges"
	ADDON_URL = https://github.com/daandelange/ofxBlend2D

common:
	#ADDON_DEPENDENCIES = ofxFps

	# Recomended optimisation by Blend2D
	ADDON_CFLAGS = -O2
	#ADDON_CFLAGS = -fno-semantic-interposition -ftree-vectorize

	# Manually add includes
	ADDON_INCLUDES = src/
	ADDON_INCLUDES += libs/asmjit/include
	ADDON_INCLUDES += libs/blend2d/include
	#ADDON_SOURCES = libs/asmjit/src/*
	#ADDON_SOURCES += libs/blend2d/src/*.cpp
	ADDON_INCLUDES += libs/asmjit/src
	ADDON_INCLUDES += libs/blend2d/src

	# Force-exclude these includes
	ADDON_INCLUDES_EXCLUDE = libs/asmjit/test/%
	ADDON_INCLUDES_EXCLUDE += libs/blend2d/test/%
	ADDON_INCLUDES_EXCLUDE += libs/blend2d/build/%

	# Note: ADDON_SOURCES are automatically parsed
	# Force-exclude these sources
	ADDON_SOURCES_EXCLUDE = libs/asmjit/test/%
	ADDON_SOURCES_EXCLUDE += libs/blend2d/test/%
	ADDON_SOURCES_EXCLUDE += libs/blend2d/build/%

	# See : https://blend2d.com/doc/build-instructions.html#BuildingWithoutCMake
	ADDON_DEFINES = NDEBUG
    ADDON_DEFINES += ASMJIT_STATIC
	# ADDON_DEFINES seems to be ignored by make ... using CFLAGS too
	ADDON_CFLAGS += -DNDEBUG
    ADDON_CFLAGS += -DASMJIT_STATIC

    # Extra feature you can enable depending on your hardware

    # SSE2
    ADDON_DEFINES += BL_BUILD_OPT_SSE2
    ADDON_CFLAGS += -DBL_BUILD_OPT_SSE2
    ADDON_CFLAGS += -msse
    #ADDON_CFLAGS += -msse

    # SSE4.2
    # Note = Doesn't compile on linux without these
    ADDON_DEFINES += BL_BUILD_OPT_SSE4_2
    ADDON_CFLAGS += -DBL_BUILD_OPT_SSE4_2
    ADDON_CFLAGS += -msse4.2

    # Linux clang / llvm
    

osx:
	# Fixes weird curl ldap linker issue
	ADDON_FRAMEWORKS = LDAP

linux64:
    #ADDON_PKG_CONFIG_LIBRARIES = libc libm libpthread librt
    ADDON_LDFLAGS = -lrt

linuxarmv7l:


vs:
