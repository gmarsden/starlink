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
*     2014-06-25 (AGM):
*        Add telpos to data structure
*        Add srcpos to data structure
*     2014-06-26 (AGM):
*        Add telescope name and focal plane rotation
*        Add start_mjd and sample_rate
*        Add dateobs
*     2014-06-30 (AGM):
*        Add instrument name, band name
*     2014-07-02 (AGM):
*        Hard code some required headers
*     2014-07-03 (AGM):
*        Make ccatsim_data* const in function prototypes, where applicable
*     2014-07-08 (AGM):
*        Add scan_speed
*        Add more required headers
*        Remove fill/free_smfHead
*        Add writebolo routine and defines
*     2014-07-09 (AGM):
*        Add ccatsim_fill_smfHead back in (w/ correct usage this time)
*        Add MAP_PA header
*     2014-07-23 (AGM):
*        Pass quality pointer to ccatsim_getdata()
*     2014-09-19 (AGM):
*        Add dut1 to data structure
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
#include "star/one.h"
#include "star/pal.h"
#include "ndf.h"
#include "star/hds.h"
#include "libsmf/smf.h"
#include "smurf_par.h"

#include <hdf5.h>
#include <hdf5_hl.h>

/*************************
 * output properties     *
 *************************/

#define CCATSIM_BOLOPOS_NAME "BOLOPOS"
#define CCATSIM_BOLOPOS_TYPE "BOLOPOST"
#define CCATSIM_BOLOPOS_NDET "NDET"
#define CCATSIM_BOLOARR_X    "FPLANEX"
#define CCATSIM_BOLOARR_Y    "FPLANEY"
#define CCATSIM_BOLOARR_TYPE "_DOUBLE"

/*************************
 * dataset properties    *
 *************************/

#define CCATSIM_TELESCOPE_NAME "telescope_name"
#define CCATSIM_INSTRUMENT_NAME "instrument_name"
#define CCATSIM_BAND_NAME "band_name"
#define CCATSIM_TRACKSOURCE_NAME "track_source"
#define CCATSIM_FPLANEROT_NAME "focal_plane_rotation"
#define CCATSIM_DATEOBS_NAME "date_obs"

#define CCATSIM_TELRA_NAME   "telescope_ra"
#define CCATSIM_TELRA_RANK   1
#define CCATSIM_TELRA_UNIT   "degrees"

#define CCATSIM_TELDEC_NAME  "telescope_dec"
#define CCATSIM_TELDEC_RANK  1
#define CCATSIM_TELDEC_UNIT  "degrees"

#define CCATSIM_DETRA_NAME   "detector_x"
#define CCATSIM_DETRA_RANK   1
#define CCATSIM_DETRA_UNIT   "degrees"

#define CCATSIM_DETDEC_NAME  "detector_y"
#define CCATSIM_DETDEC_RANK  1
#define CCATSIM_DETDEC_UNIT  "degrees"

#define CCATSIM_DETDATA_NAME "timestream_data"
#define CCATSIM_DETDATA_RANK 2
#define CCATSIM_DETDATA_UNIT "Jy"

#define CCATSIM_SCANNUM_NAME "scan_number"
#define CCATSIM_SCANNUM_RANK 1

#define CCATSIM_TELLON_NAME  "telescope_longitude"
#define CCATSIM_TELLON_RANK  1
#define CCATSIM_TELLON_UNIT  "degrees"

#define CCATSIM_TELLAT_NAME  "telescope_latitude"
#define CCATSIM_TELLAT_RANK  1
#define CCATSIM_TELLAT_UNIT  "degrees"

#define CCATSIM_TELALT_NAME  "telescope_altitude"
#define CCATSIM_TELALT_RANK  1
#define CCATSIM_TELALT_UNIT  "meters"

#define CCATSIM_SRCRA_NAME   "source_ra"
#define CCATSIM_SRCRA_RANK   1
#define CCATSIM_SRCRA_UNIT   "degrees"

#define CCATSIM_SRCDEC_NAME  "source_dec"
#define CCATSIM_SRCDEC_RANK  1
#define CCATSIM_SRCDEC_UNIT  "degrees"

