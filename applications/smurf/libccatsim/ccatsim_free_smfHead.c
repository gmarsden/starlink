/*
*+
*  Name:
*     ccatsim_free_smfHead

*  Purpose:
*     Free memory in smfHead initialized by ccatsim_fill_smfHead

*  Language:
*     ANSI C

*  Invocation:
*     void ccatsim_free_smfHead(smfHead *hdr, int *status);

*  Arguments:
*     hdr = smfHead* (Given/Returned)
*        Pointer to the smfHead
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Free all memory in smfHead initialized by ccatsim_fill_smfHead

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-17 (AGM):
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

#define FUNC_NAME "ccatsim_free_smfHead"

#define EXTENSION "CCATSIM"

void ccatsim_free_smfHead(smfHead *hdr, int *status) {

  astFree(hdr->fplanex);
  astFree(hdr->fplaney);

}
