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
*     2014-06-25 (AGM):
*        Read telpos into data structure
*        Read source ra/dec into data structure
*     2014-06-26 (AGM):
*        Read telescope name and focal plane rotation
*        Read start_mjd and sample_rate
*        Read dateobs
*     2014-06-30 (AGM):
*        Read instrument name and band name
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
  herr_t h5error;         /* hdf5 error code */
  hid_t file_id;          /* hdf5 file pointer */
  char message[CCATSIM_MESSAGE_LEN]; /* error message */
  hsize_t dims[2];        /* data dimensions */
  int track_source;       /* retrieve track_source value from file */

  /* set filename */
  data->filename = filename;

  /* Turn off automatic HDF5 error reporting */
  h5error = H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
  if (h5error < 0) {
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

  /* mark file opened */
  data->isopen = 1;

  /* check that simulation tracks source -- don't know how to deal with it
     otherwise */
  h5error = H5LTread_dataset_int(file_id, CCATSIM_TRACKSOURCE_NAME, &track_source);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN, "could not read dataset '%s'",
             CCATSIM_TRACKSOURCE_NAME);
    ccatsim_error(message, status);
    return;
  }
  if (!track_source) {
    ccatsim_error("Simulation does not track source. Exiting.", status);
    return;
  }

  /* check that band name is given and that there is only one band_name */
  dims[0] = 1;
  ccatsim_check_dset(data, CCATSIM_BAND_NAME, 1, dims, NULL, status);
  if (*status != SAI__OK) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "band name not set, or more than one bands in dataset");
    ccatsim_error(message, status);
  }

  /* now get band name */
  ccatsim_getstring(data, CCATSIM_BAND_NAME, CCATSIM_ATTR_LEN,
                    data->bandname, status);
  if (*status != SAI__OK) return;


  /* get dimensions using DATASET_NAME_DET_DATA */
  h5error = H5LTget_dataset_info(file_id, CCATSIM_DETDATA_NAME, dims,
                                 NULL, NULL);
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN, "%s",
             "could not retrieve data dimensions");
    ccatsim_error(message, status);
    return;
  }
  data->nsamp = dims[0];
  data->ndet = dims[1];

  /*****************************
   * get telescope position    *
   *****************************/

  /* all fields have single element */
  dims[0] = 1;

  /* check longitude */
  ccatsim_check_dset(data, CCATSIM_TELLON_NAME, CCATSIM_TELLON_RANK,
                     dims, CCATSIM_TELLON_UNIT, status);
  if (*status != SAI__OK) return;

  /* get longitude */
  h5error = H5LTread_dataset(data->file_id, CCATSIM_TELLON_NAME,
                             H5T_NATIVE_DOUBLE, &(data->telpos[0]));
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_TELLON_NAME);
    ccatsim_error(message, status);
    return;
  }

  /* check latitude */
  ccatsim_check_dset(data, CCATSIM_TELLAT_NAME, CCATSIM_TELLAT_RANK,
                     dims, CCATSIM_TELLAT_UNIT, status);
  if (*status != SAI__OK) return;

  /* get latitude */
  h5error = H5LTread_dataset(data->file_id, CCATSIM_TELLAT_NAME,
                             H5T_NATIVE_DOUBLE, &(data->telpos[1]));
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_TELLAT_NAME);
    ccatsim_error(message, status);
    return;
  }

  /* check altitude */
  ccatsim_check_dset(data, CCATSIM_TELALT_NAME, CCATSIM_TELALT_RANK,
                     dims, CCATSIM_TELALT_UNIT, status);
  if (*status != SAI__OK) return;

  /* get altitude */
  h5error = H5LTread_dataset(data->file_id, CCATSIM_TELALT_NAME,
                             H5T_NATIVE_DOUBLE, &(data->telpos[2]));
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_TELALT_NAME);
    ccatsim_error(message, status);
    return;
  }

  /*****************************
   * get source position       *
   *****************************/

  /* all fields have single element */
  dims[0] = 1;

  /* check source ra */
  ccatsim_check_dset(data, CCATSIM_SRCRA_NAME, CCATSIM_SRCRA_RANK,
                     dims, CCATSIM_SRCRA_UNIT, status);
  if (*status != SAI__OK) return;

  /* get source ra */
  h5error = H5LTread_dataset(data->file_id, CCATSIM_SRCRA_NAME,
                             H5T_NATIVE_DOUBLE, &(data->srcpos[0]));
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_SRCRA_NAME);
    ccatsim_error(message, status);
    return;
  }

  /* check source dec */
  ccatsim_check_dset(data, CCATSIM_SRCDEC_NAME, CCATSIM_SRCDEC_RANK,
                     dims, CCATSIM_SRCDEC_UNIT, status);
  if (*status != SAI__OK) return;

  /* get source dec */
  h5error = H5LTread_dataset(data->file_id, CCATSIM_SRCDEC_NAME,
                             H5T_NATIVE_DOUBLE, &(data->srcpos[1]));
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_SRCDEC_NAME);
    ccatsim_error(message, status);
    return;
  }

  /*****************************
   * timing info               *
   *****************************/

  /* all fields have single element */
  dims[0] = 1;

  /* start_mjd */
  ccatsim_check_dset(data, CCATSIM_STARTMJD_NAME, CCATSIM_STARTMJD_RANK,
                     dims, NULL, status);
  if (*status != SAI__OK) return;
  h5error = H5LTread_dataset(data->file_id, CCATSIM_STARTMJD_NAME,
                             H5T_NATIVE_DOUBLE, &(data->start_mjd));
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_STARTMJD_NAME);
    ccatsim_error(message, status);
    return;
  }

  /* sample rate */
  ccatsim_check_dset(data, CCATSIM_SAMPRATE_NAME, CCATSIM_SAMPRATE_RANK,
                     dims, CCATSIM_SAMPRATE_UNIT, status);
  if (*status != SAI__OK) return;
  h5error = H5LTread_dataset(data->file_id, CCATSIM_SAMPRATE_NAME,
                             H5T_NATIVE_DOUBLE, &(data->sample_rate));
  if (h5error < 0) {
    snprintf(message, CCATSIM_MESSAGE_LEN,
             "could not read dataset '%s'", CCATSIM_SAMPRATE_NAME);
    ccatsim_error(message, status);
    return;
  }

  /*****************************
   * general info              *
   *****************************/

  /* get telescope name */
  ccatsim_getstring(data, CCATSIM_TELESCOPE_NAME, CCATSIM_ATTR_LEN,
                    data->telname, status);
  if (*status != SAI__OK) return;

  /* get instrument name */
  ccatsim_getstring(data, CCATSIM_INSTRUMENT_NAME, CCATSIM_ATTR_LEN,
                    data->instname, status);
  if (*status != SAI__OK) return;

  /* get focal plane rotation type */
  ccatsim_getstring(data, CCATSIM_FPLANEROT_NAME, CCATSIM_ATTR_LEN,
                    data->fplane_rot, status);
  if (*status != SAI__OK) return;

  /* get observation date */
  ccatsim_getstring(data, CCATSIM_DATEOBS_NAME, CCATSIM_ATTR_LEN,
                    data->dateobs, status);
  if (*status != SAI__OK) return;

  /* success */
}
