
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
CD = $(UTILSPATH)cd

ifeq ($(OS),Windows_NT)
  MAKE = gmake
#  MAKE =  $(EMAKE)
else
  MAKE = make
endif


.PHONY : all clean allclean
.PHONY : diagrams user_design user_design_clean api_guide api_guide_clean design_doc design_doc_clean

all: diagrams user_design api_guide design_doc

clean : user_design_clean api_guide_clean design_doc_clean

allclean: clean

diagrams:
	$(MAKE) -C diagrams/ all

user_design:
	$(DOXYGEN) docs.cfg 1>output.txt

user_design_clean:
	$(RM) -R ../../docs/user_guide
	$(RM) -R output.txt

api_guide:
	cd ./content_pages/api_guide &&  $(MAKE) api_guide

api_guide_clean:
	 $(RM) -R ../../docs/api_guide
	 $(RM) -R ./content_pages/api_guide/output.txt

design_doc:
	cd ./content_pages/design_doc && $(MAKE) design_doc

design_doc_clean:
	 $(RM) -R ../../docs/design_doc
	 $(RM) -R ./content_pages/design_doc/output.txt

# Nothing beyond this point
