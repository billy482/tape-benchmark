MAKEFLAGS 	+= -rR --no-print-directory

# commands
# TARGET		:= arm-linux-gnueabi-
GCC			:= ccache ${TARGET}gcc
CSCOPE		:= cscope
CTAGS		:= ctags
OBJCOPY		:= ${TARGET}objcopy
STRIP		:= ${TARGET}strip

# variable
NAME		:= tape-benchmark
BIN			:= bin/${NAME}
DIR_NAME	:= $(lastword $(subst /, , $(realpath .)))
VERSION		:= 0.1
#VERSION		:= $(shell git describe)

BUILD_DIR	:= build
DEPEND_DIR	:= depend

SRC_FILES	:= $(sort $(shell test -d src && find src -name '*.c'))
HEAD_FILES	:= $(sort $(shell test -d include -a src && find include src -name '*.h'))

OBJ_FILES	:= $(sort $(patsubst src/%.c,${BUILD_DIR}/%.o,${SRC_FILES}))
DEP_FILES	:= $(sort $(shell test -d ${DEPEND_DIR} && find ${DEPEND_DIR} -name '*.d'))

BIN_DIRS	:= $(sort $(dir ${BIN}))
OBJ_DIRS	:= $(sort $(dir ${OBJ_FILES}))
DEP_DIRS	:= $(patsubst ${BUILD_DIR}/%,${DEPEND_DIR}/%,${OBJ_DIRS})

INCLUDE_DIR	:=

ifeq (${SRC_FILES},)
$(error no source files found)
endif


# compilation flags
CFLAGS		:= -pipe -Os -ggdb3 -Wall -Wextra ${INCLUDE_DIR} -DTAPEBENCHMARK_VERSION=\"${VERSION}\"
LDFLAGS		:= -O2

CSCOPE_OPT	:= -b -R -s src -U


# phony target
.PHONY: all clean cscope ctags distclean lib prepare realclean stat stat-extra TAGS tar

all: binary cscope tags

binary: prepare ${BIN}

check:
	@echo "Checking sources files..."
	@${GCC} -fsyntax-only ${CFLAGS} ${SRC_FILES}

clean:
	@echo " [ RM      ] ${BUILD_DIR}"
	@rm -Rf ${BUILD_DIR}

cscope: cscope.out

ctags TAGS: tags

distclean realclean: clean
	@echo " [ RM      ] ${BIN} ${BIN}.debug cscope.out ${DEPEND_DIR} tags"
	@rm -Rf ${BIN} ${BIN}.debug cscope.out ${DEPEND_DIR} tags

prepare: ${BIN_DIRS} ${DEP_DIRS} ${OBJ_DIRS}

rebuild: clean all

stat:
	@wc $(sort ${SRC_FILES} ${HEAD_FILES} ${YACC_FILES})
	@git diff --cached --stat=${COLUMNS}

stat-extra:
	@c_count -p $(sort ${HEAD_FILES} ${SRC_FILES})

tar: ${NAME}.tar.bz2


# real target
${BIN}: ${OBJ_FILES}
	@echo " [ LD      ] $@"
	@${GCC} -o ${BIN} ${OBJ_FILES} ${CFLAGS} ${LDFLAGS}
	@${OBJCOPY} --only-keep-debug $@ $@.debug
	@${STRIP} $@
	@${OBJCOPY} --add-gnu-debuglink=$@.debug $@
	@chmod -x $@.debug

${BIN_DIRS} ${DEP_DIRS} ${OBJ_DIRS}:
	@echo " [ MKDIR   ] $@"
	@mkdir -p $@

${NAME}.tar.bz2:
	git archive --format=tar --prefix=${DIR_NAME}/ master | bzip2 -9c > $@

cscope.out: ${SRC_FILES}
	@echo " [ CSCOPE  ]"
	@${CSCOPE} ${CSCOPE_OPT}

tags: ${SRC_FILES} ${HEAD_FILES}
	@echo " [ CTAGS   ]"
	@${CTAGS} -R src

# implicit target
${BUILD_DIR}/%.o: src/%.c
	@echo " [ CC      ] $@"
	@${GCC} -c ${CFLAGS} -Wp,-MD,${DEPEND_DIR}/$*.d,-MT,$@ -o $@ $<

ifneq (${DEP_FILES},)
include ${DEP_FILES}
endif

