#!/bin/sh -
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
# $Log: newvers.sh,v $
# Revision 1.2  2000/10/27 01:58:45  welchd
#
# Updated to latest source
#
# Revision 1.1.1.1  1995/03/02  21:49:41  mike
# Initial Lites release from hut.fi
#
#

copyright="$1"; major="$2"; variant="$3"; edit="$4"; patch="$5"; version="$6";
v="VERSION(${variant}${edit}${patch})" d=`pwd` h=`hostname` t=`date`
if [ -z "$d" -o -z "$h" -o -z "$t" ]; then
    exit 1
fi
CONFIG=`expr "$d" : '.*/\([^/]*\)$'`
d=`expr "$d" : '.*/\([^/]*/[^/]*\)$'`
(
  echo "char ostype[] = \"4.4BSD\";" ;
  echo "char osrelease[] = \"4.4BSD-Lite\";" ;
  echo "char  version_major[]   = \"${major}\";" ;
  echo "char version_variant[]  = \"${variant}\";" ;
  echo "char version_patch[]    = \"${patch}\";" ;
  echo "int  version_edit       = ${edit};" ;
  echo "char  version_config[]   = \"$t; $d ($h)\";" ;
  echo "char version[] = \"${major} ${v}: ${t}; $d ($h)\\n\";" ;
  echo "char version_short[] = \"${version}\\n\";" ;
  echo "char cmu_copyright[] = \"\\" ;
  sed <$copyright -e '/^#/d' -e 's;[ 	]*$;;' -e '/^$/d' -e 's;$;\\n\\;' -e 's/"/\\"/g' ;
  echo "\";";
) > vers.c
if [ -s vers.suffix -o ! -f vers.suffix ]; then
    echo ".${variant}${edit}${patch}.${CONFIG}" >vers.suffix
fi
exit 0
