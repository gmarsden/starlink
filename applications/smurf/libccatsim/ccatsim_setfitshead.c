/*
*+
*  Name:
*     ccatsim_setfitshead

*  Purpose:
*     Create fits header based on metadata

*  Language:
*     Starlink ANSI C

*  Invocation:
*     ccatsim_setfitshead(ccatsim_data *data, AstFitsChan *fitschan,
*                         int *status);

*  Arguments:
*     data = ccatsim_data * (Given)
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

void ccatsim_setfitshead(ccatsim_data *data, AstFitsChan *fitschan,
                         int *status)
{
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

    astSetFitsI(fitschan, "NUMDET", data->ndet, "number of detectors", 0);
    astSetFitsI(fitschan, "NUMSAMP", data->nsamp, "number of samples", 0);
    astSetFitsS(fitschan, "TELESCOP", data->telname, "Name of telescope", 0);
    astSetFitsS(fitschan, "DATE-OBS", data->dateobs, "Observation Date", 0);
    astSetFitsF(fitschan, "RA", data->srcpos[0], "Right Ascension of observation [deg]", 0);
    astSetFitsF(fitschan, "DEC", data->srcpos[1], "Declination of observation [deg]", 0);

  }
}
