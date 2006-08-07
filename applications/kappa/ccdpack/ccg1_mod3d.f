      SUBROUTINE CCG1_MOD3D( NSIGMA, NITER, STACK, NPIX, NLINES,
     :                         VARS, MINPIX, RESULT, WRK1, WRK2, NCON,
     :                         POINT, USED, STATUS )
*+
*  Name:
*     CCG1_MOD3

*  Purpose:
*     Combines data lines using a mode.

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL CCG1_MOD3( NSIGMA, NITER, STACK, NPIX, NLINES, VARS, MINPIX,
*                     RESULT, WRK1, WRK2, NCON, POINT, USED, STATUS )

*  Description:
*     This routine accepts an array consisting a series of (vectorised)
*     lines of data. The data values in the lines are then combined to
*     form an output mode line. The output modal values are returned in
*     the array RESULT.

*  Arguments:
*     NSIGMA = REAL (Given)
*        The number of sigma at which to reject data values.
*     NITER = INTEGER (Given)
*        Maximum number of refining iterations.
*     STACK( NPIX, NLINES ) = DOUBLE PRECISION (Given)
*        The array of lines which are to be combined into a single line.
*     NPIX = INTEGER (Given)
*        The number of pixels in a line of data.
*     NLINES = INTEGER (Given)
*        The number of lines of data in the stack.
*     VARS( NLINES ) = DOUBLE PRECISION (Given)
*        The variance to to used for each line of data.
*     MINPIX = INTEGER (Given)
*        The minimum number of pixels required to contribute to an
*        output pixel.
*     RESULT( NPIX ) = DOUBLE PRECISION (Returned)
*        The output line of data.
*     WRK1( NLINES ) = DOUBLE PRECISION (Given and Returned)
*        Workspace for calculations.
*     WRK2( NLINES ) = DOUBLE PRECISION (Given and Returned)
*        Workspace for calculations.
*     NCON( NLINES ) = DOUBLE PRECISION (Given and Returned)
*        The actual number of contributing pixels.
*     POINT( NLINES ) = INTEGER (Given and Returned)
*        Workspace to hold pointers to the original positions of the
*        data before extraction and conversion in to the WRK1 array.
*     USED( NLINES ) = LOGICAL (Given and Returned)
*        Workspace used to indicate which values have been used in
*        estimating a resultant value.
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Authors:
*     PDRAPER: Peter Draper (STARLINK)
*     BRADC: Brad Cavanagh (JAC)
*     MJC: Malcolm J. Currie (STARLINK)
*     {enter_new_authors_here}

*  History:
*     20-MAY-1992 (PDRAPER):
*        Original version.
*     11-OCT-2004 (BRADC):
*        No longer use NUM_CMN.
*     2006 August 6 (MJC):
*        Exclude data with non-positive or bad variance.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'PRM_PAR'          ! PRIMDAT constants

*  Arguments Given:
      REAL NSIGMA
      INTEGER NITER
      INTEGER NPIX
      INTEGER NLINES
      INTEGER MINPIX
      DOUBLE PRECISION STACK( NPIX, NLINES )
      DOUBLE PRECISION VARS( NLINES )

*  Arguments Given and Returned:
      DOUBLE PRECISION NCON( NLINES )
      DOUBLE PRECISION WRK1( NLINES )
      DOUBLE PRECISION WRK2( NLINES )

*  Arguments Returned:
      DOUBLE PRECISION RESULT( NPIX )
      LOGICAL USED( NLINES )
      INTEGER POINT( NLINES )

*  Status:
      INTEGER STATUS             ! Global status

*  Global Variables:


*  External References:
      EXTERNAL NUM_WASOK
      LOGICAL NUM_WASOK          ! Was numeric operation ok?
      EXTERNAL NUM_TRAP
      INTEGER NUM_TRAP           ! Numerical error handler

*  Local constants:
      REAL PBAD                  ! Probability of bad value
      PARAMETER ( PBAD = 0.01 )
      REAL TOLL                  ! Accuracy criterion
      PARAMETER ( TOLL = 0.001 )

*  Local Variables:
      INTEGER I                  ! Loop variable
      INTEGER J                  ! Loop variable
      DOUBLE PRECISION VAL       ! Weighted median
      DOUBLE PRECISION SUM2      ! Population variance after rejection
      INTEGER NGOOD              ! Number of good pixels
      INTEGER IGOOD              ! Number of unrejected pixels

*  Internal References:
      INCLUDE 'NUM_DEC_CVT'      ! NUM_ type conversion functions
      INCLUDE 'NUM_DEF_CVT'      ! Define functions...

*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Set the numeric error and set error flag value.
      CALL NUM_HANDL( NUM_TRAP )

*  Loop over for all possible output pixels.
      DO 1 I = 1, NPIX
         NGOOD = 0
         CALL NUM_CLEARERR()

*  Loop over all possible contributing pixels.
         DO 2 J = 1, NLINES
            IF ( STACK( I, J ) .NE. VAL__BADD .AND.
     :           VARS( J ) .NE. VAL__BADD .AND.
     :           VARS( J ) .GT. VAL__SMLD ) THEN

*  Increment good value counter.
               NGOOD = NGOOD + 1

*  Update the pointers to the positions of the unextracted data.
               POINT( NGOOD ) = J

*  Convert input type floating point value.
               WRK1( NGOOD ) = NUM_DTOD( STACK( I, J ) )

*  Change variance to weight.
               WRK2( NGOOD ) = 1.0D0 / VARS( J )

*  Trap conversion failures.
               IF ( .NOT. NUM_WASOK() ) THEN

*  Decrement NGOOD, do not let it go below zero.
                  NGOOD = MAX( 0, NGOOD - 1 )
                  CALL NUM_CLEARERR()
               END IF
            END IF
 2       CONTINUE

*  Continue if more than one good value.
         IF ( NGOOD .GT. 0 ) THEN

*  Form mode.
            CALL CCG1_WMD2D( WRK1, WRK2, NGOOD, PBAD, NITER, TOLL,
     :                       NSIGMA, VAL, SUM2, USED, IGOOD, STATUS )

*  If have any values left, update counters
            IF ( IGOOD .GT. 0 ) THEN
               DO 3 J = 1, NGOOD
                  IF ( USED ( J ) ) THEN
                     NCON( POINT( J ) ) = NCON( POINT( J ) ) + 1.0D0
                  END IF
 3             CONTINUE

*  Trap occasions when all values are rejected.
            ELSE
               NGOOD = 0
            END IF
         END IF

*  If there are sufficient good pixels output the result.
         IF ( NGOOD .GE. MINPIX ) THEN
            RESULT( I ) = VAL

*  Trap numeric errors.
            IF ( .NOT. NUM_WASOK() ) THEN
               RESULT( I ) = VAL__BADD
            END IF
         ELSE

*  Not enough contributing pixels, set output invalid.
            RESULT( I ) = VAL__BADD
         END IF
 1    CONTINUE

*  Remove the numerical error handler.
      CALL NUM_REVRT

      END
* $Id$
