
include ../../ethfw_tools_path.mak

export DOXYGEN_VERSION ?= 1.8.13
export DOXYGEN_PATH ?= $(PSDK_PATH)
export DOXYGEN ?= $(DOXYGEN_PATH)/doxygen-$(DOXYGEN_VERSION)/bin/doxygen

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
.PHONY : user_design user_design_clean

all: user_design

clean : user_design_clean

allclean: clean

user_design:
	$(DOXYGEN) docs.cfg 1>output.txt

user_design_clean:
	$(RM) -R ../../docs/user_guide
	$(RM) -R output.txt

# Nothing beyond this point
# Nothing beyond this point
