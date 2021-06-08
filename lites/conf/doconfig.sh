#!/bin/sh
# 
# Mach Operating System
# Copyright (c) 1994 Johannes Helander
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# JOHANNES HELANDER ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  JOHANNES HELANDER DISCLAIMS ANY LIABILITY OF ANY KIND
# FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
#
#
# HISTORY
# $Log: doconfig.sh,v $
# Revision 1.2  2000/10/27 01:53:08  welchd
#
# Updated to latest source
#
# Revision 1.1.1.2  1995/03/23  01:15:19  law
# lites-950323 from jvh.
#
#
#	File:	conf/doconfig.sh
#	Author:	Johannes Helander, Helsinki University of Technology, 1994.
#
#	Configuration script for Lites.  This script is run by configure.
#

if [ $# != 1 ] ; then
	echo "$0: move-if-change path missing"
	exit 1
fi

moveifchange="$1"

opt="Doopt.sh"
filelist="Filelist"
cleanlist="Cleanlist"
makevar="../conf/Makevar"

awk "
	BEGIN {
		opt = \"$opt\"
		filelist = \"$filelist\"
		cleanlist = \"$cleanlist\"
		makevar = \"$makevar\"
		moveifchange = \"$moveifchange\"
		"'
		printf "#!/bin/sh\n" > opt
		printf "# Automatically generated. Do not edit.\n" > opt
		printf "# Automatically generated. Do not edit.\n" > makevar
		printf "# Automatically generated. Do not edit.\n" > filelist
		printf "# Automatically generated. Do not edit.\n" > cleanlist
		printf "SOURCEFILES = " > filelist
		printf "CLEAN_FILES += " > cleanlist

		# Standard is always true
		options["standard"] = 1;
		options["true"] = 1;
	}
	$1 ~ /^config$/ {
		# We have a list of options separated by spaces pluses or minuses
		# A space or plus means add option
		# A minus means remove option
		# Order is significant (options are processed left to right)
		printf("\t%s\n", $0);
		# This loop handles spaces
		for (f = 2; f <= NF; f++) {
			nx = split($f, xxx, "+");
			# This loop takes care of pluses
			for (x = 1; x <= nx; x++) {
				ny = split(xxx[x], yyy, "-");
				if (yyy[1] !~ /^[ 	]*$/) {
					# print "A " yyy[1]
					# Add pluses
					options[yyy[1]] = 1;
				}
				# This loop handles minuses
				for (y = 2; y <= ny; y++) {
					if (yyy[y] !~ /^[ 	]*$/) {
						# print "D " yyy[y]
						# Remove minuses
						options[yyy[y]] = 0;
					}
				}
			}
		}
	}
	$1 ~ /^options$/ {
		# $2 is condition, $3 is define, $4 is value, $5 is filename
		# XXX handle boolean expressions in $2
		if (options[$2]) {
			val=$4
		} else {
			val=0
		}
		if ($5 != "")
		{
		        printf("rm -f %s.tmp; echo \"#define %s %s\" > %s.tmp && %s %s.tmp %s\n\n", $5, $3, val, $5, moveifchange, $5, $5) > opt
		}
		printf("\\\n	%s ", $5) > cleanlist
	}
	$1 ~ /^makeoptions$/ {
		# XXX handle boolean expressions in $2
		if (options[$2]) {
			printf("%s", $3) > makevar
			for (f = 4; f <= NF; f++)
				printf(" %s", $f) > makevar
			printf("\n") > makevar
		}
	}
	$1 ~ /^fileif$/ {
		# XXX handle boolean expressions in $2
		if (options[$2])
			printf "\\\n	%s ", $3 > filelist
	}
	$1 ~ /^file$/ {
		printf "\\\n	%s ", $2 > filelist
	}
	END {
		print > filelist
		print > cleanlist
#		print "ENDENDENDENDENDENDENDENDENDENDEND"
#		for (x in options)
#			print x, options[x]
	}
'
