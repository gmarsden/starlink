/*
*+
*  Name:
*     smf_deepcopy_smfData

*  Purpose:
*     Copy all elements of a smfData structure

*  Language:
*     Starlink ANSI C

*  Type of Module:
*     Subroutine

*  Invocation:
*     new = smf_deepcopy_smfData( const smfData *old, const int rawconvert, int * status );

*  Arguments:
*     old = const smfData* (Given)
*        Pointer to smfData to be copied
*     rawconvert = const int (Given)
*        Flag to denote whether to convert integer to double
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Return Value:
*     smf_deepcopy_smfData = smfData*
*        Pointer to newly created smfData. NULL on error.

*  Description:
*     This function copies all information from an existing smfData
*     structure and all the internal structures to a new smfData
*     structure.

*  Notes:
*     - Free this memory using smf_close_file, via a smfData structure.
*     - Can be freed with a smf_free if header resources are freed first.

*  Authors:
*     Tim Jenness (TIMJ)
*     Andy Gibb (UBC)
*     {enter_new_authors_here}

*  History:
*     2006-03-23 (AGG):
*        Initial version.
*     2006-03-24 (TIMJ):
*        Deal with different data types and quality
*     2006-03-28 (AGG):
*        Change API to allow copying of raw integer data to a double
*        output array
*     {enter_further_changes_here}

*  Copyright:
*     Copyright (C) 2006 Particle Physics and Astronomy Research
*     Council. University of British Columbia. All Rights Reserved.

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
*     Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
*     MA 02111-1307, USA

*  Bugs:
*     {note_any_bugs_here}
*-
*/

/* System includes */
#include <stdlib.h>
#include <string.h>

/* Starlink includes */
#include "sae_par.h"
#include "mers.h"
#include "ndf.h"

/* SMURF routines */
#include "smf.h"
#include "smf_typ.h"
#include "smf_err.h"

#define FUNC_NAME "smf_deepcopy_smfData"

smfData *
smf_deepcopy_smfData( const smfData *old, const int rawconvert, int * status ) {

  smfDA *da = NULL;       /* New smfDA */
  dim_t dims[NDF__MXDIM]; /* Dimensions of each axis of data array */
  smf_dtype dtype;        /* Data type */
  smfFile *file = NULL;   /* New smfFile */
  smfHead *hdr = NULL;    /* New smfHead */
  size_t i;                  /* Loop counter */
  size_t j;                  /* Loop counter */
  size_t nbytes;          /* number of bytes in data type */
  dim_t ncoeff = 0;       /* Number of coefficients */
  int ndims;              /* Number of dimensions in data array */
  smfData *new = NULL;    /* New smfData */
  int npts;               /* Number of data points */
  double *outdata;            /* Pointer to output DATA */
  void *pntr[3];          /* Data, variance and quality arrays */
  double *poly = NULL;    /* Polynomial coefficients */
  int *tstream;               /* Pointer to raw time series data */
  int virtual;            /* Is it a virtual smfData? */

  if (*status != SAI__OK) return NULL;

  /* Copy elements */
  ndims = old->ndims;
  ncoeff = old->ncoeff;
  virtual = old->virtual;
  dtype = old->dtype;
  for (i=0; i<ndims; i++) {
    dims[i] = (old->dims)[i];
  }
  npts = dims[0]*dims[1];
  if (ndims == 3 ) {
    npts *= dims[2];
  }

  /* DATA and VARIANCE */
  for (i=0; i<2; i++) {
    if ( (old->pntr)[i] != NULL ) {
      /* Check if we are converting from integer to double */
      if (rawconvert && (old->dtype == SMF__INTEGER) ) {
	nbytes = sizeof(double);
	dtype = SMF__DOUBLE;
	pntr[i] = smf_malloc( npts, nbytes, 0, status);
	outdata = pntr[i];
	tstream = (old->pntr)[i];
	/* Input data are ints: must re-cast as double */
	for (j=0; j<npts; j++) {
	  outdata[j] = (double)tstream[j];
	}
	pntr[i] = outdata;
      } else {
	nbytes = smf_dtype_size(old, status);
	pntr[i] = smf_malloc( npts, nbytes, 0, status);
	memcpy( pntr[i], (old->pntr)[i], nbytes*npts);
      }
    } else {
      pntr[i] = NULL;
    }
  }
  /* Quality */
  if ( (old->pntr)[2] != NULL ) {
    pntr[2] = smf_malloc( npts, 1, 0, status);
    memcpy( pntr[2], (old->pntr)[2], npts );
  } else {
    pntr[2] = NULL;
  }

  /* Redefine npts */
  npts = dims[0]*dims[1]*ncoeff;
  poly = smf_malloc( npts, sizeof(double), 0, status);
  memcpy( poly, old->poly, npts*sizeof(double));

  /* Create the new smfHead */
  hdr = smf_deepcopy_smfHead( old->hdr, status );

  /* Construct the new smfData */
  new = smf_construct_smfData( new, file, hdr, da, dtype, pntr, dims, ndims, 
			       virtual, ncoeff, poly, status);

  return new;
}
