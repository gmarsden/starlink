/*
*+
*  Name:
*     ccatsim_getstring

*  Purpose:
*     Safely retrieve a single string from dataset

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_getstring(ccatsim_data *data, const char *dsetname, int maxstrlen,
*                            char *string, int *status);

*  Arguments:
*     data = ccatsim_data * (Given)
*        Pointer to data structure
*     dsetname = const char * (Given)
*        Name of dataset to check
*     maxstrlen = int (Given)
*        maximum length of string (include terminator)
*     string = char * (Returned)
*        string stored in dataset
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Safely retrieve a single string from dataset. Checks that dataset is in fact a
*     scalar and that string fits in output buffer.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-26 (AGM):
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

#define FUNC_NAME "ccatsim_getstring"

#define EXTENSION "CCATSIM"

void ccatsim_getstring(ccatsim_data *data, const char *dsetname, int maxstrlen,
                       char *string, int *status)
{
  hid_t dset_id;           /* hdf5 dataset id */
  hid_t dtyp_id;           /* hdf5 datatype id */
  H5T_class_t h5class;     /* hdf5 class type */
  herr_t h5error;          /* hdf5 error status */
  size_t strlen;           /* length of string in dataset */

  char message[CCATSIM_MESSAGE_LEN]; /* error message */

  /* dataset should be scalar */
  int rank=1;
  hsize_t dims[] = {1};

  /* skip test if status is bad */
  if (*status != SAI__OK) return;

  /* check dataset dimensions */
  ccatsim_check_dset(data, dsetname, rank, dims, NULL, status);
  if (*status != SAI__OK) return;

  /* get dataset */
  dset_id = H5Dopen2(data->file_id, dsetname, H5P_DEFAULT);
  if (dset_id < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not open dataset '%s'", dsetname);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

  /* get datatype */
  dtyp_id = H5Dget_type(dset_id);
  if (dtyp_id < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not get data type for dataset '%s'", dsetname);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

  /* check class type */
  h5class = H5Tget_class(dtyp_id);
  if (h5class != H5T_STRING) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "dataset '%s' not is not a string", dsetname);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

  /* check that string is not too long */
  strlen = H5Tget_size(dtyp_id);
  if (strlen > (size_t)(maxstrlen)) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "string in dataset '%s' is too long", dsetname);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

  /* get string */
  h5error = H5LTread_dataset_string(data->file_id, dsetname, string);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read string from dataset '%s'", dsetname);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

 CLEANUP:
  H5Tclose(dtyp_id);
  H5Dclose(dset_id);
}
