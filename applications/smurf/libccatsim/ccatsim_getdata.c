/*
*+
*  Name:
*     ccatsim_getdata

*  Purpose:
*     Read detector data into array

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_getdata(const ccatsim_data *data, double *dataptr,
*                          unsigned char *quaptr, int *status);

*  Arguments:
*     data = const ccatsim_data * (Given)
*        Pointer to data structure from which hdr info will be read
*     dataptr = double * (Returned)
*        Pointer to data array. Memory should already be allocated.
*     quaptr = unsigned char * (Returned)
*        Pointer to data quality flag array. Memory for output should already
*        be allocated. Quality not calculated if quaptr == NULL.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Read detector data into data array and quality flag into quality array.
*     Data are stored with bolo index varying fastest.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-17 (AGM):
*        Initial version
*     2014-07-03 (AGM):
*        Make ccatsim_data* const
*     2014-07-23 (AGM):
*        Add quality pointer
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

#define FUNC_NAME "ccatsim_getdata"

#define EXTENSION "CCATSIM"

void ccatsim_getdata(const ccatsim_data *data, double *dataptr,
                     unsigned char *quaptr, int *status) {

  herr_t h5error;          /* hdf5 error status */
  hsize_t dims[CCATSIM_DETDATA_RANK]; /* dimensions */
  char message[CCATSIM_MESSAGE_LEN]; /* error message */

  int ti;                  /* time loop index */
  int di;                  /* detector loop index */
  int *scannum = NULL;     /* scan number (for quality) */

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

    /* build quality if requested */
    if (quaptr != NULL) {

      /* read scan_number to build quality array */

      /* check scan number */
      ccatsim_check_dset(data, CCATSIM_SCANNUM_NAME, CCATSIM_SCANNUM_RANK,
                         dims, NULL, status);
      if (*status != SAI__OK) return;

      /* allocate memory to store scan number */
      scannum = astMalloc(data->nsamp*sizeof(*scannum));

      /* read scan number */
      h5error = H5LTread_dataset(data->file_id, CCATSIM_SCANNUM_NAME,
                                 H5T_NATIVE_INT, scannum);

      if (h5error < 0) {

        snprintf(message, CCATSIM_MESSAGE_LEN,
               "could not read dataset '%s'", CCATSIM_DETDATA_NAME);
        ccatsim_error(message, status);

      } else {

        /* where scannum == -1, set quality to bad (SMF__Q_BADDA) */
        for (ti=0; ti<data->nsamp; ti++) {
          for (di=0; di<data->ndet; di++) {
            quaptr[ti*data->ndet+di] =
              (scannum[ti] == CCATSIM_BAD_SCANNUM) ?
              SMF__Q_BADDA : 0;
          }
        }

      }

      /* free memory */
      scannum = astFree(scannum);
    }

  }

}
