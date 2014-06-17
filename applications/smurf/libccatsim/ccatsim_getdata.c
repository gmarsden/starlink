/*
*+
*  Name:
*     ccatsim_getdata

*  Purpose:
*     Read detector data into array

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_getdata(ccatsim_data *data, double *dataptr, int *status);

*  Arguments:
*     data = ccatsim_data * (Given)
*        Pointer to data structure from which hdr info will be read
*     dataptr = double * (Returned)
*        Pointer to data array. Memory should already be allocated.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Read detector data into data array. data are stored with bolo index
*     varying fastest.

*  Notes:
*     Data are stored on disk with bolo index varying

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-17 (AGM):
*        Initial version

*  Copyright:
*     Copyright (C) 2006 Particle Physics and Astronomy Research Council.
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

#define FUNC_NAME "ccatsim_getdata"

#define EXTENSION "CCATSIM"

void ccatsim_getdata(ccatsim_data *data, double *dataptr, int *status) {

  herr_t h5error;          /* hdf5 error status */
  hsize_t dims[CCATSIM_DETDATA_RANK]; /* dimensions */
  char message[CCATSIM_MESSAGE_LEN]; /* error message */

  if (*status == SAI__OK) {

    /* check that data has been loaded */
    if (!data->isopen) {
      ccatsim_error("ccatsim data has not been loaded", status);
      return;
    }

    /* expected dimensions for DETRA and DETDEC */
    dims[0] = (hsize_t)(data->nsamp);
    dims[1] = (hsize_t)(data->ndet);

    /* check dimensions for DET_DATA
       not currently worried about units */
    ccatsim_check_dset(data, CCATSIM_DETDATA_NAME, CCATSIM_DETDATA_RANK,
                       dims, NULL, status);
    if (*status != SAI__OK) return;

    /* read DET_RA */
    h5error = H5LTread_dataset(data->file_id, CCATSIM_DETDATA_NAME,
                               H5T_NATIVE_DOUBLE, dataptr);
    if (h5error < 0) {
      snprintf(message, CCATSIM_MESSAGE_LEN,
               "could not read dataset '%s'", CCATSIM_DETDATA_NAME);
      ccatsim_error(message, status);
      return;
    }

  }

}
