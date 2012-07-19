      SUBROUTINE ARY_DUPE( IARY1, PLACE, IARY2, STATUS )
*+
*  Name:
*     ARY_DUPE

*  Purpose:
*     Duplicate an array.

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL ARY_DUPE( IARY1, PLACE, IARY2, STATUS )

*  Description:
*     The routine duplicates an array, creating a new base array with
*     the same attributes as an existing array (or array section). The
*     new array is left in an undefined state.

*  Arguments:
*     IARY1 = INTEGER (Given)
*        Identifier for the array to be duplicated.
*     PLACE = INTEGER (Given and Returned)
*        An array placeholder (e.g. generated by the ARY_PLACE routine)
*        which indicates the position in the data system where the new
*        array will reside. The placeholder is annulled by this
*        routine, and a value of ARY__NOPL will be returned (as defined
*        in the include file ARY_PAR).
*     IARY2 = INTEGER (Returned)
*        Identifier for the new duplicate array.
*     STATUS = INTEGER (Given and Returned)
*        The global status.

*  Notes:
*     -  Duplicating a scaled or delta array produces an equivalent
*     simple array.
*     -  If this routine is called with STATUS set, then a value of
*     ARY__NOID will be returned for the IARY2 argument, although no
*     further processing will occur. The same value will also be
*     returned if the routine should fail for any reason.  In either
*     event, the placeholder will still be annulled. The ARY__NOID
*     constant is defined in the include file ARY_PAR.

*  Algorithm:
*     -  Set an initial value for the IARY2 argument before checking the
*     inherited status.
*     -  Save the error status and mark the error stack.
*     -  Import the array placeholder.
*     -  If no error has occurred, then import the input array
*     identifier.
*     -  Obtain an index to the data object entry in the DCB and ensure
*     that storage form information is available for it.
*     -  Handle each form of array in turn.
*     -  For primitive arrays, ensure that data type and bounds
*     information is available for the data object.
*     -  See if the array's bounds are suitable for creating a new
*     primitive array.
*     -  Create a new data object with the same attributes and an entry
*     in the DCB. Create a primitive array if possible. Otherwise
*     create a simple array.
*     -  For simple arrays, ensure that data type and bounds information
*     are available for the data object.
*     -  Create a new data object with the same attributes and an entry
*     in the DCB.
*     -  Create a new base array entry in the ACB to describe it.
*     -  If the DCB form entry was not recognised, then report an error.
*     -  Export an identifier for the new array.
*     -  If an error has occurred, then annul the new ACB entry.
*     -  Annul the placeholder, erasing the associated object if an
*     error has occurred.
*     -  Reset the PLACE argument.
*     -  Restore the error context, reporting additional context
*     information if appropriate.

*  Copyright:
*     Copyright (C) 2010 Science & Technology Facilities Council.
*     All Rights Reserved.
*     Copyright (C) 1989, 1990 Science & Engineering Research Council.
*     All Rights Reserved.

*  Licence:
*     This program is free software; you can redistribute it and/or
*     modify it under the terms of the GNU General Public License as
*     published by the Free Software Foundation; either version 2 of
*     the License, or (at your option) any later version.
*
*     This program is distributed in the hope that it will be
*     useful,but WITHOUT ANY WARRANTY; without even the implied
*     warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
*     PURPOSE. See the GNU General Public License for more details.
*
*     You should have received a copy of the GNU General Public License
*     along with this program; if not, write to the Free Software
*     Foundation, Inc., 51 Franklin Street,Fifth Floor, Boston, MA
*     02110-1301, USA

*  Authors:
*     RFWS: R.F. Warren-Smith (STARLINK)
*     DSB: David S Berry (JAC)
*     {enter_new_authors_here}

*  History:
*     20-OCT-1989 (RFWS):
*        Original version.
*     10-NOV-1989 (RFWS):
*        Added status check after call to ARY1_DFRM.
*     27-NOV-1989 (RFWS):
*        Fixed bug causing duplicated array to take its bounds from the
*        DCB entry of the input array, rather than its ACB entry.
*     28-NOV-1989 (RFWS):
*        Re-structured, to create the new ACB entry after the
*        form-dependent part of the routine.
*     13-FEB-1990 (RFWS):
*        Installed support for primitive arrays.
*     5-MAY-2006 (DSB):
*        Installed support for scaled arrays.
*     12-JUL-2006 (DSB):
*        Changed so that that duplicating a scaled array produces a simple
*        array.
*     17-JUL-2006 (DSB):
*        Changed so that creation of the HDS array is deferred until it
*        is needed. This prevents arrays being created that are larger
*        than they need to be.
*     1-NOV-2010 (DSB):
*        Include support for delta compressed arrays.
*     19-JUL-2012 (DSB):
*        The new array data type should take account of any change in
*        data type caused by the old array being stored in delta or
*        scaled form.
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*  Global Constants:
      INCLUDE 'SAE_PAR'          ! Standard SAE constants
      INCLUDE 'DAT_PAR'          ! DAT_ public constants
      INCLUDE 'ARY_PAR'          ! ARY_ public constants
      INCLUDE 'ARY_CONST'        ! ARY_ private constants
      INCLUDE 'ARY_ERR'          ! ARY_ error codes

