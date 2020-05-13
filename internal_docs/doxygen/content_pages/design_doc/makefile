
include ../../../../ethfw_tools_path.mak

export DOXYGEN ?= doxygen
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
.PHONY : design_doc design_doc_clean

all: design_doc

clean : design_doc_clean

allclean: clean

design_doc:
	$(DOXYGEN) doxyfile 1>output.txt

design_doc_clean:
	$(RM) -R ../../../docs/design_doc
	$(RM) -R output.txt

# Nothing beyond this point
