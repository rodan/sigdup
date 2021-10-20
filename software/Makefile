
PROJ = pg
INCLUDES  := -I ./ -I tlpi-lib/
LOCAL_SRC := $(wildcard *.c)
PROJ_SRC   = main.c
SRC        = $(PROJ_SRC) $(LOCAL_SRC)
EXOBJECT   = $(PROJ).o
EXOUTPUT   = $(PROJ)

LOCAL_OBJECTS     = $(LOCAL_SRC:%.c=%.o)

include Makefile.inc

# Append specific CFLAGS/LDFLAGS
CFLAGS_REL	 += -Os -fdata-sections -ffunction-sections -fomit-frame-pointer
LDFLAGS_REL	 += -Wl,-s
CFLAGS_DBG	 += -O1 -ggdb
LDFLAGS_DBG	 += 

DEBUG := $(shell grep "^\#define CONFIG_DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
CFLAGS	+= $(CFLAGS_REL)
LDFLAGS	+= $(LDFLAGS_REL)
else
TARGET	:= DEBUG
CFLAGS	+= $(CFLAGS_DBG)
LDFLAGS	+= $(LDFLAGS_DBG)
endif

.PHONY: all
.PHONY: clean
.PHONY: tags
.PHONY: lib


all : lib version.h $(EXOUTPUT) tags

lib : $(TLPI_LIB)

version.h: $(SRC)
	@./tools/setver.sh

$(TLPI_LIB): force_look
	@echo "looking into subdir $(TLPI_DIR)"
	@$(MAKE) -C $(TLPI_DIR) -j1 $(MFLAGS)

%.o: %.c
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(EXOUTPUT): $(LOCAL_OBJECTS) $(EXOBJECT)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(LDFLAGS) -lzip $(LOCAL_OBJECTS) $(TLPI_LIB) -o $@ && size $@

$(EXOBJECT): $(PROJ_SRC)
	@$(CC) $(INCLUDES) $(CFLAGS) -g -c $< -o $@

allgen : ${GEN_EXE}

clean : 
	${RM} ${EXE} *.o ${PROJ} tags ${TLPI_LIB}

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

${EXE} : ${TLPI_LIB}		# True as a rough approximation

cppcheck: $(PROJ)
	@cppcheck --quiet --force --enable=warning --enable=performance --enable=information --enable=performance --enable=portability -I ./ ./

scan-build: clean
	@scan-build make

force_look :
	true