*  Global Variables:
      INCLUDE 'ARY_DCB'          ! ARY_ Data Control Block
*        DCB_CPX( ARY__MXDCB ) = LOGICAL (Read)
*           Whether data object is complex.
*        DCB_FRM( ARY__MXDCB ) = CHARACTER * ( ARY__SZFRM ) (Read)
*           Data object storage form.
*        DCB_LBND( ARY__MXDIM, ARY__MXDCB ) = INTEGER (Read)
*           Lower bounds of data object.
*        DCB_NDIM( ARY__MXDCB ) = INTEGER (Read)
*           Number of data object dimensions.
*        DCB_TYP( ARY__MXDCB ) = CHARACTER * ( ARY__SZTYP ) (Read)
*           Data object numeric type.
*        DCB_UBND( ARY__MXDIM, ARY__MXDCB ) = INTEGER (Read)
*           Upper bounds of data object.

      INCLUDE 'ARY_ACB'          ! ARY_ Access Control Block
*        ACB_IDCB( ARY__MXACB ) = INTEGER (Read)
*           Index to data object entry in the DCB.

      INCLUDE 'ARY_PCB'          ! ARY_ Placeholder Control Block
*        PCB_LOC( ARY__MXPCB ) = CHARACTER * ( DAT__SZLOC ) (Read)
*           Placeholder locator.
*        PCB_TMP( ARY__MXPCB ) = LOGICAL (Read)
*           Whether the object which replaces the placeholder object
*           should be temporary.

*  Arguments Given:
      INTEGER IARY1

*  Arguments Given and Returned:
      INTEGER PLACE

*  Arguments Returned:
      INTEGER IARY2

*  Status:
      INTEGER STATUS             ! Global status

*  Local Variables:
      CHARACTER NEWTYP*(DAT__SZTYP)! Data type for new array
      INTEGER IACB1              ! Index to input array entry in ACB
      INTEGER IACB2              ! Index to output array entry in ACB
      INTEGER IDCB1              ! Input data object entry in the DCB
      INTEGER IDCB2              ! Output data object entry in the DCB
      INTEGER IPCB               ! Index to placeholder entry in the PCB
      INTEGER TSTAT              ! Temporary status variable
      LOGICAL ERASE              ! Whether to erase placeholder object
      LOGICAL PBND               ! Whether array bounds are primitive

*.

*  Set an initial value for the IARY2 argument.
      IARY2 = ARY__NOID

*  Save the STATUS value and mark the error stack.
      TSTAT = STATUS
      CALL ERR_MARK

*  Import the array placeholder, converting it to a PCB index.
      STATUS = SAI__OK
      IPCB = 0
      CALL ARY1_IMPPL( PLACE, IPCB, STATUS )

*  If there has been no error at all so far, then import the input array
*  identifier.
      IF ( ( STATUS .EQ. SAI__OK ) .AND. ( TSTAT .EQ. SAI__OK ) ) THEN
         CALL ARY1_IMPID( IARY1, IACB1, STATUS )
         IACB2 = 0
         IF ( STATUS .EQ. SAI__OK ) THEN

*  Obtain an index to the input data object entry in the DCB and ensure
*  that storage form information is available for it.
            IDCB1 = ACB_IDCB( IACB1 )
            CALL ARY1_DFRM( IDCB1, STATUS )

*  Handle each form of array in turn...
            IF ( STATUS .EQ. SAI__OK ) THEN

*  Primitive arrays.
*  ================
               IF ( DCB_FRM( IDCB1 ) .EQ. 'PRIMITIVE' ) THEN

*  Ensure that data type and bounds information is available for the
*  data object.
                  CALL ARY1_DTYP( IDCB1, STATUS )
                  CALL ARY1_DBND( IDCB1, STATUS )

