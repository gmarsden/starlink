      SUBROUTINE POL1_CHKEX( INDF, LOC, IGRP, STATUS )
*+
*  Name:
*     POL1_CHKEX

*  Purpose:
*     Check the values in the POLPACK extension and WCS component of
*     an NDF.

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL POL1_CHKEX( INDF, LOC, IGRP, STATUS )

*  Description:
*     The routine does the following:
*     1) Checks that the POLPACK extension contains a WPLATE component
*     with a legal value. An error is reported if not.
*     2) Sets up a default IMGID component in the POLPACK extension if
*     the extension does not currently contain an IMGID value, or if the
*     existiung value is blank. The default IMGID value used is the
*     basename of the NDF.
*     3) Checks that the IMGID value is unique amongst the NDFs being
*     processed. If not, a warning (not an error) is given.
*     4) Sets up a default ANGROT component in the POLPACK extension if
*     the extension does not currently contain an ANGROT value. The default 
*     value used is zero.
*     5) Appends the WPLATE value to the FILTER value. If there is no
*     FILTER value then one is created equal to WPLATE.
*     6) Copies the FILTER value into the CCDPACK extension (an extension
*     is created if necessary).
*     7) Adds a Frame into the NDF's WCS component representing a 2D
*     cartesian coordinate system with origin at pixel coordinates (0,0), 
*     with its first axis parallel to the analyser for WPLATE = 0.0
*     degrees.
*     
*  Arguments:
*     INDF = INTEGER (Given)
*        Identifier for the NDF.
*     LOC = CHARACTER * ( * ) (Given)
*        Locator to POLPACK extension of NDF.
*     IGRP = INTEGER (Given and Returned)
*        Identifier for a GRP group holding the user IMGID values. If
*        this is supplied equal to GRP__NOID, then a new group is created
*        and its identifier is returned.
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Authors:
*     DSB: David S. Berry (STARLINK)
*     {enter_new_authors_here}

*  History:
*     5-DEC-1997 (DSB):
*        Original version.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-
      
*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'GRP_PAR'          ! GRP constants
      INCLUDE 'DAT_PAR'          ! DAT constants
      INCLUDE 'NDF_PAR'          ! NDF constants
      INCLUDE 'AST_PAR'          ! AST constants

*  Arguments Given:
      INTEGER INDF
      CHARACTER * ( * ) LOC

*  Arguments Given and Returned:
      INTEGER IGRP

*  Status:
      INTEGER STATUS             ! Global status

*  External References:
      INTEGER CHR_LEN

*  Local Constants:
      DOUBLE PRECISION DTOR
      PARAMETER ( DTOR = 0.01745329251994329577 )

*  Local Variables:
      CHARACTER CCDLOC*(DAT__SZLOC) 
      CHARACTER FILTER*256
      CHARACTER IMGID*256
      CHARACTER NDFNAM*256
      CHARACTER WPLATE*10
      DOUBLE PRECISION COS  
      DOUBLE PRECISION MAT( NDF__MXDIM*NDF__MXDIM )
      DOUBLE PRECISION SIN
      INTEGER DIM( NDF__MXDIM )
      INTEGER FRAME                  
      INTEGER I
      INTEGER IAT
      INTEGER ICURR
      INTEGER INDX
      INTEGER IPIXEL
      INTEGER IWCS
      INTEGER LC
      INTEGER MAP
      INTEGER NDIM
      LOGICAL THERE
*.

*  Check inherited global status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*  Report an error if the WPLATE extension item does not exist.
      CALL DAT_THERE( LOC, 'WPLATE', THERE, STATUS )
      IF( .NOT. THERE .AND. STATUS .EQ. SAI__OK ) THEN
         STATUS = SAI__ERROR
         CALL ERR_REP( 'POLIMP_NOWPL', 'Mandatory extension item '//
     :                 'WPLATE has no value.', STATUS )
      END IF

*  Report an error if the WPLATE extension item now has an illegal value.
      CALL CMP_GET0C( LOC, 'WPLATE', WPLATE, STATUS )
      IF( WPLATE .NE. '0.0' .AND. WPLATE .NE. '45.0' .AND.
     :    WPLATE .NE. '22.5' .AND. WPLATE .NE. '67.5' .AND.
     :    STATUS .EQ. SAI__OK ) THEN
         STATUS = SAI__ERROR
         CALL MSG_SETC( 'WP', WPLATE )
         CALL ERR_REP( 'POLIMP_BADWPL', 'Extension item WPLATE has '//
     :                 'the illegal value ''^WP''.', STATUS )
      END IF

*  Get the value of the IMGID component, creating it with a blank value if  
*  it does not currently exist in the extension.
      CALL DAT_THERE( LOC, 'IMGID', THERE, STATUS )
      IF( .NOT. THERE ) THEN
         CALL DAT_NEW0C( LOC, 'IMGID', 1, STATUS ) 
         CALL CMP_PUT0C( LOC, 'IMGID', ' ', STATUS ) 
         IMGID = ' '
      ELSE
         CALL CMP_GET0C( LOC, 'IMGID', IMGID, STATUS )
      END IF

*  If the current IMGID value is blank, delete it and create a new one
*  with a value equal to the basename of the NDF.
      IF( IMGID .EQ. ' ' ) THEN

*  Erase the existing component.
         CALL DAT_ERASE( LOC, 'IMGID', STATUS )

*  Find the full name of the NDF.
         CALL NDF_MSG( 'NDF', INDF )
         CALL MSG_LOAD( ' ', '^NDF', NDFNAM, LC, STATUS ) 

*  Find the position of the last "/" character marking the end of the
*  directory path.
         IF( STATUS .EQ. SAI__OK ) THEN
            LC = CHR_LEN( NDFNAM )
            I = LC
            DO WHILE( NDFNAM( I : I ) .NE. '/' .AND. I .GT. 0 )
               I = I - 1
            END DO

*  We use the string following the last "/".
            I = I + 1

*  Tell the user what is happening.
            IMGID = NDFNAM( I : LC ) 
            CALL MSG_SETC( 'IMGID', IMGID )
            CALL MSG_OUT( ' ', '     Setting IMGID to ''^IMGID''', 
     :                    STATUS )

*  Create ther IMGID component and store the NDF basename as its value.
            CALL DAT_NEW0C( LOC, 'IMGID', LC - I + 1, STATUS ) 
            CALL CMP_PUT0C( LOC, 'IMGID', IMGID, STATUS ) 

         END IF
      END IF

*  If necessary, create a GRP group to hold the used IMGID values.
      IF( IGRP .EQ. GRP__NOID ) THEN
         CALL GRP_NEW( 'Used IMGIDs', IGRP, STATUS )
      ENDIF

