
PROJ = sigdup
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

DEBUG := $(shell grep "^#define CONFIG_DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
TARGET_CFLAGS	+= $(CFLAGS_REL)
TARGET_LDFLAGS	+= $(LDFLAGS_REL)
else
TARGET	:= DEBUG
TARGET_CFLAGS	+= $(CFLAGS_DBG)
TARGET_LDFLAGS	+= $(LDFLAGS_DBG)
endif

COVERITY_SCAN_TOKEN := $(shell cat ../.secrets.COVERITY_SCAN_TOKEN 2>/dev/null)

.PHONY: all
.PHONY: clean
.PHONY: tags
.PHONY: lib

all : version.h $(EXOUTPUT) tags
$(EXOUTPUT) : lib
lib : $(TLPI_LIB)

version.h: $(SRC)
	@./tools/setver.sh

$(TLPI_LIB): force_look
	@echo "looking into subdir $(TLPI_DIR)"
	@$(MAKE) -C $(TLPI_DIR) -j1 $(MFLAGS)

%.o: %.c
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -c $< -o $@

$(EXOUTPUT): $(LOCAL_OBJECTS) $(EXOBJECT)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(TARGET_LDFLAGS) -lzip $(LOCAL_OBJECTS) $(TLPI_LIB) -o $@ && size $@

$(EXOBJECT): $(PROJ_SRC)
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -g -c $< -o $@

allgen : ${GEN_EXE}

clean :
	@${RM} ${EXE} *.o ${PROJ} tags ${TLPI_LIB}
	@${RM} -rf cov-int

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

${EXE} : ${TLPI_LIB}		# True as a rough approximation

cppcheck: $(PROJ)
	@cppcheck --quiet --force --enable=warning --enable=performance --enable=information --enable=performance --enable=portability -I ./ ./

scan-build: clean
	@scan-build make

coverity: clean
	@cov-build --dir cov-int make
	@tar czvf /tmp/$(PROJ).tgz cov-int
	@curl --form token=$(COVERITY_SCAN_TOKEN) \
	  --form email=2b4eda@subdimension.ro \
	  --form file=@/tmp/$(PROJ).tgz \
	  --form version="$(shell ./tools/getver.sh -i ./version.h -t MAJ.MINbBUILD)" \
	  --form description="Description" \
	  https://scan.coverity.com/builds?project=rodan%2Fsigdup

force_look :
	true