*  See if the array's bounds are suitable for creating a new primitive
*  array.
                  CALL ARY1_PBND( IACB1, PBND, STATUS )
                  IF ( STATUS .EQ. SAI__OK ) THEN

*  Create a new data object with the same attributes and an entry in
*  the DCB. Create a primitive array if possible. Otherwise create a
*  simple array.
                     IF ( PBND ) THEN
                        CALL ARY1_DCREP( .TRUE., DCB_TYP( IDCB1 ),
     :                                   ACB_NDIM( IACB1 ),
     :                                   ACB_UBND( 1, IACB1 ),
     :                                   PCB_TMP( IPCB ),
     :                                   PCB_LOC( IPCB ), IDCB2,
     :                                   STATUS )
                     ELSE
                        CALL ARY1_DCRE( .TRUE., DCB_TYP( IDCB1 ),
     :                                  DCB_CPX( IDCB1 ),
     :                                  ACB_NDIM( IACB1 ),
     :                                  ACB_LBND( 1, IACB1 ),
     :                                  ACB_UBND( 1, IACB1 ),
     :                                  PCB_TMP( IPCB ),
     :                                  PCB_LOC( IPCB ), IDCB2, STATUS )
                     END IF
                  END IF

*  Simple, delta and scaled arrays.
*  ================================
               ELSE IF ( DCB_FRM( IDCB1 ) .EQ. 'SIMPLE' .OR.
     :                   DCB_FRM( IDCB1 ) .EQ. 'SCALED' .OR.
     :                   DCB_FRM( IDCB1 ) .EQ. 'DELTA' ) THEN

*  Ensure that data type and bounds information is available for the
*  data object.
                  CALL ARY1_DTYP( IDCB1, STATUS )
                  CALL ARY1_DBND( IDCB1, STATUS )

*  Get the required data type for the new array.
                  CALL ARY1_EXTYP( IDCB1, NEWTYP, STATUS )

*  Create a new data object with the same attributes and an entry in the
*  DCB. This is a simple array. The act of duplicating a scaled or delta
*  array creates a simple array.
                  CALL ARY1_DCRE( .TRUE., NEWTYP,
     :                            DCB_CPX( IDCB1 ), ACB_NDIM( IACB1 ),
     :                            ACB_LBND( 1, IACB1 ),
     :                            ACB_UBND( 1, IACB1 ), PCB_TMP( IPCB ),
     :                            PCB_LOC( IPCB ), IDCB2, STATUS )


*  If the DCB form entry was not recognised, then report an error.
               ELSE
                  STATUS = ARY__FATIN
                  CALL MSG_SETC( 'BADFORM', DCB_FRM( IDCB1 ) )
                  CALL ERR_REP( 'ARY_DUPE_FORM',
     :            'Invalid array form ''^BADFORM'' found in Data ' //
     :            'Control Block (internal programming error).',
     :            STATUS )
               END IF
            END IF

*  Create a base array entry in the ACB to describe the new data object.
            CALL ARY1_CRNBA( IDCB2, IACB2, STATUS )

*  Export an identifier for the new array.
            CALL ARY1_EXPID( IACB2, IARY2, STATUS )
         END IF
      END IF

*  If an error has occurred, then annul the new ACB entry.
      IF ( ( STATUS .NE. SAI__OK ) .AND. ( IACB2 .NE. 0 ) ) THEN
         CALL ARY1_ANL( IACB2, STATUS )
      END IF

*  Annul the placeholder, erasing the associated object if any error has
*  occurred.
      IF ( IPCB .NE. 0 ) THEN
         ERASE = ( STATUS .NE. SAI__OK ) .OR. ( TSTAT .NE. SAI__OK )
         CALL ARY1_ANNPL( ERASE, IPCB, STATUS )
      END IF

*  Reset the PLACE argument.
      PLACE = ARY__NOPL

*  Annul any error if STATUS was previously bad, otherwise let the new
*  error report stand.
      IF ( STATUS .NE. SAI__OK ) THEN
         IF ( TSTAT .NE. SAI__OK ) THEN
            CALL ERR_ANNUL( STATUS )
            STATUS = TSTAT

*  Report context information and call the error tracing routine if
*  appropriate.
         ELSE
            CALL ERR_REP( 'ARY_DUPE_ERR',
     :      'ARY_DUPE: Error duplicating an array.', STATUS )
            CALL ARY1_TRACE( 'ARY_DUPE', STATUS )
         END IF
      ELSE
         STATUS = TSTAT
      END IF

*  Release the error stack.
      CALL ERR_RLSE

      END
