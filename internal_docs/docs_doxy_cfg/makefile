
# include ../../packages/ti/build/Rules.make

export DOXYGEN_VERSION ?= 1.8.13
# export DOXYGEN_PATH ?= $(TOOLS_INSTALL_PATH)
export DOXYGEN_PATH ?= /datalocal/prasad/
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
.PHONY : release_notes release_notes_clean

all: user_design release_notes

clean : release_notes_clean user_design_clean

allclean: clean

user_design:
	$(DOXYGEN) docs.cfg 1>output.txt

user_design_clean:
	$(RM) -R ../../docs/user_guide
	$(RM) -R output.txt



release_notes: release_notes_clean
	$(ECHO) Release Notes...
	$(MAKE) -C content_pages/release_notes
	$(ECHO) Release Notes Done!!!
	$(ECHO) ""

release_notes_clean:
	$(MAKE) -C content_pages/release_notes clean
# Nothing beyond this point
# Nothing beyond this point
