/*
*+
*  Name:
*     ccatsim_fill_smfHead

*  Purpose:
*     Populate smfHead with ccatsim detector positions

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_fill_smfHead(smfHead *hdr, int indf, int *status);

*  Arguments:
*     hdr = smfHead* (Given/Returned)
*        Pointer to the smfHead that will be filled with ccatsim information
*     indf = int (Given)
*        NDF locator for the input data file
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This function populates a smfHead with ccatsim detector positions from the
*     BOLOPOS extension.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  Notes:
*     This function existed in an earlier incarnation, used to read
*     bolo positions from external file format. That functionality was moved
*     to ccatsim_writebolo.c.

*  History:
*     2014-07-08 (AGM):
*        Initial version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 Univeristy of British Columbia.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 2 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful, but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public
*     License along with this program; if not, write to the Free
*     Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*     MA 02110-1301, USA

*  Bugs:
*     {note_any_bugs_here}

*-
*/

#include "ccatsim.h"

#define FUNC_NAME "ccatsim_fill_smfHead"

#define EXTENSION "CCATSIM"

void ccatsim_fill_smfHead(smfHead *hdr, int indf, int *status) {

  double *fplanex = NULL;  /* X coordinates in radians */
  double *fplaney = NULL;  /* Y coordinates in radians */

  HDSLoc *bololoc = NULL;  /* hds locator for bolopos extension */
  HDSLoc *fxloc = NULL;    /* hds locator for x dataset */
  HDSLoc *fyloc = NULL;    /* hds locator for y dataset */

  size_t sizex;            /* Number of FPLANEX coordinates */
  size_t sizey;            /* Number of FPLANEY coordinates */
  double *fpntrx = NULL;   /* mapped FPLANEX */
  double *fpntry = NULL;   /* mapped FPLANEY */
  void *tpntr;             /* temporary pointer */

  if (*status != SAI__OK) return;

  /* Check that we have a valid NDF identifier */
  if (indf == NDF__NOID) {
    *status = SAI__ERROR;
    errRep(" ", FUNC_NAME ": require a valid NDF identifier", status );
    return;
  }

  /* Get the extension and find the required components. */
  ndfXloc(indf, CCATSIM_BOLOPOS_NAME, "READ", &bololoc, status);
  datFind(bololoc, CCATSIM_BOLOARR_X, &fxloc, status);
  datFind(bololoc, CCATSIM_BOLOARR_Y, &fyloc, status);

  /* map numericals them as vectorized _DOUBLEs */
  datMapV(fxloc, CCATSIM_BOLOARR_TYPE, "READ", &tpntr, &sizex, status);
  fpntrx = tpntr;
  datMapV(fyloc, CCATSIM_BOLOARR_TYPE, "READ", &tpntr, &sizey, status);
  fpntry = tpntr;


  /* sanity check */
  if (sizex != sizey && *status == SAI__OK) {
    *status = SAI__ERROR;
    msgSeti( "FX", sizex );
    msgSeti( "FY", sizey );
    errRep( " ", FUNC_NAME ": Possible corrupt file. FPLANEX size != FPLANEY"
	    " (^FX != ^FY)", status);
  }

  /* allocate memory and copy */
  if (*status == SAI__OK) {
    hdr->ndet = sizex;
    fplanex = astMalloc( sizex*sizeof(*fplanex) );
    fplaney = astMalloc( sizex*sizeof(*fplaney) );

    memcpy(fplanex, fpntrx, sizex*sizeof(*fplanex));
    memcpy(fplaney, fpntry, sizey*sizeof(*fplaney));

    /* now store in the header */
    hdr->fplanex = fplanex;
    hdr->fplaney = fplaney;
  }

  /* set instrument */
  hdr->instrument = INST__SWCAM;

  /* free resources */
  datAnnul(&bololoc, status);
  datAnnul(&fyloc, status);
  datAnnul(&fxloc, status);

}
