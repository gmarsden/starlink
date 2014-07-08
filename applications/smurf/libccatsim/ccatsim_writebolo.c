/*
*+
*  Name:
*     ccatsim_writebolo

*  Purpose:
*     Write bolo position info to ndf extension

*  Language:
*     Starlink ANSI C

*  Invocation:
*     ccatsim_writebolo(const ccatsim_data *data, int indf, int *status);

*  Arguments:
*     data = const ccatsim_data * (Given)
*        data structure containing file info
*     indf = int (Given)
*        NDF identifier.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Write bolo position info to ndf extension.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-07-08 (AGM):
*        Initial Version
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 Univeristy of British Columbia.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 3 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful,but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public
*     License along with this program; if not, write to the Free
*     Software Foundation, Inc., 51 Franklin Street,Fifth Floor, Boston,
*     MA 02110-1301, USA

*  Bugs:
*     {note_any_bugs_here}
*-
*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "ccatsim.h"

#define FUNC_NAME "ccatsim_writebolo"

#define EXTENSION "CCATSIM"

void ccatsim_writebolo(const ccatsim_data *data, int indf, int *status)
{
  double * fplanex = NULL; /* X coordinates in radians */
  double * fplaney = NULL; /* Y coordinates in radians */
  unsigned int i;          /* loop counter */
  double xdir;             /* direction of +x */

  herr_t h5error;          /* hdf5 error status */
  hsize_t hdim;             /* dimension */
  char message[CCATSIM_MESSAGE_LEN]; /* error message */

  HDSLoc *bololoc = NULL;  /* hds locator for bolopos extension */
  HDSLoc *tloc = NULL;     /* hds locator for dataset */
  void *pntr = NULL;       /* mapped data pointer */
  int dim[1];              /* hds dimension */


  if (*status == SAI__OK) {

    /* check that data has been loaded */
    if (!data->isopen) {
      ccatsim_error("ccatsim data has not been loaded", status);
      return;
    }

    /* expected dimensions for DETRA and DETDEC */
    hdim = (hsize_t)(data->ndet);

    /* check dimensions for DET_RA */
    ccatsim_check_dset(data, CCATSIM_DETRA_NAME, CCATSIM_DETRA_RANK,
                       &hdim, CCATSIM_DETRA_UNIT, status);
    if (*status != SAI__OK) return;

    /* check dimensions for DET_DEC */
    ccatsim_check_dset(data, CCATSIM_DETDEC_NAME, CCATSIM_DETDEC_RANK,
                       &hdim, CCATSIM_DETDEC_UNIT, status);
    if (*status != SAI__OK) return;

    /* allocate memory */
    fplanex = astMalloc( (data->ndet)*sizeof(*fplanex) );
    fplaney = astMalloc( (data->ndet)*sizeof(*fplaney) );

    /* read DET_RA */
    h5error = H5LTread_dataset(data->file_id, CCATSIM_DETRA_NAME,
                               H5T_NATIVE_DOUBLE, fplanex);
    if (h5error < 0) {
      snprintf(message, CCATSIM_MESSAGE_LEN,
               "could not read dataset '%s'", CCATSIM_DETRA_NAME);
      ccatsim_error(message, status);
      return;
    }

    /* read DET_DEC */
    h5error = H5LTread_dataset(data->file_id, CCATSIM_DETDEC_NAME,
                             H5T_NATIVE_DOUBLE, fplaney);
    if (h5error < 0) {
      snprintf(message, CCATSIM_MESSAGE_LEN,
               "could not read dataset '%s'", CCATSIM_DETDEC_NAME);
      ccatsim_error(message, status);
      return;
    }

    /* if focal plane rotation is radec, switch X direction*/
    xdir = (strcmp(data->fplane_rot, "radec") == 0) ? -1.0 : 1.0;

    /* convert from deg to radians (and swap direction if necessary) */
    if (fplanex && fplaney) {
      for (i=0; i<(unsigned int)(data->ndet); i++) {
	fplanex[i] *= xdir * AST__DD2R;
	fplaney[i] *= AST__DD2R;
      }
    }

    /* create ndf extension */
    ndfXnew(indf, CCATSIM_BOLOPOS_NAME, CCATSIM_BOLOPOS_TYPE, 0, NULL, &bololoc, status);

    dim[0] = data->ndet;

    /* store x positions */
    datNew(bololoc, CCATSIM_BOLOARR_X, CCATSIM_BOLOARR_TYPE, 1, dim, status);
    datFind(bololoc, CCATSIM_BOLOARR_X, &tloc, status);
    datMap(tloc, CCATSIM_BOLOARR_TYPE, "WRITE", 1, dim, &pntr, status);
    if (*status == SAI__OK)
      memcpy(pntr, fplanex, dim[0]*sizeof(*fplanex));
    datAnnul(&tloc, status);

    /* store y positions */
    datNew(bololoc, CCATSIM_BOLOARR_Y, CCATSIM_BOLOARR_TYPE, 1, dim, status);
    datFind(bololoc, CCATSIM_BOLOARR_Y, &tloc, status);
    datMap(tloc, CCATSIM_BOLOARR_TYPE, "WRITE", 1, dim, &pntr, status);
    if (*status == SAI__OK)
      memcpy(pntr, fplaney, dim[0]*sizeof(*fplaney));
    datAnnul(&tloc, status);

    /* clean up */
    datAnnul(&bololoc, status);

    astFree(fplanex);
    astFree(fplaney);
  }
}
