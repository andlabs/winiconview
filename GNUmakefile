# 13 december 2015

# TODO allow mingw-w64

OUT = winiconview.exe
OBJDIR = .obj

CFILES = \
	main.c \
	mainwin.c \
	panic.c

HFILES = \
	winiconview.h \
	winapi.h \
	resources.h

RCFILES = \
	resources.rc

# TODO /Wall does too much
# TODO -Wno-switch equivalent
# TODO /sdl turns C4996 into an ERROR
# TODO loads of warnings in the system header files
# TODO /analyze requires us to write annotations everywhere
# TODO undecided flags from qo?
CFLAGS += \
	/W4 \
	/wd4100 \
	/TC \
	/bigobj /nologo \
	/RTC1 /RTCc /RTCs /RTCu

# TODO subsystem version
LDFLAGS += \
	/largeaddressaware /nologo /incremental:no \
	user32.lib kernel32.lib gdi32.lib ole32.lib comctl32.lib

ifneq ($(NODEBUG),1)
	CFLAGS += /Zi
	CXXFLAGS += /Zi
	LDFLAGS += /debug
endif

OFILES := \
	$(CFILES:%=$(OBJDIR)/%.o) \
	$(RCFILES:%=$(OBJDIR)/%.o)

# TODO use $(CC), $(CXX), $(LD), $(RC), and $(CVTRES)

$(OUT): $(OFILES)
	@link /out:$(OUT) $(OFILES) $(LDFLAGS)
	@echo ====== Linked $(OUT)

# TODO can we put /Fd$@.pdb in a variable?
$(OBJDIR)/%.c.o: %.c $(HFILES) | $(OBJDIR)
ifeq ($(NODEBUG),1)
	@cl /Fo:$@ /c $< $(CFLAGS)
else
	@cl /Fo:$@ /c $< $(CFLAGS) /Fd$@.pdb
endif
	@echo ====== Compiled $<

$(OBJDIR)/%.rc.o: %.rc $(HFILES) | $(OBJDIR)
	@rc /nologo /v /fo $@.res $<
	@cvtres /nologo /out:$@ $@.res
	@echo ====== Compiled $<

$(OBJDIR):
	@mkdir $@

clean:
	rm -rf $(OBJDIR) $(OUT) $(OUT:%.exe=%.pdb)
.PHONY: clean
