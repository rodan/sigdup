
PROJ = sigdup

TLPI_DIR  = ../lib/tlpi-lib
TLPI_SRC  = ${TLPI_DIR}/error_functions.c ${TLPI_DIR}/get_num.c
TLPI_OBJ  = $(patsubst ${TLPI_DIR}/%.c,build/lib/tlpi/%.o,${TLPI_SRC})

LIBS_OBJ = $(TLPI_OBJ)

INCLUDES  := -I ./ -I $(TLPI_DIR) -I /usr/local/include/ 
LOCAL_SRC_C := main.c ini.c lib_convert.c list.c sig_mng.c zcrc.c
SRC        = $(LOCAL_SRC_C) $(TLPI_SRC)
#EXOBJECT   = $(PROJ).o

LOCAL_OBJECTS     = $(patsubst %.c,build/%.o,${LOCAL_SRC_C})

# Append specific CFLAGS/LDFLAGS
CFLAGS_REL	 += -O3
LDFLAGS_REL	 += -Wl,-s
CFLAGS_DBG	 += -O1 -ggdb -DNDEBUG # -pg
LDFLAGS_DBG	 += # -pg

MANDATORY_CFLAGS=-fPIC -pedantic -Wall -Wextra -Wno-sign-compare
MANDATORY_LDFLAGS=-L/usr/local/lib -lzip

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

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CTAGS       := exuberant-ctags
	MAKE		:= make
endif
ifeq ($(UNAME_S),FreeBSD)
	CTAGS		:= exctags
	MAKE		:= gmake
endif

ifeq ($(CC),gcc)
  TARGET_CFLAGS += # -Wa,-ahl=$(@:.o=.s)
else ifeq ($(CC),cc)
  TARGET_CFLAGS += -Wno-nullability-extension
endif

COVERITY_SCAN_TOKEN := $(shell cat ../../.secrets.COVERITY_SCAN_TOKEN 2>/dev/null)

.PHONY: all
.PHONY: clean
.PHONY: lib

all : libs version.h $(PROJ) tags
libs : $(LIBS_OBJ)
#$(PROJ) : libs version.h

version.h: $(SRC)
	@../scripts/setver.sh

build/lib/tlpi/%.o: ${TLPI_DIR}/%.c
	@bash ../lib/tlpi-lib/Build_ename.sh > ../lib/tlpi-lib/ename.c.inc
	@mkdir -p build/lib/tlpi
	@echo -e " * $(@F)"
	@@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -Wno-write-strings -c $< -o $@

build/%.o: %.c
	@mkdir -p build
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(TARGET_CFLAGS) -c $< -o $@

$(PROJ): $(LOCAL_OBJECTS) $(TLPI_OBJ)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(TARGET_LDFLAGS) $(LOCAL_OBJECTS) $(TLPI_OBJ) -o $@ && size $@

clean :
	@${RM} -rf build
	@${RM} -f ${PROJ}
	@${RM} -rf cov-int

clean-lib:
	@${RM} -rf build/lib

tags: $(SRCS)
	@echo "Generating tags .."
	@$(CTAGS) -R

cppcheck: $(PROJ)
	@cppcheck --quiet --force --enable=warning --enable=performance --enable=information --enable=performance --enable=portability -I ./ ./

scan-build: clean
	@scan-build make

coverity: clean clean-lib
	@cov-build --dir cov-int $(MAKE)
	@tar czvf build/cov-$(PROJ).tgz cov-int
	@curl --form token=$(COVERITY_SCAN_TOKEN) \
	  --form email=2b4eda@subdimension.ro \
	  --form file=@build/cov-$(PROJ).tgz \
	  --form version="$(shell ../scripts/getver.sh -i ./version.h -t MAJ.MINbBUILD)" \
	  --form description="Description" \
	  https://scan.coverity.com/builds?project=rodan%2Fsigdup

force_look:
	@true
