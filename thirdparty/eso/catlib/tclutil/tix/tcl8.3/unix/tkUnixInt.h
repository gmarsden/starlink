/*
 * tkUnixInt.h --
 *
 *	This file contains declarations that are shared among the
 *	UNIX-specific parts of Tk but aren't used by the rest of
 *	Tk.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id: tkUnixInt.h,v 1.2 2001/08/27 10:11:31 abrighto Exp $
 */

#ifndef _TKUNIXINT
#define _TKUNIXINT

#ifndef _TKINT
#include "tkInt.h"
#endif

/*
 * Prototypes for procedures that are referenced in files other
 * than the ones they're defined in.
 */
#include "tkIntPlatDecls.h"

#endif /* _TKUNIXINT */
