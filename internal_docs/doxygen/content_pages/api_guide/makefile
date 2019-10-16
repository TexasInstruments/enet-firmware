
include ../../../../ethfw_tools_path.mak

export DOXYGEN_VERSION ?= 1.8.13
export DOXYGEN_PATH ?= $(PSDK_PATH)
export DOXYGEN ?= $(DOXYGEN_PATH)/doxygen-$(DOXYGEN_VERSION)/bin/doxygen
export ETHFW_PATH = ../../../../

# Commands commonly used within the make files
RM = $(UTILSPATH)rm -f
MV = $(UTILSPATH)mv
RMDIR = $(UTILSPATH)rm -rf
MKDIR = $(UTILSPATH)mkdir
ECHO = @$(UTILSPATH)echo

ifeq ($(OS),Windows_NT)
  MAKE = gmake
#  MAKE =  $(EMAKE)
else
  MAKE = make
endif


.PHONY : all clean allclean
.PHONY : api_guide  api_guide_clean

all: api_guide

clean : api_guide_clean

allclean: clean

api_guide:
	$(DOXYGEN) doxyfile 1>output.txt

api_guide_clean:
	$(RM) -R ../../../docs/api_guide
	$(RM) -R output.txt

# Nothing beyond this point
# Nothing beyond this point
