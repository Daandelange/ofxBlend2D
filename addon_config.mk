meta:
	ADDON_NAME = ofxBlend2D
	ADDON_DESCRIPTION = OF wrapper for libBlend2d, a multithreaded 2d vector renderer.
	ADDON_AUTHOR = Daan de Lange
	ADDON_TAGS = "Geometry" "Graphics" "Bridges"
	ADDON_URL = https://github.com/daandelange/ofxBlend2D

common:
	#ADDON_DEPENDENCIES = ofxFps

	# Reference : https://blend2d.com/doc/build-instructions.html#BuildingWithoutCMake
	# Note: ADDON_DEFINES seems to be ignored by make ... using CFLAGS too (duplicated code)
	# - - - -
	# Better always optimise builds !
	ADDON_DEFINES = NDEBUG
	ADDON_CFLAGS = -DNDEBUG
	# Force ASMJIT static build
	ADDON_DEFINES += ASMJIT_STATIC
	ADDON_CFLAGS += -DASMJIT_STATIC

	# Optional settings
	# - - - -
	# Enable to build Blend2d statically
	ADDON_DEFINES += BLEND2D_STATIC
	ADDON_CFLAGS += -DBLEND2D_STATIC

	# Recomended optimisations by Blend2D
	# - - - -
	# Forces optimisation, even in debug mode (can also be `-O3` which often doesn't impact performance, but increases binary size)
	ADDON_CFLAGS += -O2
	# Better code generated as the compiler doesn't have to assume semantic interposition (required on GCC compilers only!)
	#ADDON_CFLAGS += -fno-semantic-interposition
	# Blend2D may benefit from auto-vectorization, if not done automatically by the C++ compiler.
	ADDON_CFLAGS += -ftree-vectorize
	# Blend2D API uses explicit public visibility so everything else should be hidden by default. (required on GCC compilers!)
	#ADDON_CFLAGS += -fvisibility=hidden
	ADDON_CFLAGS += -fvisibility-inlines-hidden

	# Optional recommended optimisations (maybe breaking your ofApp compilation)
	# - - - -
	# Blend2D doesn't use runtime type information or dynamic casts so you can turn this feature off
	#ADDON_CFLAGS = -fno-rtti
	# Blend2D doesn't check errno after calling math functions, this option may help the C++ compiler to inline some intrinsic functions like sqrt()
	#ADDON_CFLAGS = -fno-math-errno
	# Blend2D doesn't use exceptions so you can turn them off
	#ADDON_CFLAGS = -fno-exceptions
	# Blend2D has its own initialization step, which initializes everything required to make the library functional, thus thread-safe statics are not needed to guard initialization of static variables
	#ADDON_CFLAGS = -fno-threadsafe-statics
	# Blend2D doesn't compare pointers of constants so it's perfectly fine to merge constants having the same values
	#ADDON_CFLAGS = -fmerge-all-constants

	# Manually add includes
	ADDON_INCLUDES = src/
	ADDON_INCLUDES += libs/asmjit/include
	ADDON_INCLUDES += libs/blend2d/include
	#ADDON_SOURCES = libs/asmjit/src/*
	#ADDON_SOURCES += libs/blend2d/src/*.cpp
	ADDON_INCLUDES += libs/asmjit
	ADDON_INCLUDES += libs/blend2d

	# Force-exclude these includes
	ADDON_INCLUDES_EXCLUDE = libs/asmjit/asmjit-testing/%
	ADDON_INCLUDES_EXCLUDE += libs/blend2d/blend2d-testing/%
	ADDON_INCLUDES_EXCLUDE += libs/blend2d/build/%

	# Note: ADDON_SOURCES are automatically parsed
	# Force-exclude these sources
	ADDON_SOURCES_EXCLUDE = libs/asmjit/asmjit-testing/%
	ADDON_SOURCES_EXCLUDE += libs/blend2d/blend2d-testing/%
	ADDON_SOURCES_EXCLUDE += libs/blend2d/build/%

	# Older config
	# Note = Doesn't compile on linux without these
	##ADDON_DEFINES += BL_TARGET_OPT_SSE4_2
	#ADDON_DEFINES += BL_BUILD_OPT_SSE4_2
	##ADDON_CFLAGS += -DBL_TARGET_OPT_SSE4_2
	#ADDON_CFLAGS += -DBL_BUILD_OPT_SSE4_2
	##ADDON_DEFINES += BL_TARGET_OPT_SSE2
	#ADDON_DEFINES += BL_BUILD_OPT_SSE2
	##ADDON_CFLAGS += -DBL_TARGET_OPT_SSE2
	#ADDON_CFLAGS += -DBL_BUILD_OPT_SSE2


	# Extra feature you can enable depending on your hardware
	# Each CPU will need a different config. (all might not be supported by your CPU!)
	# More features enabled = more blend2d speed.
	# Either check your CPU capabilities and disable unsupported features
	# Or uncomment feature blocks starting from the bottom until compilation succeeds (sorting: from old to modern)

	# SSE1 (always required?)
	ADDON_CFLAGS += -msse

	# SSE2
	ADDON_CFLAGS += -msse2
	#ADDON_CFLAGS += -mfpmath=sse
	ADDON_CFLAGS += -DBL_BUILD_OPT_SSE2
	ADDON_DEFINES += BL_BUILD_OPT_SSE2

	# SSE3
	ADDON_CFLAGS += -msse3
	ADDON_CFLAGS += -DBL_BUILD_OPT_SSE3
	ADDON_DEFINES += BL_BUILD_OPT_SSE3

	# SSSE3
	ADDON_CFLAGS += -mssse3
	ADDON_CFLAGS += -DBL_BUILD_OPT_SSSE3
	ADDON_DEFINES += BL_BUILD_OPT_SSSE3

	# SSE4.1
	ADDON_CFLAGS += -msse4.1
	ADDON_CFLAGS += -DBL_BUILD_OPT_SSE4_1
	ADDON_DEFINES += BL_BUILD_OPT_SSE4_1

	# SSE4.2
	ADDON_CFLAGS += -msse4.2
	ADDON_CFLAGS += -mpopcnt
	ADDON_CFLAGS += -mpclmul
	ADDON_CFLAGS += -DBL_BUILD_OPT_SSE4_2
	ADDON_DEFINES += BL_BUILD_OPT_SSE4_2

	# AVX1
	ADDON_CFLAGS += -mavx
	#ADDON_CFLAGS += -mpopcnt #also required if not already enabled above
	#ADDON_CFLAGS += -mpclmul #also required if not already enabled above
	ADDON_CFLAGS += -DBL_BUILD_OPT_AVX
	ADDON_DEFINES += BL_BUILD_OPT_AVX

	# AVX2
	ADDON_CFLAGS += -mbmi
	ADDON_CFLAGS += -mbmi2
	ADDON_CFLAGS += -mavx2
	ADDON_CFLAGS += -DBL_BUILD_OPT_AVX2
	ADDON_DEFINES += BL_BUILD_OPT_AVX2

	# AVX2-FMA
	ADDON_CFLAGS += -mfma

	# ASIMD (enable on aarch32 & 6aarch4)
	#ADDON_CFLAGS += -mfpu=neon-vfpv4
	#ADDON_CFLAGS += -DBL_BUILD_OPT_ASIMD
	#ADDON_DEFINES += BL_BUILD_OPT_ASIMD

	# AVX512 support (modern CPUs)
	#ADDON_CFLAGS += -mavx512f
	#ADDON_CFLAGS += -mavx512bw
	#ADDON_CFLAGS += -mavx512dq
	#ADDON_CFLAGS += -mavx512cd
	#ADDON_CFLAGS += -mavx512vl

	# Linux clang / llvm
    

osx:
	# Fixes weird curl ldap linker issue
	ADDON_FRAMEWORKS = LDAP

linux64:
	#ADDON_PKG_CONFIG_LIBRARIES = libc libm libpthread librt
	ADDON_LDFLAGS = -lrt

linuxarmv7l:


vs:
