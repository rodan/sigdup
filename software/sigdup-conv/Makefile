
PROJ = sigdup-conv

TLPI_DIR  = ../lib/tlpi-lib
TLPI_SRC  = ${TLPI_DIR}/error_functions.c ${TLPI_DIR}/get_num.c
TLPI_OBJ  = $(patsubst ${TLPI_DIR}/%.c,build/lib/tlpi/%.o,${TLPI_SRC})

SALEAE_DIR  = ../lib/saleae
SALEAE_SRC  = ${SALEAE_DIR}/saleae_api.c
SALEAE_OBJ  = $(patsubst ${SALEAE_DIR}/%.c,build/lib/saleae/%.o,${SALEAE_SRC})

LIBS_OBJ = $(TLPI_OBJ) $(SALEAE_OBJ)

INCLUDES  := -I ./ -I $(TLPI_DIR) -I $(SALEAE_DIR) 
LOCAL_SRC_C := main.c
SRC        = $(LOCAL_SRC_C) $(TLPI_SRC) $(SALEAE_SRC)
#EXOBJECT   = $(PROJ).o
EXOUTPUT   = $(PROJ)

LOCAL_OBJECTS     = $(patsubst %.c,build/%.o,${LOCAL_SRC_C})

# Append specific CFLAGS/LDFLAGS
CFLAGS_REL	 += -O3
LDFLAGS_REL	 += -Wl,-s
CFLAGS_DBG	 += -O1 -ggdb -DNDEBUG # -pg
LDFLAGS_DBG	 += # -pg

MANDATORY_CFLAGS=-fPIC -pedantic -Wall -Wextra -Wno-sign-compare -Wa,-ahl=$(@:.o=.s) -fopenmp
MANDATORY_LDFLAGS=-lzip -fopenmp

DEBUG := $(shell grep "^#define CONFIG_DEBUG" config.h)
ifeq ($(DEBUG),)
TARGET	:= RELEASE
TARGET_CFLAGS	+= $(MANDATORY_CFLAGS) $(CFLAGS_REL)
TARGET_LDFLAGS	+= $(MANDATORY_LDFLAGS) $(LDFLAGS_REL)
else
TARGET	:= DEBUG
TARGET_CFLAGS	+= $(MANDATORY_CFLAGS) $(CFLAGS_DBG)
TARGET_LDFLAGS	+= $(MANDATORY_LDFLAGS) $(LDFLAGS_DBG)
endif

COVERITY_SCAN_TOKEN := $(shell cat ../.secrets.COVERITY_SCAN_TOKEN 2>/dev/null)

.PHONY: all
.PHONY: clean
.PHONY: tags
.PHONY: lib

all : libs version.h $(EXOUTPUT)
libs : $(LIBS_OBJ)
$(EXOUTPUT) : libs version.h

version.h: $(SRC)
	@../tools/setver.sh

build/lib/tlpi/%.o: ${TLPI_DIR}/%.c
	@bash ../lib/tlpi-lib/Build_ename.sh > ../lib/tlpi-lib/ename.c.inc
	@mkdir -p build/lib/tlpi
	@echo -e " * $(@F)"
	@@$(CC) $(INCLUDES) $(TARGET_CFLAGS) $(CFLAGS) -Wno-write-strings -c $< -o $@

build/lib/saleae/%.o: ${SALEAE_DIR}/%.c
	@mkdir -p build/lib/saleae
	@echo -e " * $(@F)"
	@@$(CC) $(INCLUDES) $(TARGET_CFLAGS) $(CFLAGS) -Wno-write-strings -c $< -o $@

build/%.o: %.c
	@mkdir -p build
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) $(CFLAGS) -c $< -o $@

%.o: %.c
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -c $< -o $@

$(EXOUTPUT): $(LOCAL_OBJECTS) $(TLPI_OBJ) $(SALEAE_OBJ)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(TARGET_LDFLAGS) $(LOCAL_OBJECTS) $(TLPI_OBJ) $(SALEAE_OBJ) -o $@ && size $@

clean :
	@${RM} -rf build
	@${RM} -f ${PROJ}
	@${RM} -rf cov-int

clean-lib:
	@${RM} -rf build/lib

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

cppcheck: $(PROJ)
	@cppcheck --quiet --force --enable=warning --enable=performance --enable=information --enable=performance --enable=portability -I ./ ./

scan-build: clean
	@scan-build make

coverity: clean clean-lib
	@cov-build --dir cov-int make
	@tar czvf /tmp/$(PROJ).tgz cov-int
	@curl --form token=$(COVERITY_SCAN_TOKEN) \
	  --form email=2b4eda@subdimension.ro \
	  --form file=@/tmp/$(PROJ).tgz \
	  --form version="$(shell ./tools/getver.sh -i ./version.h -t MAJ.MINbBUILD)" \
	  --form description="Description" \
	  https://scan.coverity.com/builds?project=rodan%2Fthpp

force_look:
	@true