#define CCATSIM_LST_NAME     "local_sidereal_time"
#define CCATSIM_LST_RANK     1
#define CCATSIM_LST_UNIT     "hours"

#define CCATSIM_STARTMJD_NAME "start_mjd"
#define CCATSIM_STARTMJD_RANK 1

#define CCATSIM_SAMPRATE_NAME "sample_rate"
#define CCATSIM_SAMPRATE_RANK 1
#define CCATSIM_SAMPRATE_UNIT "Hz"

#define CCATSIM_SCANSPEED_NAME "scan_speed"
#define CCATSIM_SCANSPEED_RANK 1
#define CCATSIM_SCANSPEED_UNIT "deg/s"

/* name of units attribute */
#define CCATSIM_UNITS_NAME   "Units"

#define CCATSIM_ATTR_LEN 80
#define CCATSIM_MESSAGE_LEN 256

/* hard-coded header info */
#define CCATSIM_OBSNUM    0
#define CCATSIM_NSUBSCAN  1
#define CCATSIM_OBJECT    "simulation"
#define CCATSIM_SIMULATE  1
#define CCATSIM_ARRAYID   "0000"
#define CCATSIM_SEQCOUNT  1
#define CCATSIM_SHUTTER   1
#define CCATSIM_SUBARRAY  "s4a"
#define CCATSIM_SAMMODE   "scan"
#define CCATSIM_SWMODE    "self"
#define CCATSIM_OBSTYPE   "science"
#define CCATSIM_INBEAM    ""
#define CCATSIM_MAPPA     0.0

/* ideally, this would be set by simulator */
#define CCATSIM_DUT1      0.0

#define CCATSIM_BAD_SCANNUM -1

/* data structure */
typedef struct ccatsim_data {
  int isopen;           /* flag stating if file is open */
  const char *filename; /* keep pointer to data filename */
  hid_t file_id;        /* hdf5 file pointer */
  int ndet;             /* number of detectors */
  int nsamp;            /* number of time samples */
  double telpos[3];     /* Geodetic location of the telescope
                           lon/lat/alt in deg/deg/m */
  double srcpos[2];     /* source ra/dec in deg */
  double start_mjd;     /* MJD of first sample */
  double sample_rate;   /* sample rate in Hz */
  double scan_speed;    /* scan speed in deg/s */
  char telname[CCATSIM_ATTR_LEN]; /* telescope name */
  char instname[CCATSIM_ATTR_LEN]; /* instrument name */
  char bandname[CCATSIM_ATTR_LEN]; /* band name */
  char fplane_rot[CCATSIM_ATTR_LEN]; /* focal plane rotation type */
  char dateobs[CCATSIM_ATTR_LEN]; /* observation date/time */
  double dut1;          /* difference between UTC and UT1 */
} ccatsim_data;


/******************************************
 * public access functions                *
 ******************************************/

/* open file and fill data structure */
void ccatsim_openfile(const char *filename, ccatsim_data *data, int *status);

/* close file pointer (and free mem if necessary) */
void ccatsim_closefile(ccatsim_data *data, int *status);

/* read detector data into array */
void ccatsim_getdata(const ccatsim_data *data, double *dataptr,
                     unsigned char *quaptr, int *status);

/* set fits headers */
void ccatsim_setfitshead(const ccatsim_data *data, AstFitsChan *fitschan,
                         int *status);

/* set JCMTState array */
void ccatsim_setstate(const ccatsim_data *data, JCMTState *state, int *status);

/* write bolo locations to ndf extension */
void ccatsim_writebolo(const ccatsim_data *data, int indf, int *status);


/******************************************
 * private access functions               *
 ******************************************/

/* handle hdf5 error */
void ccatsim_error(const char *msg, int *status);

/* check dataset properties */
void ccatsim_check_dset(const ccatsim_data *data, const char *dsetname, int rank,
                        const hsize_t *dims, const char *units, int *status);

/* read a single string from data file */
void ccatsim_getstring(const ccatsim_data *data, const char *dsetname, int maxstrlen,
                       char *string, int *status);


/******************************************
 * functions to handle exported data      *
 ******************************************/

/* read bolo positions from exported ndf */
void ccatsim_fill_smfHead(smfHead *hdr, int indf, int *status);


#endif /* CCATSIM_H_DEFINED */
