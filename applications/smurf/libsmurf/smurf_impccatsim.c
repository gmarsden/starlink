/*
*+
*  Name:
*     IMPCCATSIM

*  Purpose:
*     Import CCAT simulation HDF5 files and produce SCUBA-2 ICD-compliant files

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     ADAM A-task

*  Invocation:
*     smurf_impccatsim( int *status );

*  Arguments:
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Uses the HDF5 lite library to import raw CCAT simulator data files (see
*     https://github.com/CCATObservatory/kid-fake-data-generator) and save
*     to NDF files in a format approximating the SCUBA-2 ICD so that they
*     may subsequently be read by other SMURF routines to make maps.

*  Notes:
*     - adapted from smurf_impaztec.c and smurf_nanten2ascis.c

*  ADAM Parameters:
*     IN = _CHAR (Read)
*          Name of the input HDF5 file to be converted.  This name
*          should include the .nc extension.
*     MSG_FILTER = _CHAR (Read)
*          Control the verbosity of the application. Values can be
*          NONE (no messages), QUIET (minimal messages), NORMAL,
*          VERBOSE, DEBUG or ALL. [NORMAL]
*     OUT = _CHAR (Read)
*          Output NDF file.

*  Related Applications:
*     SMURF: MAKEMAP

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-09 (AGM):
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

#include <string.h>
#include <stdio.h>

/* STARLINK includes */
#include "ast.h"
#include "mers.h"
#include "par.h"
#include "par_par.h"
#include "prm_par.h"
#include "ndf.h"
#include "sae_par.h"
#include "star/pal.h"
#include "star/palmac.h"
#include "star/kaplibs.h"
#include "star/one.h"

/* SMURF includes */
#include "smurf_par.h"
#include "smurflib.h"
#include "libsmf/smf.h"

#include "jcmt/state.h"

#include "sc2da/sc2store_par.h"
#include "sc2da/sc2math.h"
#include "sc2da/sc2store.h"
#include "sc2da/sc2ast.h"

#include "libsc2sim/sc2sim_par.h"
#include "libsc2sim/sc2sim_struct.h"
#include "libsc2sim/sc2sim.h"

/* netCDF includes */
#if HAVE_LIBHDF5_HL
#include "libccatsim/ccatsim.h"
#endif

#define FUNC_NAME "smurf_impccatsim"
#define TASK_NAME "IMPCCATSIM"

#define MAXSTRING 256

/*
  prototypes for local hdf5 function wrappers
*/

#if HAVE_LIBHDF5_HL
/* hdf5 error handling */
static void hdf5_error(const char *msg, int *status);
#endif

void smurf_impccatsim( int *status ) {

#if HAVE_LIBHDF5_HL

  char infile[MAXSTRING];      /* input HDF5 file name */
  hid_t infile_id;             /* id of input HDF5 file */
  int infile_isopen;           /* flag stating whether infile is open */
  herr_t h5err;                /* HDF5 error return value */
  char h5msg[MAXSTRING];       /* HDF5 message string */
  char ndffile[MAXSTRING];     /* output NDF file name */
  int ndet;                    /* number of detectors */
  int nsamp;                   /* number of time samples */

  /* set defaults */
  infile_isopen = 0;

  /* Get the user defined input and output file names */
  parGet0c( "IN", infile, MAXSTRING, status);
  parGet0c( "OUT", ndffile, MAXSTRING, status);

  /* Open the HDF5 file and check for errors */
  if( *status == SAI__OK ) {

    msgOutiff(MSG__NORM," ",
              "Opening CCAT simulator file %s", status, infile);

    *status = ccatsim_openfile(infile, &infile_id);
    if (*status != SAI__OK) {
      hdf5_error("could not read file", status);
      goto CLEANUP;
    }

    infile_isopen = 1;
  }

  /* handle data */
  if( *status == SAI__OK ) {

    /* get number of detectors and samples */
    *status = ccatsim_getdims(infile_id, &ndet, &nsamp);
    if (*status != SAI__OK) {
      hdf5_error("could not get dimensions of dataset", status);
      goto CLEANUP;
    }

    msgOutiff(MSG__NORM, "", "Dataset has %d detectors, %d time samples.\n",
             status, ndet, nsamp);
  }

  msgOutif(MSG__VERB," ",
           "Writing NDF file", status);

  ndfBegin();

  /* Create HDS container file */
  //ndfPlace ( NULL, ndffile, &place, status );

  /* Close the NDF */

  sc2store_headunmap( status );

  //ndfAnnul ( &indf, status );

  ndfEnd ( status );

 CLEANUP:
  /* Free memory etc */

  /* close input file */
  if( infile_isopen ) {
    h5err = H5Fclose(infile_id);
    if (h5err < 0) {
      hdf5_error("could not close file", status);
    }
    infile_isopen = 0;
  }

  /* all done */
  if ( *status == SAI__OK ) {
    msgOutif(MSG__VERB," ",
             "impccatsim complete, NDF file written", status);
  }

#else

  *status = SAI__ERROR;
  errRep(FUNC_NAME,
         "SMURF built without libhdf5. IMPCCATSIM task not supported.",
         status);
#endif

}

#ifdef HAVE_LIBHDF5_HL

/* hand hdf5 error */
static void hdf5_error(const char *msg, int *status)
{
  msgSetc("ERROR", msg);
  *status = SAI__ERROR;
  errRep(FUNC_NAME, "HDF5 reported: ^ERROR", status);

  return;
}

#endif
