      SUBROUTINE IBGND( STATUS )
*+
*  Name:
*     IBGND

*  Purpose:
*     Background modelling if image data

*  Language:
*     Starlink Fortran

*  Type of Module:
*     ASTERIX task

*  Invocation:
*     CALL IBGND( STATUS )

*  Arguments:
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Description:
*     {routine_description}

*  Usage:
*     ibgnd {parameter_usage}

*  Environment Parameters:
*     CMD = CHARACTER (read)
*        Command name. One of RESET

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

*  Implementation Status:
*     {routine_implementation_status}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     {task_references}...

*  Keywords:
*     ibgnd, usage:public

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     23 Jan 1996 V2.0-0 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants

*  Global Variables:
      INCLUDE 'IMG_CMN'

*  Status:
      INTEGER			STATUS             	! Global status

*  Local Constants:
      CHARACTER*30		VERSION
        PARAMETER		( VERSION = 'IBGND Version V2.0-0' )

*  Local Variables:
      CHARACTER*16		CMD			! Major mode
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Version id
      CALL MSG_PRNT( VERSION )

*  Initialise ASTERIX
      CALL USI_INIT()

*  Check image processing active
      IF ( .NOT. I_OPEN ) THEN
        CALL MSG_PRNT('AST_ERR: image processing system not active')
      ELSE IF ( .NOT. I_DISP ) THEN
        CALL MSG_PRNT('AST_ERR: no image currently displayed')
      ELSE

*    Make sure transformations are correct
        CALL GTR_RESTORE( STATUS )

*    Get main mode
        CMD = ' '
        DO WHILE ( (CMD.EQ.' ') .AND. (STATUS.EQ.SAI__OK) )
          CALL USI_GET0C( 'MODE', CMD, STATUS )
          CALL CHR_UCASE( CMD )
          IF ( CMD .EQ. 'HELP' ) THEN
            CALL IBGND_HELP()
            CALL USI_CANCL( 'MODE', STATUS )
            CMD = ' '
          END IF
        END DO

*    Switch on command
        IF ( STATUS .EQ. SAI__OK ) THEN

*      Reset internal variables?
          IF ( CMD .EQ. 'RESET' ) THEN
            CALL IBGND_RESET( .TRUE., .TRUE., .TRUE., STATUS )

*      Start a new model
          ELSE IF ( CMD .EQ. 'NEW' ) THEN
            CALL IBGND_NEW( STATUS )

*      Display internal state
          ELSE IF ( CMD .EQ. 'SHOW' ) THEN
            CALL IBGND_SHOW( STATUS )

          END IF

        END IF

      END IF

*  Tidy up
      CALL USI_CLOSE()

      END



      SUBROUTINE IBGND_SHOW( STATUS )
*+
*  Name:
*     IBGND_SHOW

*  Purpose:
*     Display state of background modeller

*  Language:
*     Starlink Fortran

*  Type of Module:
*     Task subroutine

