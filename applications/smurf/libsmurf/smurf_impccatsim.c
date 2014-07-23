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
*     2014-06-16 (AGM):
*        Update for ccatsim_data structure
*     2014-06-27 (AGM):
*        Add JCMTState headers
*     2014-07-08 (AGM):
*        Remove calls to smfHead (not needed)
*        Add call to ccatsim_writebolo (writes bolo positions to ndf extension)
*     2014-07-23 (AGM):
*        Set QUALITY (based on value of -1 in scan_number)
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

void smurf_impccatsim( int *status ) {

#if HAVE_LIBHDF5_HL

  char infile[MAXSTRING];      /* input HDF5 file name */
  ccatsim_data ccatdata;       /* structure containing info about input data file */

  struct JCMTState *head=NULL; /* header data for each frame  */

  char ndffile[MAXSTRING];     /* output NDF file name */
  int place=0;                 /* NDF placeholder */
  int indf=0;                  /* NDF id for the file */
  void *pntr=NULL;             /* Temporary pointer */
  int nmap=0;                  /* Number of elements mapped */

  int lbnd[3];                 /* Dimensions of the DATA component */
  int ubnd[3];                 /* Dimensions of the DATA component */
  int ncol;                    /* number of bolometers in column  */
  int nrow=1;                  /* number of bolometers in row */
  int nframes;                 /* number of time steps */

  double *full_bolosig=NULL;   /* all bolo signals [ndet x nsamp] */
  unsigned char *full_boloqua=NULL; /* quality flag for bolo signals [ndet x nsamp] */
  AstFitsChan *fitschan=NULL;  /* FITS headers */

  /* set defaults */
  memset(&ccatdata, 0, sizeof(ccatdata));

  /* Get the user defined input and output file names */
  parGet0c( "IN", infile, MAXSTRING, status);
  parGet0c( "OUT", ndffile, MAXSTRING, status);

  /* Open the HDF5 file and check for errors */
  if( *status == SAI__OK ) {

    msgOutiff(MSG__NORM," ",
              "Opening CCAT simulator file %s", status, infile);

    ccatsim_openfile(infile, &ccatdata, status);
    if (*status != SAI__OK) goto CLEANUP;

    ncol = ccatdata.ndet;
    nframes = ccatdata.nsamp;

    msgOutiff(MSG__VERB, "", "Simulation tracks source at "
              "RA = %.1lf deg, Dec = %.1lf deg\n",
              status, ccatdata.srcpos[0], ccatdata.srcpos[1]);

    msgOutiff(MSG__VERB, "", "Data file has %d detectors and %d time samples",
              status, ccatdata.ndet, ccatdata.nsamp);
  }

  /* handle data */
  if( *status == SAI__OK ) {

    /* header for each frame */
    head = astCalloc(nframes, sizeof(*head));

    /* set state */
    ccatsim_setstate(&ccatdata, head, status);
    if (*status != SAI__OK) goto CLEANUP;

  }

  msgOutif(MSG__VERB," ", "Writing NDF file", status);

  ndfBegin();

  /* Create HDS container file */
  ndfPlace ( NULL, ndffile, &place, status );

  /* Create an NDF inside the container */
  lbnd[SC2STORE__COL_INDEX] = SC2STORE__BOL_LBND;
  ubnd[SC2STORE__COL_INDEX] = lbnd[SC2STORE__COL_INDEX] + ncol - 1;
  lbnd[SC2STORE__ROW_INDEX] = SC2STORE__BOL_LBND;
  ubnd[SC2STORE__ROW_INDEX] = lbnd[SC2STORE__ROW_INDEX] + nrow - 1;
  lbnd[2] = 1;
  ubnd[2] = nframes;

  ndfNew ( "_DOUBLE", 3, lbnd, ubnd, &place, &indf, status );
  ndfHcre ( indf, status );

  /* Map the data array */
  ndfMap( indf, "DATA", "_DOUBLE", "WRITE", &pntr, &nmap, status );
  full_bolosig = pntr;

  ndfMap( indf, "QUALITY", "_UBYTE", "WRITE", &pntr, &nmap, status );
  full_boloqua = pntr;

  /* read data into full_bolosig. */
  /* data are to be stored with detector index varying most quickly. */
  ccatsim_getdata(&ccatdata, full_bolosig, full_boloqua, status);
  if (*status != SAI__OK) goto CLEANUP;

  /* fits headers */
  fitschan = astFitsChan(NULL, NULL, " ");
  ccatsim_setfitshead(&ccatdata, fitschan, status);
  kpgPtfts(indf, fitschan, status); /* add as extension to ndf */

  /* Create storage for Header values for each frame - store in JCMTSTATE */
  sc2store_writejcmtstate(indf, nframes, head, status);

  /* Write bolo positions */
  ccatsim_writebolo(&ccatdata, indf, status);

  /* Close the NDF */
  sc2store_headunmap( status );
  ndfAnnul ( &indf, status );
  ndfEnd ( status );

 CLEANUP:
  /* Free memory etc */
  if (fitschan) fitschan = astAnnul( fitschan );
  head = astFree( head );

  /* close input file */
  if(ccatdata.isopen) {
    ccatsim_closefile(&ccatdata, status);
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

