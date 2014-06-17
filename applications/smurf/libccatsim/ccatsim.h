/*
*+
*  Name:
*     ccatsim.h

*  Purpose:
*     Prototypes and constants for libccat functions

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Header File

*  Invocation:
*     #include "ccatsim.h"

*  Description:
*     Prototypes and constants used by the libccatsim functions.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-09 (AGM):
*        Initial version
*     2014-06-16 (AGM):
*        Introduce ccatsim_data structure, fill using ccatsim_openfile()
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2014 University of British Columbia.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 2 of
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
#ifndef CCATSIM_H_DEFINED
#define CCATSIM_H_DEFINED

#include <string.h>

/* starlink includes */
#include "ast.h"
#include "mers.h"
#include "sae_par.h"
#include "star/hds.h"
#include "ndf.h"
#include "libsmf/smf.h"
#include "smurf_par.h"

#include <hdf5.h>
#include <hdf5_hl.h>

/*************************
 * dataset properties    *
 *************************/

#define CCATSIM_DSETNAME_TELRA   "telescope_ra"
#define CCATSIM_DSETNAME_TELDEC  "telescope_dec"
#define CCATSIM_DSETNAME_DETRA   "detector_ra"
#define CCATSIM_DSETNAME_DETDEC  "detector_dec"
#define CCATSIM_DSETNAME_DETDATA "timestream_data"

#define CCATSIM_MESSAGE_LEN 256

/* data structure */
typedef struct ccatsim_data {
  int isopen;           /* flag stating if file is open */
  const char *filename; /* keep pointer to data filename */
  hid_t file_id;        /* hdf5 file pointer */
  int ndet;             /* number of detectors */
  int nsamp;            /* number of time samples */
} ccatsim_data;

/* hand hdf5 error */
void ccatsim_error(const char *msg, int *status);

/* open file and fill data structure */
void ccatsim_openfile(const char *filename, ccatsim_data *data, int *status);

/* close file pointer (and free mem if necessary) */
void ccatsim_closefile(ccatsim_data *data, int *status);



#endif /* CCATSIM_H_DEFINED */
