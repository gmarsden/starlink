/*
*+
*  Name:
*     ccatsim_fill_smfHead

*  Purpose:
*     Populate smfHead with ccatsim specific information

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_fill_smfHead(ccatsim_data *data, smfHead *hdr, int *status);

*  Arguments:
*     data = ccatsim_data * (Given)
*        Pointer to data structure from which hdr info will be read
*     hdr = smfHead* (Given/Returned)
*        Pointer to the smfHead that will be filled with ccatsim information
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     This function populates a smfHead with ccatsim specific
*     information.

*  Notes:
*     - The ccatsim data file stores offets in RA/Dec. For now, assume this
*       is equivalent to az/el. This will get sorted as the simulator evolves.
*     - The offsets are stored in file as degrees. The hdr expects them in
*       radians.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-16 (AGM):
*        Initial version
*     2014-06-27 (AGM):
*        Handle focal plane rotation types
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

void ccatsim_fill_smfHead(ccatsim_data *data, smfHead *hdr, int *status) {

  double * fplanex = NULL; /* X coordinates in radians */
  double * fplaney = NULL; /* Y coordinates in radians */
  unsigned int i;          /* loop counter */
  double xdir;             /* direction of +x */

  herr_t h5error;          /* hdf5 error status */
  int ndim;                /* rank */
  hsize_t dim;             /* dimension */
  char message[CCATSIM_MESSAGE_LEN]; /* error message */

  /* azoff and eloff are the Azimuth and Elevation tangent plane offsets
     measured in radians for each bolometer */

  /* allocate memory for LUTs and copy */
  if (*status == SAI__OK) {

    /* check that data has been loaded */
    if (!data->isopen) {
      ccatsim_error("ccatsim data has not been loaded", status);
      return;
    }

    /* expected dimensions for DETRA and DETDEC */
    dim = data->ndet;

    /* check dimensions for DET_RA */
    ccatsim_check_dset(data, CCATSIM_DETRA_NAME, CCATSIM_DETRA_RANK,
                       &dim, CCATSIM_DETRA_UNIT, status);
    if (*status != SAI__OK) return;

    /* check dimensions for DET_DEC */
    ccatsim_check_dset(data, CCATSIM_DETDEC_NAME, CCATSIM_DETDEC_RANK,
                       &dim, CCATSIM_DETDEC_UNIT, status);
    if (*status != SAI__OK) return;

    /* allocate memory */
    hdr->ndet = data->ndet;
    fplanex = astMalloc( (hdr->ndet)*sizeof(*fplanex) );
    fplaney = astMalloc( (hdr->ndet)*sizeof(*fplaney) );

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
      for (i = 0; i < hdr->ndet; i++) {
	fplanex[i] *= xdir * AST__DD2R;
	fplaney[i] *= AST__DD2R;
      }
    }

    /* now store in the header */
    hdr->fplanex = fplanex;
    hdr->fplaney = fplaney;

  }

}
