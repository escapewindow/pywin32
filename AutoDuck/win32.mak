# MAKEFILE
# Builds documentation for win32 extensions using the AUTODUCK tool
#

!include "common_top.mak"

TARGET  = Python Win32 Extensions
GENDIR  = ..\build\Temp\Help
TITLE   = $(TARGET) Help
DOCHDR  = $(TARGET) Reference

SOURCE_DIR = ../win32/src
HELP_DIR   = ../win32/help
SOURCE  = $(SOURCE_DIR)/*.cpp \
	  $(SOURCE_DIR)/*.h \
	  $(HELP_DIR)/*.d \
	  $(SOURCE_DIR)/perfmon/*.cpp \
	  $(SOURCE_DIR)/win32net/*.cpp \
	  $(SOURCE_DIR)/win32wnet/*.cpp \
	  $(SOURCE_DIR)/win32print/*.cpp \
	  $(GENDIR)/win32evtlog.d $(GENDIR)/win32event.d $(GENDIR)/win32file.d \
	  $(GENDIR)/win32service.d $(GENDIR)/win32pipe.d $(GENDIR)/win32security.d \
	  $(GENDIR)/win32process.d $(GENDIR)/wincerapi.d

# Help and Doc targets
all: help htmlhlp

help : $(GENDIR) "..\$(TARGET).hlp"

htmlhlp: $(GENDIR) "..\$(TARGET).chm"

doc : "$(TARGET).doc"

clean: cleanad

$(GENDIR)/win32file.d: $(SOURCE_DIR)/win32file.i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

$(GENDIR)/win32event.d: $(SOURCE_DIR)/win32event.i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

$(GENDIR)/win32evtlog.d: $(SOURCE_DIR)/win32evtlog.i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

$(GENDIR)/win32service.d: $(SOURCE_DIR)/win32service.i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

$(GENDIR)/win32pipe.d: $(SOURCE_DIR)/win32pipe.i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

$(GENDIR)/win32security.d: $(SOURCE_DIR)/$(*B).i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

$(GENDIR)/win32process.d: $(SOURCE_DIR)/$(*B).i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

$(GENDIR)/wincerapi.d: $(SOURCE_DIR)/$(*B).i
	makedfromi.py -o$*.d $(SOURCE_DIR)/$(*B).i

!include "common.mak"
