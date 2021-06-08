#
# Copyright (c) 1994, The University of Utah and
# the Computer Systems Laboratory at the University of Utah (CSL).
#
# Permission to use, copy, modify and distribute this software is hereby
# granted provided that (1) source code retains these copyright, permission,
# and disclaimer notices, and (2) redistributions including binaries
# reproduce the notices in supporting documentation, and (3) all advertising
# materials mentioning features or use of this software display the following
# acknowledgement: ``This product includes software developed by the Computer
# Systems Laboratory at the University of Utah.''
#
# THE UNIVERSITY OF UTAH AND CSL ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
# IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSL DISCLAIM ANY LIABILITY OF
# ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
#
# CSL requests users of this software to return to csl-dist@cs.utah.edu any
# improvements that they make and grant CSL redistribution rights.
#
# HISTORY
# $Log: gensym.awk,v $
# Revision 1.2  2000/10/27 01:53:08  welchd
#
# Updated to latest source
#
# Revision 1.1.1.1  1995/03/02  21:49:26  mike
# Initial Lites release from hut.fi
#
#

BEGIN {
	bogus_printed = "no"
}

# Start the bogus function just before the first sym directive,
# so that any #includes higher in the file don't get stuffed inside it.
/^[a-z]/ {
	if (bogus_printed == "no")
	{
		print "bogus() {";
		bogus_printed = "yes";
	}
}

# Take an arbitrarily complex C symbol or expression and constantize it.
/^expr/ {
	print "asm (\"";
	if ($3 == "")
		printf "* %s mAgIc%%0\" : : \"i\" (%s));\n", $2, $2;
	else
		printf "* %s mAgIc%%0\" : : \"i\" (%s));\n", $3, $2;
}

# Output a symbol defining the size of a C structure.
/^size/ {
	print "asm (\"";
	printf "* %s_SIZE mAgIc%%0\" : : \"i\" (sizeof(struct %s)));\n",
		toupper($3), $2;
}

# Output a symbol defining the byte offset of an element of a C structure.
/^offset/ {
	print "asm (\"";
	if ($5 == "")
	{
		printf "* %s_%s mAgIc%%0\" : : \"i\" (&((struct %s*)0)->%s));\n",
			toupper($3), toupper($4), $2, $4;
	}
	else
	{
		printf "* %s mAgIc%%0\" : : \"i\" (&((struct %s*)0)->%s));\n",
			toupper($5), $2, $4;
	}
}

# Copy through all preprocessor directives.
/^#/ {
	print
}

END {
	print "}"
}

