      SUBROUTINE GKS1_ACTIV ( STATUS )
*+
*  Name:
*     GKS1_ACTIV

*  Purpose:
*     Activate the GKS package

*  Language:
*     Starlink Fortran 77

*  Invocation:
*     CALL GKS1_ACTIV( STATUS )

*  Description:
*     GKS is opened if it is not already opened and the table of assigned 
*     graphics devices in the GKS_PA Common Block is initialised.

*  Arguments:
*     STATUS = INTEGER (Given and returned)
*        The global status.

*  Authors:
*     SLW: Sid Wright (UCL)
*     BDK: Dennis Kelly (ROE)
*     AJC: A J Chipperfield (Starlink, RAL)
*     DLT  David Terrett (Starlink, RAL)
*     {enter_new_authors_here}

*  History:
*     11-OCT-1983 (SLW):
*        Starlink Version.
*     04-MAR-1985 BDK):
*        ADAM version.
*     21-FEB-1986 (AJC):   
*        Use GERHND to initialize error flag.
*     29-MAY-1986 (AJC):
*        Open GKS if not already (for NOCLR flag ops).
*     09-JAN-1992 (DLT):   
*        Convert to Unix and GKS 7.4, rename to GKS1_ACTIV and reformat 
*        comments.
*        Declare GKS1_BLK as EXTERNAL to force loading on block data subprogram
*     {enter_further_changes_here}

*  Bugs:
*     {note_any_bugs_here}

*-

*  Type Definitions:
      IMPLICIT NONE              ! No implicit typing

*    Global constants :
      INCLUDE 'SAE_PAR'   ! Standard SAE Constants
      INCLUDE 'PAR_PAR'   ! PAR constants
      INCLUDE 'GKS_PAR'   ! GKS Internal Parameters
      INCLUDE 'gksenv_par'              ! GKS Environment Symbolic Constants

*    Status:
      INTEGER STATUS                    ! Status

*    Global variables:
      INCLUDE 'gkspa_cmn'               ! GKS Parameter Table
      INCLUDE 'gksgo_cmn'               ! GKS Initialisation Switch
      EXTERNAL GKS1_BLK

*    Local variables:
      INTEGER I                         ! loop index
      INTEGER IOPSTA                    ! GKS Open status

*    Local constants:
      INTEGER LUEGKS                    ! Unit no. for GKS errors (not used)
      PARAMETER (LUEGKS = 6)

*.

      IF ( STATUS .NE. SAI__OK ) RETURN

*   Initialise common blocks
      DO I = 1, GKS__MXPAR
         PFREE(I) = .TRUE.
         PDESC(I) = 0
         PTNAME(I) = ' '
      ENDDO

*   See if GKS open
      CALL GQOPS( IOPSTA )
      IF (IOPSTA .EQ. GGKCL) THEN
         CALL GOPKS( LUEGKS )
      ENDIF
      CALL GKS_GSTAT ( STATUS )
      IF ( STATUS .NE. SAI__OK ) RETURN

*   Declare GKS awake
      GKSSLP = .FALSE.

      END
