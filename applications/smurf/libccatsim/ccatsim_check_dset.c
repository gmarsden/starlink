/*
*+
*  Name:
*     ccatsim_check_dset

*  Purpose:
*     Check metadata for dataset

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_check_dset(ccatsim_data *data, const char *dsetname, int rank,
*                             const hsize_t *dims, const char *units, int *status);

*  Arguments:
*     data = ccatsim_data * (Given)
*        Pointer to data structure
*     dsetname = const char * (Given)
*        Name of dataset to check
*     rank = int (Given)
*        Expected rank of dataset
*     dims = hsize_t * (Given)
*        Expected dimensions of dataset
*     units = const char * (Given)
*        Expected units of dataset (set to NULL to skip test)
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Check that dataset has expected properties. Error message printed and status
*     set to SAI__ERROR if tests fail.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-16 (AGM):
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

#define FUNC_NAME "ccatsim_check_dset"

#define EXTENSION "CCATSIM"

void ccatsim_check_dset(ccatsim_data *data, const char *dsetname, int rank,
                        const hsize_t *dims, const char *units, int *status)
{
  int i;                   /* loop counter */

  herr_t h5error;          /* hdf5 error status */
  int thisrank;            /* rank */
  hsize_t *thisdims;       /* dimensions */
  char thisunits[CCATSIM_MESSAGE_LEN]; /* units */

  char message[CCATSIM_MESSAGE_LEN]; /* error message */

  /* skip test if status is bad */
  if (*status != SAI__OK) return;

  /* check that file is open */
  if (!data->isopen) {
    ccatsim_error("ccatsim data has not been loaded", status);
    return;
  }

  /* get rank */
  h5error = H5LTget_dataset_ndims(data->file_id, dsetname, &thisrank);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not get rank of dataset '%s'", dsetname);
    ccatsim_error(message, status);
    return;
  }

  /* check rank */
  if (thisrank != rank) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "dataset '%s' has bad rank (expected %d, got %d)",
             dsetname, rank, thisrank);
    ccatsim_error(message, status);
    return;
  }

  /* allocate dims */
  thisdims = astMalloc( rank*sizeof(*thisdims) );

  /* get dims */
  h5error = H5LTget_dataset_info(data->file_id, dsetname, thisdims,
                                 NULL, NULL);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not get dimension of dataset '%s'", dsetname);
    ccatsim_error(message, status);
    return;
  }

  /* check dims */
  for (i=0; i<thisrank; i++) {
    if (thisdims[i] != dims[i]) {
      snprintf(message, CCATSIM_MESSAGE_LEN,
               "dataset '%s' has bad dimesion %d (expected %d, got %d)",
               dsetname, rank, (int)(dims[i]), (int)(thisdims[i]));
      ccatsim_error(message, status);
      return;
    }
  }

  /* check units (if requested) */
  if (thisunits != NULL) {

    /* get units */
    h5error = H5LTget_attribute_string(data->file_id, dsetname,
                                       CCATSIM_UNITS_NAME, thisunits);
    if (h5error < 0) {
      snprintf(message, CCATSIM_MESSAGE_LEN,
               "could not get units of dataset '%s'", dsetname);
      ccatsim_error(message, status);
      return;
    }

    /* check units */
    if (units != NULL) {
      if (strncmp(thisunits, units, CCATSIM_MESSAGE_LEN) != 0) {
        snprintf(message, CCATSIM_MESSAGE_LEN,
                 "dataset '%s' has wrong units (expected %s, got %s)",
                 dsetname, units, thisunits);
        ccatsim_error(message, status);
        return;
      }
    }

  }

  /* success */
}
