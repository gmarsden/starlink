/*
*+
*  Name:
*     ccatsim_getdims

*  Purpose:
*     Get number of detectors and number of time samples from file

*  Language:
*     Starlink ANSI C

*  Invocation:
*     int error = ccatsim_getdims(hid_t file_id, int *ndet, int *nsamp);

*  Arguments:
*     file_id = hid_t (Given)
*        file object of opened file
*     ndet = int * (Returned)
*        number of detectors in dataset
*     nsamp = int * (Returned)
*        number of time samples in dataset

*  Returned:
*     error (int)
*        Non-zero on error.

*  Description:
*     Get dimensions HDF5 CCATsimulation file.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-11 (AGM):
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

#define FUNC_NAME "ccatsim_getdims"

#define EXTENSION "CCATSIM"

int ccatsim_getdims(hid_t file_id, int *ndet, int *nsamp)
{
  herr_t h5err;
  hsize_t dims[2];  /* dimension array */

  /* use DATASET_NAME_DET_DATA to get dimensions */
  h5err = H5LTget_dataset_info(file_id, CCATSIM_DSETNAME_DETDATA, dims,
                               NULL, NULL);

  if (h5err < 0) return h5err;

  /* set output values */
  *nsamp = dims[0];
  *ndet = dims[1];

  /* all good */
  return 0;
}
