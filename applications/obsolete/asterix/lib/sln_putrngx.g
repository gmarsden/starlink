      SUBROUTINE SLN_PUTRNG<T>( SID, QNAM, NRNG, START, STOP, STATUS )
*+
*  Name:
*     SLN_PUTRNG<T>

*  Purpose:
*     Write selector consisting of range pairs

*  Language:
*     Starlink Fortran

*  Invocation:
*     CALL SLN_PUTRNG<T>( SID, QNAM, NRNG, START, STOP, STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     SID = INTEGER (given)
*        ADI identifier of selection record
*     QNAM = CHARACTER*(*) (given)
*        Name of selection quantity
*     NRNG = INTEGER (given)
*        Number of ranges
*     START[] = <TYPE> (given)
*        Array of range lower bounds
*     STOP[] = <TYPE> (given)
*        Array of range upper bounds
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
*     SLN Subroutine Guide : http://www.sr.bham.ac.uk/asterix-docs/Programmer/Guides/sln.html

*  Keywords:
*     package:sln, usage:public

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     4 Sep 1995 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'AST_PKG'

*  Arguments Given:
      INTEGER			SID, NRNG
      CHARACTER*(*)		QNAM
      <TYPE>			START(*), STOP(*)

*  Status:
      INTEGER 			STATUS             	! Global status

*  External References:
      LOGICAL			AST_QPKGI
        EXTERNAL		AST_QPKGI

*  Local Variables:
      INTEGER			QID			! Quantity structure
      INTEGER			SSID			! Selectors structure
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Check initialised
      IF ( .NOT. AST_QPKGI( SLN__PKG ) ) CALL SLN0_INIT( STATUS )

*  Locate Selectors structure
      CALL ADI_FIND( SID, 'Selectors', SSID, STATUS )

*  Create new structure, and locate it
      CALL ADI_CNEW0( SSID, QNAM, 'STRUC', STATUS )
      CALL ADI_FIND( SSID, QNAM, QID, STATUS )

*  Write variant
      CALL ADI_CNEWV0C( QID, 'Variant', 'RANGE_PAIRS', STATUS )

*  Write ranges
      CALL ADI_CPUT1<T>( QID, 'START', NRNG, START, STATUS )
      CALL ADI_CPUT1<T>( QID, 'STOP', NRNG, STOP, STATUS )

*  Release identifiers
      CALL ADI_ERASE( QID, STATUS )
      CALL ADI_ERASE( SSID, STATUS )

*  Report any errors
      IF ( STATUS .NE. SAI__OK ) CALL AST_REXIT( 'SLN_PUTRNG<T>', STATUS )

      END
