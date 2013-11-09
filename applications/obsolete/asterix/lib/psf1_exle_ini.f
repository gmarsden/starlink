      SUBROUTINE PSF1_EXLE_INI( PSID, STATUS )
*+
*  Name:
*     PSF1_EXLE_INI

*  Purpose:
*     Initialise an EXOSAT LE psf

*  Language:
*     Starlink Fortran

*  Invocation:
*     CALL PSF1_EXLE_INI( PSID, STATUS )

*  Description:
*

*  Arguments:
*     PSID = INTEGER (given)
*        ADI identifier of psf storage object. The data held by the object
*        is altered
*     STATUS = INTEGER (given and returned)
*        The global status.

*  Examples:
*     {routine_example_text}
*        {routine_example_description}

*  Pitfalls:
*     {pitfall_description}...

*  Notes:
*     {routine_notes}...

*  Prior Requirements:
*     {routine_prior_requirements}...

*  Side Effects:
*     {routine_side_effects}...

*  Algorithm:
*     {algorithm_description}...

*  Accuracy:
*     {routine_accuracy}

*  Timing:
*     {routine_timing}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     PSF Subroutine Guide : http://www.sr.bham.ac.uk/asterix-docs/Programmer/Guides/psf.html

*  Keywords:
*     package:psf, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1996

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     2 May 1996 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants

*  Arguments Given:
      INTEGER                   PSID

*  Status:
      INTEGER 			STATUS             	! Global status

*  External References:
      EXTERNAL			PSF1_EXLE_DAT
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Set the data routine
      CALL PSF0_SETRTN( PSID, 'Data', PSF1_EXLE_DAT, STATUS )

*  Report any errors
      IF ( STATUS .NE. SAI__OK ) THEN
        CALL AST_REXIT( 'PSF1_EXLE_INI', STATUS )
      END IF

      END
