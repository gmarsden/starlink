/*
*+
*  Name:
*     ccatsim_openfile

*  Purpose:
*     Open HDF5 for reading

*  Language:
*     Starlink ANSI C

*  Invocation:
*     int error = ccatsim_openfile(const char *filename, hid_t *file_id);

*  Arguments:
*     filename = const char * (Given)
*        filename of HDF5 file to open
*     file_id = hid_t * (Returned)
*        pointer to file object of opened file

*  Returned:
*     error (int)
*        Non-zero on error.

*  Description:
*     Open an HDF5 for reading.

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

#define FUNC_NAME "ccatsim_openfile"

#define EXTENSION "CCATSIM"

int ccatsim_openfile(const char *filename, hid_t *file_id)
{
  herr_t h5err;

  /* Turn off automatic HDF5 error reporting */
  h5err = H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  if (h5err < 0) return h5err;

  /* open file */
  *file_id = H5Fopen(filename, H5F_ACC_RDONLY, H5P_DEFAULT);
  if (*file_id < 0) return *file_id;

  /* all good */
  return 0;
}
