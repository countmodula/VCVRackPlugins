# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

SLUG = CountModula

VERSION = 0.6.0

# FLAGS will be passed to both the C and C++ compiler
FLAGS +=
CFLAGS +=
CXXFLAGS +=

ifdef DEVBUILD
	FLAGS += -DDEV
endif

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine.
LDFLAGS +=

# Add .cpp and .c files to the build
SOURCES += $(wildcard src/*.cpp)
SOURCES += $(wildcard src/modules/*.cpp)
SOURCES += $(wildcard src/lib/*.cpp)
SOURCES += $(wildcard src/components/*.cpp)

ifdef DEVBUILD
	# include any modules that are in development
	SOURCES += $(wildcard src/dev/*.cpp)
else
	
endif

# Add files to the ZIP package when running `make dist`
# The compiled plugin is automatically added.
ifdef DEVBUILD
	#add all of the resources
	DISTRIBUTABLES += $(wildcard LICENSE*) res
else
	#only add the ones that we want to publish
	DISTRIBUTABLES += res/AnalogueShiftRegister.svg
	DISTRIBUTABLES += res/Attenuator.svg
	DISTRIBUTABLES += res/BinarySequencer.svg
	DISTRIBUTABLES += res/Blank12HP.svg
	DISTRIBUTABLES += res/Blank16HP.svg
	DISTRIBUTABLES += res/Blank20HP.svg
	DISTRIBUTABLES += res/Blank24HP.svg
	DISTRIBUTABLES += res/Blank4HP.svg
	DISTRIBUTABLES += res/Blank8HP.svg
	DISTRIBUTABLES += res/BooleanAND.svg
	DISTRIBUTABLES += res/BooleanOR.svg
	DISTRIBUTABLES += res/BooleanVCNOT.svg
	DISTRIBUTABLES += res/BooleanXOR.svg
	DISTRIBUTABLES += res/BurstGenerator.svg
	DISTRIBUTABLES += res/Comparator.svg
	DISTRIBUTABLES += res/CVSpreader.svg
	DISTRIBUTABLES += res/EventArranger.svg
	DISTRIBUTABLES += res/G2T.svg
	DISTRIBUTABLES += res/GateDelay.svg
	DISTRIBUTABLES += res/GateDelayMT.svg
	DISTRIBUTABLES += res/ManualCV.svg
	DISTRIBUTABLES += res/ManualGate.svg
	DISTRIBUTABLES += res/MatrixMixer.svg
	DISTRIBUTABLES += res/MinimusMaximus.svg
	DISTRIBUTABLES += res/MorphShaper.svg
	DISTRIBUTABLES += res/Multiplexer.svg
	DISTRIBUTABLES += res/Mute-iple.svg
	DISTRIBUTABLES += res/PolyrhythmicGenerator.svg
	DISTRIBUTABLES += res/ShepardGenerator.svg
	DISTRIBUTABLES += res/SRFlipFlop.svg
	DISTRIBUTABLES += res/VCFrequencyDivider.svg
	DISTRIBUTABLES += res/VCPolarizer.svg
	DISTRIBUTABLES += res/VoltageControlledSwitch.svg
	DISTRIBUTABLES += res/VoltageInverter.svg
	
	#and the components
	DISTRIBUTABLES += res/Components
endif

# Include the VCV Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

