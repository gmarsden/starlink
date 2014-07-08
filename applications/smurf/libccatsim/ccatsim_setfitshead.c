/*
*+
*  Name:
*     ccatsim_setfitshead

*  Purpose:
*     Create fits header based on metadata

*  Language:
*     Starlink ANSI C

*  Invocation:
*     ccatsim_setfitshead(const ccatsim_data *data, AstFitsChan *fitschan,
*                         int *status);

*  Arguments:
*     data = const ccatsim_data * (Given)
*        data structure containing file info
*     fitschan = AstFitsChan * (Returned)
*        pointer to fitschan object to be filled
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Create FITS header for inclusion in NDF.

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-18 (AGM):
*        Initial Version
*     2014-06-27 (AGM):
*        Add some new headers
*     2014-07-02 (AGM):
*        Add headers required by makemap
*     2014-07-03 (AGM):
*        Make ccatsim_data* const
*     2014-07-08 (AGM):
*        Add scan_speed
*        Add more required headers
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

#define FUNC_NAME "ccatsim_setfitshead"

#define EXTENSION "CCATSIM"

void ccatsim_setfitshead(const ccatsim_data *data, AstFitsChan *fitschan,
                         int *status)
{
  char obsid[CCATSIM_ATTR_LEN]; /* obs id fits header */
  char obsidss[CCATSIM_ATTR_LEN]; /* obs id + subsystem fits header */

  /* get date info from dateobs */
  int date_yr;
  int date_mo;
  int date_da;
  int date_hr;
  int date_mn;
  double date_sc;

  /* wavelength (from bandname field) */
  double wavelen;
  char *endptr;           /* used for error checking in conversion */

  unsigned int utdate;    /* utdate yyyymmdd */

  if (*status == SAI__OK) {

    /* check that data has been loaded */
    if (!data->isopen) {
      ccatsim_error("ccatsim data has not been loaded", status);
      return;
    }

    /* check fitschan has been initialized */
    if (fitschan == NULL) {
      ccatsim_error("fitschan has not been initialized", status);
      return;
    }

    /* get date info for obsid */
    sscanf(data->dateobs, "%4d-%2d-%2dT%2d:%2d%lf", &date_yr, &date_mo, &date_da,
           &date_hr, &date_mn, &date_sc);

    /* build obsid */
    sprintf(obsid, "%s_%05d_%04d%02d%02dT%02d%02d%02d",
            data->instname, CCATSIM_OBSNUM, date_yr, date_mo, date_da,
            date_hr,date_mn,(int)(round(date_sc)));

    /* obsidss */
    snprintf(obsidss, CCATSIM_ATTR_LEN, "%s_%s", obsid, data->bandname);

    /* get wavelength */
    wavelen = strtod(data->bandname, &endptr);
    if (endptr == data->bandname) { /* warn on conversion failure */
      msgSetc("NAME", CCATSIM_BAND_NAME);
      msgOutif(MSG__NORM, "", "Attribute '^NAME' is not numeric", status);
    }

    /* utdate */
    utdate = date_yr*10000 + date_mo*100 + date_da;

    astSetFitsI(fitschan, "NUMDET", data->ndet, "number of detectors", 0);
    astSetFitsI(fitschan, "NUMSAMP", data->nsamp, "number of samples", 0);
    astSetFitsS(fitschan, "TELESCOP", data->telname, "Name of telescope", 0);
    astSetFitsS(fitschan, "INSTRUME", data->instname, "Name of instrument", 0);
    astSetFitsF(fitschan, "WAVELEN", wavelen, "Observing wavelength", 0);
    astSetFitsS(fitschan, "DATE-OBS", data->dateobs, "Observation Date", 0);
    astSetFitsS(fitschan, "OBSID", obsid, "Unique observation identifier", 0);
    astSetFitsS(fitschan, "OBSIDSS", obsidss, "Unique observation identifier (w/ subsystem)", 0);
    astSetFitsF(fitschan, "RA", data->srcpos[0], "Right Ascension of observation [deg]", 0);
    astSetFitsF(fitschan, "DEC", data->srcpos[1], "Declination of observation [deg]", 0);
    astSetFitsS(fitschan, "OBJECT", CCATSIM_OBJECT, "Target of observation", 0);
    astSetFitsI(fitschan, "OBSNUM", CCATSIM_OBSNUM, "Observation number", 0);
    astSetFitsI(fitschan, "UTDATE", utdate, "UT Date as integer in yyyymmdd format", 0);
    astSetFitsL(fitschan, "SIMULATE", CCATSIM_SIMULATE, "True if any data are simulated", 0);
    astSetFitsS(fitschan, "ARRAYID", CCATSIM_ARRAYID, "Manufacturer's serial number", 0);
    astSetFitsI(fitschan, "SEQCOUNT", CCATSIM_SEQCOUNT, "Setup Sequence Counter", 0);
    astSetFitsI(fitschan, "SHUTTER", CCATSIM_SHUTTER, "shutter position 0-Closed 1-Open", 0);
    astSetFitsS(fitschan, "SUBARRAY", CCATSIM_SUBARRAY, "", 0);
    astSetFitsS(fitschan, "SAM_MODE", CCATSIM_SAMMODE, "Sampling Mode", 0);
    astSetFitsS(fitschan, "SW_MODE", CCATSIM_SWMODE, "Switching Mode", 0);
    astSetFitsS(fitschan, "OBS_TYPE", CCATSIM_OBSTYPE, "Type of observation", 0);
    astSetFitsS(fitschan, "INBEAM", CCATSIM_INBEAM, "Hardware in the beam", 0);
    astSetFitsF(fitschan, "STEPTIME", 1.0/data->sample_rate, "Samp steptime [s]", 0);
    astSetFitsF(fitschan, "SCAN_VEL", data->scan_speed/DAS2D, "[arcsec/s] Scan velocity", 0);

    /* SEQSTART: assume that JCMTState rts_num starts at 0 and ends at nsamp-1 */
    astSetFitsI(fitschan, "SEQSTART", 0, "RTS index number of first frame", 0);
    astSetFitsI(fitschan, "SEQEND", data->nsamp-1, "RTS index number of last frame", 0);
  }
}
