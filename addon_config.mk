meta:
	ADDON_NAME = ofxBlend2D
	ADDON_DESCRIPTION = OF wrapper for libBlend2d, a multithreaded 2d vector renderer.
	ADDON_AUTHOR = Daan de Lange
	ADDON_TAGS = "Geometry" "Graphics" "Bridges"
	ADDON_URL = https://github.com/daandelange/ofxBlend2D

common:
	ADDON_DEPENDENCIES = "ofxFps"
	ADDON_INCLUDES = src/
	ADDON_INCLUDES += libs/asmjit/include
	ADDON_INCLUDES += libs/blend2d/include
	#ADDON_SOURCES = libs/asmjit/src/*
	#ADDON_SOURCES += libs/blend2d/src/*.cpp
	ADDON_INCLUDES += libs/asmjit/src
	ADDON_INCLUDES += libs/blend2d/src

	ADDON_SOURCES_EXCLUDE += libs/asmjit/test
	ADDON_SOURCES_EXCLUDE += libs/blend2d/test
	ADDON_SOURCES_EXCLUDE += libs/blend2d/build

	# See : https://blend2d.com/doc/build-instructions.html#BuildingWithoutCMake
	# ADDON_DEFINES seems to be ignored by make ... using CFLAGS too
	ADDON_DEFINES = NDEBUG
	ADDON_DEFINES += BL_BUILD_OPT_SSE2
	ADDON_CPPFLAGS = -DNDEBUG -DBL_BUILD_OPT_SSE2

osx:


linux64:


linuxarmv7l:


vs:
