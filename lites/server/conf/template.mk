# 
# Mach Operating System
# Copyright (c) 1992 Carnegie Mellon University
# Copyright (c) 1994 Johannes Helander
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON AND JOHANNES HELANDER ALLOW FREE USE OF THIS
# SOFTWARE IN ITS "AS IS" CONDITION.  CARNEGIE MELLON AND JOHANNES
# HELANDER DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
# WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
#
# HISTORY
# $Log: template.mk,v $
# Revision 1.2  2000/10/27 01:58:45  welchd
#
# Updated to latest source
#
# Revision 1.1.1.1  1995/03/02  21:49:41  mike
# Initial Lites release from hut.fi
#
# $EndLog$
#
#	File:	 conf/template.mk
#	Authors: Mary Thompson, Johannes Helander
#	Date:	 1992, 1994
#


# ${EXPORTBASE}/lites has the machine link.
VPATH		= ..:${EXPORTBASE}/lites

# We want the LITES version & configuration to be part of this name.
# Makeconf defined LITES_CONFIG and
# Makefile-version defined VERSION.

CONFIG          ?=${LITES_${TARGET_MACHINE}_CONFIG:U${LITES_CONFIG:UDEFAULT}}

.if exists( ${MAKETOP}Makefile-version)
.include "${MAKETOP}Makefile-version"
.endif

# set BINARIES to get the osf.obj.mk rules included
BINARIES        =

VMUNIX          = vmunix.${VERSION}.${CONFIG}
OTHERS          = ${VMUNIX}

ILIST           = ${VMUNIX}
IDIR            = /special/

# We are going to use xstrip, so we don't want release to try to strip it again
NOSTRIP         =

DEPENDENCIES	=
SAVE_D		=
MIG_HDRS	= 

# directories that must be created in the object area
MKODIRS 	= serv/


#  Pre-processor environment.
#  config outputs a definition of IDENT.
#  LOCAL_DEFINES can be set by makeoptions in MASTER.local.
#  (eg. -DTIMEZONE=-120)
#  LINENO is (optionally) set from the config file
#  (VOLATILE should be defined as "-Dvolatile=" if your
#  compiler doesn't support volatile declarations.)

VOLATILE        ?=
#XX		= -DBSD=44 -DMACH_IPC_COMPAT=0
DEFINES		= -nostdinc -I- ${MASTER_DEFINES} ${LOCAL_DEFINES} ${IDENT} -DKERNEL ${XX} $(VOLATILE)
#
# The line below should not be here; it overrides the external default.
# BUT ... ux is known not to build with -O2 ...
# Someday this should be fixed.
#CC_OPT_LEVEL	= -O
#
CC_OPT_EXTRA	?= ${LINENO}


# CFLAGS are for all normal files
# DRIVER_CFLAGS are for files marked as device-driver
# NPROFILING_CFLAGS are for files which support profile and thus
#       should not be compiled with the profiling flags

CFLAGS		= ${DEFINES} ${PROFILING:D-pg -DGPROF}
DRIVER_CFLAGS	=${CFLAGS}
NPROFILING_CFLAGS=${DEFINES} -DGPROF

#INCFLAGS are procesed by genpath and expanded relative to all the 
# sourcedirs, INCDIRS is not expanded.

INCFLAGS	= -I.. -I../../include

# The EXPORTBASE/server directory contains the include files
# in machine

INCDIRS         := -I${EXPORTBASE}/lites ${INCDIRS}

.if (${target_cpu} == "mips" || ${target_cpu} == "i386" || ${target_cpu} == "ns532")
USE_LIBPROF = ${LIBPROF1}
.else
USE_LIBPROF =
.endif 

.if 	defined(PROFILING)
LIBS		= ${USE_LIBPROF} ${LIBMACHID} ${LIBNETNAME} ${LIBTHREADS_P} ${LIBMACH_SA_P}
.else
LIBS		= ${LIBMACHID} ${LIBNETNAME} ${LIBTHREADS} ${LIBMACH_SA}
.endif

#
#  LDOBJS is the set of object files which comprise the server.
#  LDOBJS_PREFIX and LDOBJS_SUFFIX are defined in the machine
#  dependent Makefile (if necessary).
#
LDOBJS=${LDOBJS_PREFIX} ${OBJS} ${LDOBJS_SUFFIX}

#
#  LDDEPS is the set of extra dependencies associated with
#  loading the server
#
#  LDDEPS_PREFIX is defined in the machine 
#  dependent Makefile (if necessary).
#

LDDEPS=${LDDEPS_PREFIX} 

#
#  PRELDDEPS is another set of extra dependencies associated with
#  loading the server.
#  It is defined in the machine dependent Makefile (if necessary).
#

#
#  These macros are filled in by the config program depending on the
#  current configuration.  The MACHDEP macro is replaced by the
#  contents of the machine dependent makefile template and the others
#  are replaced by the corresponding symbol definitions for the
#  configuration.
#

%OBJS

%CFILES

%CFLAGS

%SFILES

%BFILES

%ORDERED

#  All macro definitions should be before this point,
#  so that the machine dependent fragment can redefine the macros.
#  All rules (that use macros) should be after this point,
#  so that they pick up any redefined macro values.

%MACHDEP

.include <${RULES_MK}>

_CC_GENINC_=-I.

.PRECIOUS: Makefile

# Make all the directories in the object directory

.BEGIN:
	-makepath ${MKODIRS}

UX_LDFLAGS	?= ${${TARGET_MACHINE}_LDFLAGS:U${LDFLAGS}}

NEWVERS_DEPS = \
	conf/version.major \
	conf/version.variant \
	conf/version.edit \
	conf/version.patch \
	conf/newvers.sh	\
	conf/copyright

VERSION_FILES_SOURCE = \
	${conf/version.major:P} \
	${conf/version.variant:P} \
	${conf/version.edit:P} \
	${conf/version.patch:P}


${VMUNIX} : ${PRELDDEPS} ${LDOBJS} ${LDDEPS} \
		${CC_DEPS_NORMAL} ${NEWVERS_DEPS} LINKSERVER

# The relink rule allows you to relink the server without checking
#  all the dependencies. 

relink: ${VMUNIX}.relink

${VMUNIX}.relink: ${LDDEPS} ${NEWVERS_DEPS} LINKSERVER

#  We create vers.c/vers.o right here so that the timestamp in vers.o
#  always reflects the time that the vmunix binary is actually created.
#  We link the vmunix binary to "vmunix" so that there is a short name
#  for the most recently created binary in the object directory.

#Need to use MONCRT0 instead of CRT0 when the libraries are profiled.

#.if 	defined(PROFILING)
#CRT 	= ${MONCRT0}
#.else
CRT	= ${CRT0}
#.endif

LINKSERVER: .USE
	@echo "creating vers.o"
	@${RM} ${_RMFLAGS_} vers.c vers.o
	@sh ${conf/newvers.sh:P} ${conf/copyright:P} `cat ${VERSION_FILES_SOURCE}` ${VERSION}
	@${_CC_} -c ${_CCFLAGS_} vers.c
	@${RM} -f ${VMUNIX} ${VMUNIX}.out ${VMUNIX}.unstripped
	@echo "loading ${VMUNIX}"
	ld  -o ${VMUNIX}.out ${UX_LDFLAGS} ${LIBDIRS} \
		${CRT} ${LDOBJS} vers.o -llites ${LIBS} ${LDLIBS} && \
		${MV} ${VMUNIX}.out ${VMUNIX}.unstripped
	-${SIZE} ${VMUNIX}.unstripped
	${CP} ${VMUNIX}.unstripped ${VMUNIX}.out
	${XSTRIP} ${VMUNIX}.out && ${MV} ${VMUNIX}.out ${VMUNIX}
	@${RM} -f vmunix
	ln ${VMUNIX} vmunix
	${CP} ${VMUNIX} ${EXPORTBASE}/special/${VMUNIX}


#
#  OBJSDEPS is the set of files (defined in the machine dependent
#  template if necessary) which all objects depend on (such as an
#  in-line assembler expansion filter
#

${OBJS}: ${OBJSDEPS}

# Use the standard rules with slightly non-standard .IMPSRC

${COBJS}: $${$${.TARGET}_SOURCE}

${SOBJS}: $${$${.TARGET}_SOURCE}

${BOBJS}: $${$${.TARGET}_SOURCE}

#
#  Rules for components which are not part of the kernel proper or that
#  need to be built in a special manner.
#


#  The Mig-generated files go into subdirectories.
#  This target makes sure they exist

.BEGIN : 
	-makepath ${MKODIRS}


#
#  Mach IPC-based interfaces
#

#  Explicit dependencies on generated files,
#  to ensure that Mig has been run by the time
#  these files are compiled.

bsd_server.o: bsd_1_server.c bsd_types_gen.h 

bsd_server_side.o: bsd_types_gen.h bsd_1_server.h

BSD_1_FILES = bsd_1_server.c

serv/bsd_server_side.o : bsd_1.server.h

$(BSD_1_FILES) bsd_1_server.h: bsd_types_gen.h serv/bsd_1.defs
	 $(MIG) $(_MIGFLAGS_) ${DEFINES} -UKERNEL \
		-header /dev/null \
		-user /dev/null \
		-server bsd_1_server.c \
		-sheader bsd_1_server.h \
		 ${serv/bsd_1.defs:P}

SIG_FILES = signal_user.h
# serv/signal_user.c

sendsig.o : signal_user.h

# The C file is patched by hand as I wasn't able to figure out how to
# get MiG to produce the correct code.
$(SIG_FILES): bsd_types_gen.h serv/signal.defs
	$(MIG) $(_MIGFLAGS_) ${DEFINES} -UKERNEL \
		-header signal_user.h \
		-user /dev/null \
		-server /dev/null \
		${serv/signal.defs:P}

# Make syscall.h before any objects
# The kern/* deps are here so that makesyscalls is executed only once.
${OBJS} kern/init_sysent.c kern/syscalls.c : sys/syscall.h
${OBJS} ${BSD_1_FILES} ${SIG_FILES} : bsd_types_gen.h
xxx_bsd_types_gen.c : vnode_if.h
xxx_bsd_types_gen.c : sys/syscall.h
${OBJS} vnode_if.c : vnode_if.h

sys/syscall.h: kern/makesyscalls.sh kern/syscalls.master
	-rm -rf kern sys libjacket libsys
	-mkdir sys kern libjacket libsys
	cd kern;/bin/sh ${kern/makesyscalls.sh:P} ${kern/syscalls.master:P}

# VFS interface
vnode_if.h: kern/vnode_if.sh kern/vnode_if.src
	-rm -f vnode_if.h vnode_if.c
	/bin/sh ${kern/vnode_if.sh:P} ${kern/vnode_if.src:P} gawk


bsd_types_gen_CCTYPE	= host
HOST_CFLAGS		= ${DEFINES}
HOST_LDFLAGS		= ${LDFLAGS}

# -P
xxx_bsd_types_gen.c: serv/bsd_types_gen.c
	${ansi_CPP} ${_CCFLAGS_} ${serv/bsd_types_gen.c:P} > xxx_bsd_types_gen.c
	cat /dev/null >> bsd_types_gen.d
	sed 's/^bsd_types_gen\.o/xxx_bsd_types_gen.c/' \
			bsd_types_gen.d > xxx_bsd_types_gen.c.d
	${RM} bsd_types_gen.d

bsd_types_gen: xxx_bsd_types_gen.c
	( LPATH="${_host_LPATH_}"; export LPATH; \
	${HOST_CC} ${_host_FLAGS_} -o bsd_types_gen.X xxx_bsd_types_gen.c)
	${MV} bsd_types_gen.X bsd_types_gen

bsd_types_gen.h : bsd_types_gen
	./bsd_types_gen > bsd_types_gen.h


ALWAYS:


printenv:
	@echo VPATH=${VPATH}
	@echo INCDIRS=${INCDIRS}

