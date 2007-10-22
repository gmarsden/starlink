/*
*+
*  Name:
*     smf_fits_export2DA

*  Purpose:
*     Convert AstFitsChan FITS headers to character array

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Subroutine

*  Invocation:
*     smf_fits_export2DA ( const AstFitsChan *fitschan, int *ncards, 
*                          char *fitsrec, 
*                          int *status )

*  Arguments:
*     fitschan = const AstFitsChan* (Given)
*        AstFitsChan containing FITS headers
*     ncards = int* (Returned)
*        Number of cards in FITS header
*     fitsrec = char* (Returned)
*        Character array for converted FITS headers. This should be large enough
*        to hold SC2STORE__MAXFITS headers.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This routine converts the contents of an AstFitsChan
*     into a character array for storage by the SCUBA-2 DA system.

*  Authors:
*     Tim Jenness (TIMJ)
*     Jen Balfour (UBC)
*     {enter_new_authors_here}

*  History:
*     2006-11-01 (JB):
*        Original version
*     2007-10-22 (TIMJ):
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) Science and Technology Facilities Council.
*     Copyright (C) 2005-2006 Particle Physics and Astronomy Research
*     Council. University of British Columbia. All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 3 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public
*     License along with this program; if not, write to the Free
*     Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*     MA 02111-1307, USA

*  Bugs:
*     {note_any_bugs_here}
*-
*/

/* Standard includes */
#include <stdio.h>
#include <string.h>

/* Starlink includes */
#include "sae_par.h"
#include "ast.h"
#include "mers.h"
#include "msg_par.h"
#include "prm_par.h"

/* SMURF includes */
#include "smf.h"
#include "smurf_par.h"

/* Simple default string for errRep */
#define FUNC_NAME "smf_fits_export2DA"

void smf_fits_export2DA ( const AstFitsChan *fitschan, int *ncards, 
                          char *fitsrec,
                          int *status ) {

  /* Local variables */
  char *outpos = NULL;    /* current position in output buffer */
  char card[SZFITSCARD];  /* temporary buffer for current card */   
  int found;              /* Boolean to indicate if a card was found */
  int i;                  /* Loop counter */

  /* Check status */
  if (*status != SAI__OK) return;

  /* Find the number of cards in this AstFitsChan, and make
     sure that it is no larger than the maximum allowed */
   *ncards = astGetI ( fitschan, "Ncard" );
   if ( *ncards > SC2STORE__MAXFITS ) {
      *status = SAI__ERROR;
      msgSeti("NC", *ncards);
      msgSeti("MC", SC2STORE__MAXFITS);
      errRep( FUNC_NAME, 
              "Number of FITS cards ^NC exceeds maximum allowed (^MC)",
              status );
      return;
   }
  
   /* Rewind */
   astClear ( fitschan, "Card");

   /* Retrieve all the FITS headers and store them in the character array */
   outpos = fitsrec;
   for ( i = 0; i <= *ncards; i++ ) {
      found = astFindFits ( fitschan, "%f", card, 1 );
      if ( found ) {
	 strncpy ( outpos, card, SZFITSCARD );	 
	 outpos += 80;
      } else {
	break;
      }
   } 

}
