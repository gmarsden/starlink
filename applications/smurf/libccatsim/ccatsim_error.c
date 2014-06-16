/*
*+
*  Name:
*     ccatsim_error

*  Purpose:
*     Report error in ccatsim library

*  Language:
*     Starlink ANSI C

*  Invocation:
*     ccatsim_error(const char *message, int *status);

*  Arguments:
*     message = const char * (Given)
*        message to report to user
*     data = ccatsim_data * (Returned)
*        data structure containing file info
*     status = int* (Given and Returned)
*        Pointer to global status.

*  Description:
*     Report error and set status

*  Authors:
*     AGM: Gaelen Marsden (UBC)
*     {enter_new_authors_here}

*  History:
*     2014-06-16 (AGM):
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

#include "ccatsim.h"

#define FUNC_NAME "ccatsim_error"

#define EXTENSION "CCATSIM"

void ccatsim_error(const char *message, int * status)
{
  msgSetc("ERROR", message);
  *status = SAI__ERROR;
  errRep(FUNC_NAME, "ccatsim reported: ^ERROR", status);

  return;
}
