#!/bin/csh -f
# 
# Mach Operating System
# Copyright (c) 1992 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon 
# the rights to redistribute these changes.
#
#
# HISTORY
# $Log: mkconfig.csh,v $
# Revision 1.2  2000/10/27 01:58:45  welchd
#
# Updated to latest source
#
# Revision 1.1.1.1  1995/03/02  21:49:41  mike
# Initial Lites release from hut.fi
#
# Revision 2.2  92/07/08  16:28:39  mrt
# 	Redid script into separate sed call so it would work with the
# 	CMU-BSD 4.3 sed and the gnu-sed.
# 	[92/07/08            mrt]
# 
# Revision 2.1  92/04/21  17:11:50  rwd
# BSDSS
# 
#
#

set CONFIG="${argv[1]}"

shift argv

(\
sed -n \
    -e "/^[^#]/d" \
    -e 's;	; ;g' \
    -e "s;^# *\([^ ]*\)[ ]*=[ ]*\[\(.*\)\].*;\1#\2;p" ${argv}\
;\
echo +${CONFIG} | \
sed  -n -e "s;[-+];#&;gp"\
;\
sed -n \
    -e '/^#/d' \
    -e '/^$/d' \
    -e 's;^\([^#]*\).*#[ 	]*<\(.*\)>[ 	]*$;SKIPIT\2#\1;' \
    -e "/^SKIPIT/\!s;\([^#]*\).*;#\1;" \
    -e 's;^SKIPIT;;' \
    -e 's;[ 	]*$;;' \
    -e 's;^\!\(.*\);\1#\!;' \
    -e p ${argv}\
) | \
awk '-F#' '\
part == 0 && $1 != "" {\
    m[$1]=m[$1] " " $2;\
    next;\
}\
part == 0 && $1 == "" {\
    for (i=NF;i>1;i--){\
	s=substr($i,2);\
	c[++na]=substr($i,1,1);\
	a[na]=s;\
    }\
    while (na > 0){\
	s=a[na];\
	d=c[na--];\
	if (m[s] == "") {\
	    f[s]=d;\
	} else {\
	    nx=split(m[s],x," ");\
	    for (j=nx;j>0;j--) {\
		z=x[j];\
		a[++na]=z;\
		c[na]=d;\
	    }\
	}\
    }\
    part=1;\
    next;\
}\
part != 0 {\
    if ($1 != "") {\
	n=split($1,x,",");\
	ok=0;\
	for (i=1;i<=n;i++) {\
	    if (f[x[i]] == "+") {\
		ok=1;\
	    }\
	}\
	if (NF > 2 && ok == 0 || NF <= 2 && ok != 0) {\
	    print $2; \
	}\
    } else { \
	print $2; \
    }\
}'
