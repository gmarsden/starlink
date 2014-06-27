/*
*+
*  Name:
*     ccatsim_setstate

*  Purpose:
*     Read pointing and timing info and set JCMTState records

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_setstate(ccatsim_data *data, JCMTState *state, int *status);

*  Arguments:
*     data = ccatsim_data * (Given)
*        Pointer to data structure from which hdr info will be read
*     state = JCMTState * (Returned)
*        Pointer to array of JCMT state records. Memory should already be allocated.
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Read pointing and timing info and set JCMTState records

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-25 (AGM):
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

#define FUNC_NAME "ccatsim_setstate"

#define EXTENSION "CCATSIM"

void ccatsim_setstate(ccatsim_data *data, JCMTState *state, int *status) {

  herr_t h5error;          /* hdf5 error status */
  hsize_t dim;             /* dimension of array (all arrays used are 1-d) */

  int nframes;             /* number of frames */
  double *lst=NULL;        /* lst of each observation (hours) */
  double *bore_ra=NULL;    /* ra of telescope boresight */
  double *bore_dc=NULL;    /* dec of telescope boresight */

  double bore_az;          /* azimuth of telescope boresight */
  double bore_el;          /* elevation of telescope boresight */
  double mjd;              /* modified julian day of frame */
  double ha;               /* hour angle */
  double phi;              /* observatory latitude in rad */
  double ang;              /* focal plane rotation angle */
  double airmass;          /* calculate airmass from elevation */

  unsigned int i;          /* loop counter */

  char message[CCATSIM_MESSAGE_LEN]; /* error message */

  if (*status != SAI__OK) return;

  /* check that data has been loaded */
  if (!data->isopen) {
    ccatsim_error("ccatsim data has not been loaded", status);
    return;
  }

  /***************************
   * allocate memory         *
   ***************************/

  nframes = data->nsamp;
  lst = astMalloc(nframes*sizeof(*lst));
  bore_ra = astMalloc(nframes*sizeof(*bore_ra));
  bore_dc = astMalloc(nframes*sizeof(*bore_dc));

  /***************************
   * read data               *
   ***************************/

  /* expected dimensions for lst, boresight pointing */
  dim = (hsize_t)(nframes);

  /* check format for LST */
  ccatsim_check_dset(data, CCATSIM_LST_NAME, CCATSIM_LST_RANK,
                     &dim, CCATSIM_LST_UNIT, status);
  if (*status != SAI__OK) return;

  /* read LST */
  h5error = H5LTread_dataset(data->file_id, CCATSIM_LST_NAME,
                             H5T_NATIVE_DOUBLE, lst);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_LST_NAME);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

  /* read ra/dec boresight offsets (from tracked source) */

  /* RA */
  ccatsim_check_dset(data, CCATSIM_TELRA_NAME, CCATSIM_TELRA_RANK,
                     &dim, CCATSIM_TELRA_UNIT, status);
  if (*status != SAI__OK) return;

  h5error = H5LTread_dataset(data->file_id, CCATSIM_TELRA_NAME,
                             H5T_NATIVE_DOUBLE, bore_ra);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_TELRA_NAME);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

  /* Dec */
  ccatsim_check_dset(data, CCATSIM_TELDEC_NAME, CCATSIM_TELDEC_RANK,
                     &dim, CCATSIM_TELDEC_UNIT, status);
  if (*status != SAI__OK) return;

  h5error = H5LTread_dataset(data->file_id, CCATSIM_TELDEC_NAME,
                             H5T_NATIVE_DOUBLE, bore_dc);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_TELDEC_NAME);
    ccatsim_error(message, status);
    goto CLEANUP;
  }

  /***************************
   * loop through frames     *
   ***************************/

  /* observatory latitude */
  phi = data->telpos[1]*AST__DD2R;

  for (i=0; i<(unsigned int)(nframes); i++) {

    /* calculate mjd of sample */
    mjd = data->start_mjd + i / data->sample_rate;

    /* timing things */
    state[i].rts_num = i;
    state[i].rts_end = mjd;
    state[i].tcs_tai = mjd;

    /* nominal pointing center */
    state[i].tcs_tr_bc1 = data->srcpos[0] * AST__DD2R;
    state[i].tcs_tr_bc2 = data->srcpos[1] * AST__DD2R;

    /* bore_ra and bore_dc are offsets in degrees from target */
    bore_dc[i] = (data->srcpos[1] + bore_dc[i]) * AST__DD2R;
    bore_ra[i] = (data->srcpos[0] + bore_ra[i] / cos(bore_dc[i])) * AST__DD2R;

    /* demand and actual boresight pointing */
    state[i].tcs_tr_ac1 = bore_ra[i];
    state[i].tcs_tr_ac2 = bore_dc[i];
    state[i].tcs_tr_dc1 = bore_ra[i];
    state[i].tcs_tr_dc2 = bore_dc[i];

    /* az/el coordinate conversion */

    /* first, pointing center */
    ha = (lst[i]*15 - data->srcpos[0]) * AST__DD2R; /* in radians */
    palDe2h(ha, state[i].tcs_tr_bc2, data->telpos[1]*AST__DD2R, &bore_az, &bore_el);
    state[i].tcs_az_bc1 = bore_az;
    state[i].tcs_az_bc2 = bore_el;

    /* now telescope boresight */
    ha = lst[i] * 15.0 * AST__DD2R - bore_ra[i]; /* bore_ra already in rad */
    palDe2h(ha, bore_dc[i], phi, &bore_az, &bore_el);
    state[i].tcs_az_ac1 = bore_az;
    state[i].tcs_az_ac2 = bore_el;
    state[i].tcs_az_dc1 = bore_az;
    state[i].tcs_az_dc2 = bore_el;

    /* rotation of array */
    if (strcmp(data->fplane_rot, "radec") == 0) {
      /* focal plane is fixed w.r.t. ra/dec */
      state[i].tcs_tr_ang = 0.0;

      /* calculate rotation w.r.t. az/el */
      /* PA is angle between direction to pole from direction to zenith, so
         rotation from RA/Dec to az/el is -PA */
      ang = palPa(ha, bore_dc[i], phi);
      state[i].tcs_az_ang = -ang;
    } else {
      /* focal plane rotation type not known */
      snprintf(message, CCATSIM_MESSAGE_LEN,
               "unknown focal plane rotation type '%s'", data->fplane_rot);
      ccatsim_error(message, status);
      goto CLEANUP;
    }

    /* airmass (copied from sc2sim_calculate.c) */
    /* Calculate the airmass - note this assumes a PLANE-PARALLEL
       atmosphere with no refraction. Set constant below an
       elevation of 1 deg. */
    if( bore_el >= 1.0 * AST__DPI/180. ) {
      airmass = 1.0/sin(bore_el);
    } else {
      airmass = 3283.0;
    }
    state[i].tcs_airmass = airmass;

    /* set some other quantities */
    state[i].smu_x         = 0.0;
    state[i].smu_y         = 0.0;
    state[i].smu_z         = 0.0;
    strcpy( state[i].smu_chop_phase, " ");
    state[i].smu_jig_index = 0;
    state[i].smu_az_jig_x  = 0.0;
    state[i].smu_az_jig_y  = 0.0;
    state[i].smu_az_chop_x = 0.0;
    state[i].smu_az_chop_y = 0.0;
    state[i].smu_tr_jig_x  = 0.0;
    state[i].smu_tr_jig_y  = 0.0;
    state[i].smu_tr_chop_x = 0.0;
    state[i].smu_tr_chop_y = 0.0;

    /* No polarimeter or FTS */
    state[i].pol_ang = VAL__BADD;
    state[i].fts_pos = VAL__BADR;
  }

 CLEANUP:
  lst = astFree(lst);
  bore_ra = astFree(bore_ra);
  bore_dc = astFree(bore_dc);

  /* done */
}
