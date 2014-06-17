/*
*+
*  Name:
*     ccatsim_openfile

*  Purpose:
*     Open HDF5 for reading and fill data structure

*  Language:
*     Starlink ANSI C

*  Invocation:
*     ccatsim_openfile(const char *filename, ccatsim_data *data, int *status);

*  Arguments:
*     filename = const char * (Given)
*        filename of HDF5 file to open
*     data = ccatsim_data * (Returned)
*        data structure containing file info
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Open an HDF5 for reading.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-11 (AGM):
*        Initial Version
*     2014-06-16 (AGM):
*        Introduce ccatsim_data structure
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

#define FUNC_NAME "ccatsim_openfile"

#define EXTENSION "CCATSIM"

void ccatsim_openfile(const char *filename, ccatsim_data *data, int *status)
{
  herr_t h5err;           /* hdf5 error code */
  hid_t file_id;          /* hdf5 file pointer */
  char message[CCATSIM_MESSAGE_LEN]; /* error message */
  hsize_t dims[2];        /* data dimensions */

  /* set filename */
  data->filename = filename;

  /* Turn off automatic HDF5 error reporting */
  h5err = H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  if (h5err < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN, "%s",
             "could not turn off HDF5 error reporting");
    ccatsim_error(message, status);
    return;
  }

  /* open file */
  file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (file_id < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN, "%s", "could not open HDF5 file");
    ccatsim_error(message, status);
    return;
  }
  data->file_id = file_id;

  /* get dimensions using DATASET_NAME_DET_DATA */
  h5err = H5LTget_dataset_info(file_id, CCATSIM_DETDATA_NAME, dims,
                               NULL, NULL);
  if (h5err < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN, "%s",
             "could not retrieve data dimensions");
    ccatsim_error(message, status);
    return;
  }
  data->nsamp = dims[0];
  data->ndet = dims[1];

  /* success */
  data->isopen = 1;
}