*  See if the IMGID value has already been used. If so, issue a warning.
*  If not, add it to the group of used IMGID values.
      LC = CHR_LEN( IMGID ) 
      IF( LC .GT. 0 ) THEN
         CALL GRP_INDEX( IMGID( : LC ), IGRP, 1, INDX, STATUS )
      ELSE
         CALL GRP_INDEX( ' ', IGRP, 1, INDX, STATUS )
      END IF

      IF( INDX .GT. 0 ) THEN
         CALL MSG_SETC( 'IMGID', IMGID )
         CALL MSG_OUT( ' ', '     WARNING - The IMGID value ''^IMGID'' '
     :                 //'has already been used!', STATUS )
      ELSE      
         CALL GRP_PUT( IGRP, 1, IMGID, 0, STATUS ) 
      END IF

*  If the extension does not currently contain an ANGROT value,
*  create one with the default value of zero.
      CALL DAT_THERE( LOC, 'ANGROT', THERE, STATUS )
      IF( .NOT. THERE ) THEN
         CALL DAT_NEW0R( LOC, 'ANGROT', STATUS ) 
         CALL CMP_PUT0R( LOC, 'ANGROT', 0.0, STATUS ) 
         CALL MSG_SETC( 'IMGID', IMGID )
         CALL MSG_OUT( ' ', '     Setting ANGROT to 0.0 degrees.',
     :                 STATUS )
      END IF

*  If the extension does not currently contain a FILTER value,
*  create a blank one. Otherwise, get the existing one.
      CALL DAT_THERE( LOC, 'FILTER', THERE, STATUS )
      IF( .NOT. THERE ) THEN
         CALL DAT_NEW0C( LOC, 'FILTER', 1, STATUS ) 
         CALL CMP_PUT0C( LOC, 'FILTER', ' ', STATUS ) 
         FILTER = ' '
      ELSE
         CALL CMP_GET0C( LOC, 'FILTER', FILTER, STATUS )
      END IF

*  Append WPLATE to the FILTER value unless the FILTER value already
*  contains the WPLATE string.
      IAT = CHR_LEN( FILTER )
      IF( INDEX( FILTER, WPLATE ) .EQ. 0 ) THEN
         CALL CHR_APPND( '_', FILTER, IAT )
         CALL CHR_APPND( WPLATE, FILTER, IAT )
      END IF

*  Store the new FILTER value.
      CALL MSG_SETC( 'VL', FILTER( : IAT ) )
      CALL MSG_OUT( ' ', '     Setting FILTER to ''^VL''', STATUS )
      CALL DAT_ERASE( LOC, 'FILTER', STATUS )
      CALL DAT_NEW0C( LOC, 'FILTER', IAT, STATUS ) 
      CALL CMP_PUT0C( LOC, 'FILTER', FILTER( : IAT ), STATUS ) 

*  See if there is a CCDPACK extension. If not create one.
      CALL NDF_XSTAT( INDF, 'CCDPACK', THERE, STATUS )       
      IF ( .NOT. THERE ) THEN
         CALL NDF_XNEW( INDF, 'CCDPACK', 'CCDPACK_EXT', 0, 0, CCDLOC, 
     :                  STATUS ) 

*  Erase any FILTER component in the existing CCDPACK extension.
      ELSE
         CALL NDF_XLOC( INDF, 'CCDPACK', 'UPDATE', CCDLOC, STATUS ) 
         CALL DAT_THERE( CCDLOC, 'FILTER', THERE, STATUS )
         IF( THERE ) CALL DAT_ERASE( CCDLOC, 'FILTER', STATUS )         
      END IF

*  Store the new FILTER value in the CCDPACK extension.
      CALL DAT_NEW0C( CCDLOC, 'FILTER', IAT, STATUS ) 
      CALL CMP_PUT0C( CCDLOC, 'FILTER', FILTER( : IAT ), STATUS ) 

*  Annul the locator to the CCDPACK extension.
      CALL DAT_ANNUL( CCDLOC, STATUS )

*  Add a Frame to the NDFs WCS component in which the first axis
*  corresponds to the analyser WPLATE=0.0 axis. 
*  ============================================================
*  Get the number of axes in the NDF.
      CALL NDF_DIM( INDF, NDF__MXDIM, DIM, NDIM, STATUS )

*  Start an AST context.
      CALL AST_BEGIN( STATUS )

*  Get an AST pointer for the FrameSet stored in the WCS component of
*  the NDF.
      CALL NDF_GTWCS( INDF, IWCS, STATUS )

*  Note the original Current frame.
      ICURR = AST_GETI( IWCS, 'CURRENT' )

*  Remove any existing POLANL Frame.
      IF( AST_FINDFRAME( IWCS, AST_FRAME( NDIM, 'MINAXES=1, MAXAXES=20',
     :                STATUS ), 'POLANL', STATUS ) .NE. AST__NULL ) THEN

         IPOLAN = AST_GETI( IWCS, 'CURRENT' )
         CALL AST_REMOVEFRAME( IWCS, AST__CURRENT, STATUS )

*  Correct the original index of the Current Frame to take account of the
*  removed Frame.
         IF( ICURR .GT. IPOLAN ) THEN
            ICURR = ICURR - 1
         ELSE IF( ICURR .EQ. IPOLAN ) THEN
            ICURR = AST__NOFRAME
         END IF

      END IF

*  Create a mapping from pixel coordinates (given by the PIXEL Frame in the
*  FrameSet), to a 2D cartesian coordinate system in which the X axis is
*  parallel to the analyser WPLATE = 0.0 axis. First get the
*  anti-clockwise angle in degrees fro mthe pixel X axis to the analyser X
*  axis.
      CALL CMP_GET0R( LOC, 'ANGROT', ANGROT, STATUS )

*  If the rotation is zero, this is just a unit mapping.
      IF( ANGROT .EQ. 0.0 ) THEN
         MAP = AST_UNITMAP( NDIM, ' ', STATUS )

*  Otherwise, create a MatrixMap describing the rotation from pixel
*  coordinates to analyser coordinates. 
      ELSE

*  Set the entire matrix to zero.
         DO I = 1, NDIM*NDIM
            MAT( I ) = 0.0
         END DO

*  Set the diagonal elements to 1.0.
         DO I = 0, NDIM - 1
            MAT( 1 + ( NDIM + 1 )*I ) = 1.0
         END DO

*  Store the  trig terms describing the rotation of the first 2 axes.
         COS = COS( DTOR*DBLE( ANGROT ) )
         SIN = SIN( DTOR*DBLE( ANGROT ) )

         MAT( 1 ) = COS
         MAT( 2 ) = SIN
         MAT( 1 + NDIM ) = -SIN
         MAT( 2 + NDIM ) = COS

*  Create the MatrixMap.
         MAP = AST_MATRIXMAP( NDIM, NDIM, 0, MAT, ' ', STATUS )
      END IF

*  Now try to find the PIXEL Frame in the NDF's FrameSet. The PIXEL
*  Frame becomes the current Frame if found.
      IF( AST_FINDFRAME( IWCS, AST_FRAME( NDIM, ' ', STATUS ), 'PIXEL',
     :                   STATUS ) .NE. AST__NULL ) THEN

*  Get the index of the current Frame (i.e. the PIXEL Frame) within the
*  FrameSet.
         IPIXEL = AST_GETI( IWCS, 'CURRENT', STATUS )

*  Create the Frame describing analyser coordinates. Use the domain
*  POLANL to identify it.
         FRAME = AST_FRAME( NDIM, 'Domain=POLANL Title=Polpack '//
     :                      'analyser frame', STATUS )

*  Now add the analyser Frame into the FrameSet, using the Mapping created
*  above to connect it to the pixel coordinate Frame.
         CALL AST_ADDFRAME( IWCS, IPIXEL, MAP, FRAME, STATUS )

*  Reinstate the original Current Frame (if it still exists).
         IF( ICURR .NE. AST__NOFRAME ) THEN
            CALL AST_SETI( IWCS, 'Current', ICURR )
         END IF

*  Store the new FrameSet back in the NDF.
         CALL NDF_PTWCS( INDF, IWCS, STATUS )

      END IF

*  End the AST context.
      CALL AST_END( STATUS )

      END
