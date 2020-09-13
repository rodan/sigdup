include ../Makefile.inc

PROJ = pg
INCLUDES  := -I ./ -I ../tlpi-lib/
LOCAL_SRC := $(wildcard *.c)
PROJ_SRC   = $(PROJ).c
SRC        = $(PROJ_SRC) $(LOCAL_SRC)
EXOBJECT   = $(PROJ).o
EXOUTPUT   = $(PROJ)

LOCAL_OBJECTS     = $(LOCAL_SRC:%.c=%.o)

.PHONY: all
.PHONY: clean
.PHONY: tags

all : ${EXOUTPUT} tags

%.o: %.c
	@echo -e " * $(@F)"
	@$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(EXOUTPUT): $(LOCAL_OBJECTS) $(EXOBJECT)
	@echo -e "\n>> Building $@ as target $(TARGET)"
	@$(CC) $(LDFLAGS) $(LOCAL_OBJECTS) $(TLPI_LIB) -o $@

$(EXOBJECT): $(PROJ_SRC)
	@$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

allgen : ${GEN_EXE}


clean : 
	${RM} ${EXE} *.o ${PROJ} tags

tags: $(SRCS)
	@echo "Generating tags .."
	@exuberant-ctags -R

${EXE} : ${TLPI_LIB}		# True as a rough approximation