*  Invocation:
*     CALL IBGND_SHOW( STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     STATUS = INTEGER (given)
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

*  Implementation Status:
*     {routine_implementation_status}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     {task_references}...

*  Keywords:
*     ibgnd, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     23 Jan 1996 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants

*  Global Variables:
      INCLUDE 'IMG_CMN'

*  Status:
      INTEGER			STATUS             	! Global status

*  Local Variables:
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Modelling started?
      CALL MSG_BLNK()
      IF ( I_BGM_ON ) THEN
      ELSE
        CALL MSG_PRNT( '  No source candidates defined' )
        CALL MSG_PRNT( '  No image sampling defined' )
        CALL MSG_PRNT( '  No surface definition defined' )
      END IF
      CALL MSG_BLNK()

      END


      SUBROUTINE IBGND_NEW( STATUS )
*+
*  Name:
*     IBGND_NEW

*  Purpose:
*     Start new background model

*  Language:
*     Starlink Fortran

*  Type of Module:
*     Task subroutine

*  Invocation:
*     CALL IBGND_NEW( STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     STATUS = INTEGER (given)
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

*  Implementation Status:
*     {routine_implementation_status}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     {task_references}...

*  Keywords:
*     ibgnd, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     23 Jan 1996 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'QUAL_PAR'

*  Global Variables:
      INCLUDE 'IMG_CMN'

*  Status:
      INTEGER			STATUS             	! Global status

*  Local Variables:
      LOGICAL			ALLOC			! Allocate bg memory?
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN
	print *,'In NEW'
	call flush(6)

*  Reset internals
      CALL IBGND_RESET( .TRUE., .TRUE., .TRUE., STATUS )
	print *,'done RESET'
	call flush(6)

*  Allocate space for the background data surface, quality (and errors?). If
*  image size has changed we need to increase existing allocations
      ALLOC = .TRUE.
      IF ( I_BGM_ON .AND. ((I_NX*I_NY) .GT. I_BGM_NELM) ) THEN
        CALL DYN_UNMAP( I_BGM_DPTR, STATUS )
        CALL DYN_UNMAP( I_BGM_QPTR, STATUS )
      ELSE
        ALLOC = .FALSE.
      END IF
      IF ( ALLOC ) THEN
        CALL DYN_MAPR( 1, I_NX*I_NY, I_BGM_DPTR, STATUS )
        CALL DYN_MAPB( 1, I_NX*I_NY, I_BGM_QPTR, STATUS )
        I_BGM_NELM = I_NX*I_NY
      END IF
	print *,'done ALLOC'
	print *,i_nx,i_ny,i_qptr,i_bgm_qptr
	call flush(6)

*  Initialise the the background model quality array. This is ok for points
*  inside the current region, and bad outside and for bad input pixels
      CALL IBGND_SETQ( I_NX, I_NY, %VAL(I_QPTR),
     :                 %VAL(I_BGM_QPTR), STATUS )
	print *,'done SETQ'
	call flush(6)

*  Set the sample mode to the whole image, simple mean and compute the samples
      CALL IBGND_SETSAMP( 'WHOLE', 'MEAN', STATUS )
	print *,'done SETSAMP'
	call flush(6)

*  Switch modelling on
      I_BGM_ON = (STATUS.EQ.SAI__OK)

      END



      SUBROUTINE IBGND_SETQ( NX, NY, QUAL, BQ, STATUS )
*+
*  Name:
*     IBGND_SETQ

*  Purpose:
*     Set quality good/bad for points inside/outside the current region

*  Language:
*     Starlink Fortran

*  Type of Module:
*     Task subroutine

*  Invocation:
*     CALL IBGND_SETQ( NX, NY, QUAL, BQ, STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     NX = INTEGER (given)
*        Number of pixels in X axis
*     NY = INTEGER (given)
*        Number of pixels in Y axis
*     QUAL[NX,NY] = BYTE (given)
*        Mapped input data quality
*     BQ[NX,NY] = BYTE (returned)
*        QUAL__GOOD inside, QUAL__MISSING outside
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

*  Implementation Status:
*     {routine_implementation_status}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     {task_references}...

*  Keywords:
*     ibgnd, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     23 Jan 1996 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'QUAL_PAR'

*  Global Variables:
      INCLUDE 'IMG_CMN'

*  Arguments Given:
      INTEGER			NX, NY
      BYTE			QUAL(NX,NY)

*  Arguments Returned:
      BYTE			BQ(NX,NY)

*  Status:
      INTEGER			STATUS             	! Global status

*  External References:
      EXTERNAL			BIT_ANDUB
        BYTE			BIT_ANDUB

      EXTERNAL			IMG_INREG
        LOGICAL			IMG_INREG

*  Local Variables:
      INTEGER			I, J			! Loop over image

      LOGICAL			GOOD			! Good input pixel?
      LOGICAL			REGEX			! Region exists?
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Region definition exists?
      REGEX = (I_REG_TYPE .NE. 'NONE')

*  If no region defined, and no data quality present
      IF ( (I_REG_TYPE .EQ. 'NONE') .AND. .NOT. I_BAD ) THEN
        CALL ARR_INIT1B( QUAL__GOOD, NX*NY, BQ, STATUS )

*  Otherwise must test each pixel
      ELSE

*    Loop over whole image
        DO J = 1, NY
          DO I = 1, NX
            IF ( I_BAD ) THEN
              GOOD = (BIT_ANDUB(QUAL(I,J),I_MASK).EQ.QUAL__GOOD)
            ELSE
              GOOD = .TRUE.
            END IF
            IF ( GOOD ) THEN
              BQ(I,J) = QUAL__GOOD
              IF ( REGEX ) THEN
                IF ( .NOT. IMG_INREG( I, J ) ) THEN
                  BQ(I,J) = QUAL__MISSING
                END IF
              END IF
            ELSE
              BQ(I,J) = QUAL__BAD
            END IF

          END DO
        END DO

      END IF

      END


      SUBROUTINE IBGND_SETSAMP( AREA, MEAN, STATUS )
*+
*  Name:
*     IBGND_SETSAMP

*  Purpose:
*     Set up sampling space and compute samples

*  Language:
*     Starlink Fortran

*  Type of Module:
*     Task subroutine

*  Invocation:
*     CALL IBGND_SETSAMP( AREA, MEAN, STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     AREA = CHARACTER*(*) (given)
*        Method of setting up samples
*     MEAN = CHARACTER*(*) (given)
*        Method of estimating means in areas
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

*  Implementation Status:
*     {routine_implementation_status}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     {task_references}...

*  Keywords:
*     ibgnd, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     23 Jan 1996 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'QUAL_PAR'

*  Global Variables:
      INCLUDE 'IMG_CMN'

*  Arguments Given:
      CHARACTER*(*)		AREA, MEAN

*  Status:
      INTEGER			STATUS             	! Global status

*  Local Variables:
      INTEGER			I			! Sample data
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Reset existing sampling
      CALL IBGND_RESET( .FALSE., .TRUE., .FALSE., STATUS )

*  Store attributes
      I_BGM_AREA = AREA
      I_BGM_MEAN = MEAN

*  Decide how many samples
      IF ( AREA .EQ. 'WHOLE' ) THEN
        I_BGM_NSAMP = 1
      ELSE IF ( AREA .EQ. 'ANNULUS' ) THEN
      ELSE IF ( AREA .EQ. 'BOX' ) THEN
      END IF

*  Allocate space for information needed per sample
      DO I = 1, I__NSAMATT
        CALL DYN_MAPR( 1, I_BGM_NSAMP, I_BGM_SAMPTR(I), STATUS )
      END DO
      CALL ARR_INIT1R( 0.0, I_BGM_NSAMP, %VAL(I_BGM_SAMPTR(1)), STATUS )
      CALL ARR_INIT1R( 0.0, I_BGM_NSAMP, %VAL(I_BGM_SAMPTR(2)), STATUS )
      CALL ARR_INIT1I( 0, I_BGM_NSAMP, %VAL(I_BGM_SAMPTR(3)), STATUS )

*  Compute the samples
      CALL IBGND_SAMP_COMP( AREA, MEAN, I_NX, I_NY, %VAL(I_DPTR),
     :                      %VAL(I_BGM_QPTR),
     :                      I_BGM_NSAMP, %VAL(I_BGM_SAMPTR(1)),
     :                      %VAL(I_BGM_SAMPTR(2)),
     :                      %VAL(I_BGM_SAMPTR(3)),
     :                      STATUS )

      END


      SUBROUTINE IBGND_SAMP_COMP( AREA, MEANM, NX, NY, DATA, BQ,
     :                      NSAMP, SAMM, SAMEM, SAMNP, STATUS )
*+
*  Name:
*     IBGND_SAMP_COMP

*  Purpose:
*     Compute values of samples

*  Language:
*     Starlink Fortran

*  Type of Module:
*     Task subroutine

*  Invocation:
*     CALL IBGND_SETSAMP( AREA, MEANM, NX, NY, DATA, BQ, STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     AREA = CHARACTER*(*) (given)
*        Method of setting up samples
*     MEAN = CHARACTER*(*) (given)
*        Method of estimating means in areas
*     NX = INTEGER (given)
*        Number of pixels in X axis
*     NY = INTEGER (given)
*        Number of pixels in Y axis
*     DATA[NX,NY] = REAL (given)
*        The image data
*     BQ[NX,NY] = BYTE (given)
*        QUAL__GOOD inside, QUAL__MISSING outside
*     NSAMP = INTEGER (given)
*        Number of samples
*     SAMM[NSAMP] = REAL (returned)
*        Sample means
*     SAMEM[NSAMP] = REAL (returned)
*        Sample mean errors
*     SAMNP[NSAMP] = INTEGER (returned)
*        Sample pixel counts
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

*  Implementation Status:
*     {routine_implementation_status}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     {task_references}...

*  Keywords:
*     ibgnd, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     23 Jan 1996 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'QUAL_PAR'

*  Global Variables:
      INCLUDE 'IMG_CMN'

*  Arguments Given:
      CHARACTER*(*)		AREA, MEANM
      INTEGER			NX, NY, NSAMP
      REAL			DATA(NX,NY)
      BYTE			BQ(NX,NY)

*  Arguments Given and Returned:
      REAL			SAMM(*), SAMEM(*)
      INTEGER			SAMNP(*)

*  Status:
      INTEGER			STATUS             	! Global status

*  Local Variables:
      REAL			AMEAN			! Assumed mean
      REAL			MEAN, SUM, STDDEV	!
      REAL			D, DMEAN, WTSUM, WTSUM2	!

      INTEGER			I, J			! Loop over image
      INTEGER			ISAMP			! Loop over samples
      INTEGER			N			! Pixel count
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Decide how many samples
      ISAMP = 1
      IF ( AREA .EQ. 'WHOLE' ) THEN

*    Assumed mean from existing sample value
        AMEAN = SAMM(ISAMP)
        SUM = 0.0
        STDDEV = 0.0
        DO J = 1, NY
          DO I = 1, NX
            IF ( BQ(I,J) .EQ. QUAL__GOOD ) THEN
              SUM = SUM + DATA(I,J)
              STDDEV = STDDEV + (DATA(I,J)-AMEAN)**2
              N = N + 1
            END IF
          END DO
        END DO
        WTSUM = REAL(N)
        WTSUM2 = WTSUM

*      Find values from sums
        MEAN = MEAN / WTSUM
        D = WTSUM - WTSUM2/WTSUM

        SAMNP(ISAMP) = N

*    Correct standard deviation for assumed mean
        DMEAN = AMEAN - MEAN
        IF ( N .GT. 1 ) THEN
          STDDEV = STDDEV - WTSUM*DMEAN**2
          STDDEV = SQRT(STDDEV/D)
        ELSE
          STDDEV = -1.0
        END IF
        SAMM(ISAMP) = DMEAN
        SAMEM(ISAMP) = STDDEV / SQRT(REAL(N))

*    Simply report mean
        CALL MSG_SETR( 'MEAN', SAMM(1) )
        CALL MSG_SETR( 'EMEAN', SAMEM(2) )
        CALL MSG_PRNT( '  Mean value in image is ^MEAN +- ^EMEAN' )

      ELSE IF ( AREA .EQ. 'ANNULUS' ) THEN
      ELSE IF ( AREA .EQ. 'BOX' ) THEN
      END IF

      END


      SUBROUTINE IBGND_RESET( SRC, SAMP, FIT, STATUS )
*+
*  Name:
*     IBGND_RESET

*  Purpose:
*     Reset background modeller internal state

*  Language:
*     Starlink Fortran

*  Type of Module:
*     Task subroutine

*  Invocation:
*     CALL IBGND_RESET( SRC, SAMP, FIT, STATUS )

*  Description:
*     {routine_description}

*  Arguments:
*     SRC = LOGICAL (given)
*        Reset source database info?
*     SAMP = LOGICAL (given)
*        Reset sampling info?
*     FIT = LOGICAL (given)
*        Reset surface fitting info?
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

*  Implementation Status:
*     {routine_implementation_status}

*  External Routines Used:
*     {name_of_facility_or_package}:
*        {routine_used}...

*  Implementation Deficiencies:
*     {routine_deficiencies}...

*  References:
*     {task_references}...

*  Keywords:
*     ibgnd, usage:private

*  Copyright:
*     Copyright (C) University of Birmingham, 1995

*  Authors:
*     DJA: David J. Allan (Jet-X, University of Birmingham)
*     {enter_new_authors_here}

*  History:
*     23 Jan 1996 (DJA):
*        Original version.
*     {enter_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants

*  Global Variables:
      INCLUDE 'IMG_CMN'

*  Arguments Given:
      LOGICAL			SRC, SAMP, FIT

*  Status:
      INTEGER			STATUS             	! Global status

*  Local Variables:
      INTEGER			I			! Loop over src arrays
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Destroy source database?
      IF ( SRC ) THEN
        I_BGM_NSRC = 0
        DO I = 1, I__NSRCATT
          IF ( I_BGM_ON .AND. (I_BGM_SRCPTR(I) .NE. 0) ) THEN
            CALL DYN_UNMAP( I_BGM_SRCPTR(I), STATUS )
          END IF
          I_BGM_SRCPTR(I) = 0
        END DO
      END IF

*  Destroy sampling data if defined
      IF ( SAMP ) THEN
        I_BGM_AREA = 'NONE'
        I_BGM_NSAMP = 0
        DO I = 1, I__NSAMATT
          IF ( I_BGM_ON .AND. (I_BGM_SAMPTR(I) .NE. 0) ) THEN
            CALL DYN_UNMAP( I_BGM_SAMPTR(I), STATUS )
          END IF
          I_BGM_SAMPTR(I) = 0
        END DO
      END IF

      END


*+
      SUBROUTINE IBGND_HELP()
*    Description :
*    Deficiencies :
*    Bugs :
*    Authors :
*     BHVAD::RJV
*    History :
*    Type definitions :
      IMPLICIT NONE
*    Global constants :
*    Global variables :
*    Import :
*    Export :
*    Status :
*    Function declarations :
*    Local constants :
      INTEGER MLINE
      PARAMETER (MLINE=7)
      INTEGER PLINE
      PARAMETER (PLINE=7)
*    Local variables :
      CHARACTER*79 MTEXT(MLINE)
     :/' CIRcle  - circular region     BOX     - box parallel to axes',
     : ' POLygon - irregular polygon   SLIce   - rectangular slice',
     : ' ANNulus - annular region      ELLipse - elliptical region',
     : ' CONtour - within contour      XSPokes - ROSAT XRT spokes',
     : ' WHOle   - whole image         INVert  - invert region',
     : ' SHOw    - outline all regions LISt    - list ARD text',
     : ' IMPort  - input ARD           EXPort  - output ARD'/
      CHARACTER*79 PTEXT(PLINE)
     :/' ADD     - add a new region to previous definition',
     : ' AND     - select only overlap of new region with existing one',
     : ' NOT     - select pixels outside the specified region',
     : ' EXC     -   "      "       "     "      "       "',
     : ' ADDNOT  - add pixels outside new region to existing region',
     : ' ANDNOT  - select overlap of pixels outside new region',
     : '           with existing region'/
      INTEGER ILINE
*-

      CALL MSG_BLNK()
      CALL MSG_PRNT('Available modes are:')
      CALL MSG_BLNK()
      DO ILINE=1,MLINE
        CALL MSG_PRNT(MTEXT(ILINE))
      ENDDO

      CALL MSG_BLNK()
      CALL MSG_PRNT('With the following prefixes:')
      CALL MSG_BLNK()
      DO ILINE=1,PLINE
        CALL MSG_PRNT(PTEXT(ILINE))
      ENDDO

      CALL MSG_BLNK()
      CALL MSG_PRNT('*** WARNING - ellipse doesn''t work yet!!')
      CALL MSG_BLNK()

      END
