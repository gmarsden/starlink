// -*-c++-*-
#ifndef _TclQueryUtil_h_
#define _TclQueryUtil_h_

/*
 * E.S.O. - VLT project/ESO Archive
 * $Id: TclQueryUtil.h,v 1.5 2001/08/27 10:10:36 abrighto Exp $
 *
 * TclQueryUtil.h - utility routines for Tcl catalog commands
 *
 * who             when       what
 * --------------  --------   ----------------------------------------
 * Allan Brighton  14 Jun 96  Created
 */


#include <tcl.h>
#include <stdio.h>

class AstroQuery;
class WorldCoords;

int genAstroQuery(Tcl_Interp* interp, int argc, char* argv[], 
		  AstroQuery& q, WorldOrImageCoords& pos1, WorldOrImageCoords& pos2, 
		  char* equinoxStr, FILE* feedback, CatalogInfoEntry* entry);


#endif /* _TclQueryUtil_h_ */
