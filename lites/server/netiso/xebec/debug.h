/* $Header: /cvsroot/xmach/lites/server/netiso/xebec/debug.h,v 1.2 2000/10/27 01:58:48 welchd Exp $ */
/* $Source: /cvsroot/xmach/lites/server/netiso/xebec/debug.h,v $ */

#define OUT stdout

extern int	debug[128];

#ifdef DEBUG
extern int column;

#define IFDEBUG(letter) \
	if(debug['letter']) { 
#define ENDDEBUG  ; (void) fflush(stdout);}

#else 

#define STAR *
#define IFDEBUG(letter)	 //*beginning of comment*/STAR
#define ENDDEBUG	 STAR/*end of comment*//

#endif DEBUG

