*+  FOLDBIN - Folds binned data into phase bins
      SUBROUTINE FOLDBIN( STATUS )
*    Description :
*     Folds nD data into phase bins and generates a folded nD data set
*     containing the bin means (weighted if errors are available) and standard
*     deviations, also the chi-squared fits to constant levels.
*     Note - if axis zero is defined then this must be incorporated in the
*     phase zero epoch.
*    Method :
*     The basic technique is straightforward.
*     Arrays are mapped so there is no size limitation.
*     Bad quality data are excluded.
*     If time is not AXIS1 then the axes are swapped.
*    Deficiencies :
*    Bugs :
*    Authors :
*     Trevor Ponman  (BHVAD::TJP)
*     Phillip Andrews (PLA_ROSAT@uk.ac.bham.sr.star)
*     David J. Allan (BHVAD::DJA)
*     Simon Duck (BHVAD::SRD)
*     Andrew Norton (SOTON::AJN)
*    History :
*     26 Mar 86: Original (TJP)
*     29 Aug 86: Renamed (JCMP)
*
*     10 Sep 87: Accepts primitive input object. Checks validity of locators
*                before attempting to annul them. Displays name of input object.
*                Displays AXIS(1) UNITS. Displays max No of output bins (and
*                makes sure this is not exceeded). Program suggests defaults
*                for phase zero epoch and No of phase bins
*                (= No of input bins in phase period). ACTIONS added to
*                History record. (PLA)
*     10 Dec 89: Ghastly spelling mistakes removed! Axis(1) text now correct.
*                Uses MSG_PRNT now. (DJA)
*     12 Jan 90: V1.0-3 Re-write to use ASTLIB! No longer generates negative
*                       variances. Doesn't re-prompt after invalid input.
*                       The TIM_ routines now use IMPLICIT NONE. (BHVAD::DJA)
*     10 Apr 90: V1.0-4 Accepts nD data and folds each dimension over time. If
*                       time is not the AXIS(1) then the axes are swapped so
*                       that it is. (SRD)
*     19 Jul 90:      - bugs fixed in TIM_FOLDU : arrays  BVAR & YBAR had
*                       not been initialised to zero (AJN)
*     24 Jul 90:      - serious bug fixed - epoch zero was carried over
*                       to subroutines in days and calculation was done
*                       in seconds - now converted to seconds before use,
*                       also used in dble precision for accuracy with short
*                       periods & long offsets (AJN)
*     31 Jul 91:      - prints a warning when the max. no. of phase bine
*                       for ordinary sampling is used. (LTVAD::RDS)
*      7 Oct 92: V1.7-0 TIM_FOLDx routine renamed FOLDBIN_FOLDx to avoid
*                       clash with FOLDLOTS routines of same name but
*                       different function (DJA)
*     30 Mar 93: V1.7-1 Updated by someone (not me!) (DJA)
*     17 Aug 93: V1.7-2 Trivial changes to ARR_REG routines (DJA)
*     18 Apr 95 : V1.8-0 Updated data interface (DJA)
*
*    Type Definitions :
*
      IMPLICIT NONE
*
*    Global constants :
*
      INCLUDE 'SAE_PAR'
      INCLUDE 'ADI_PAR'
      INCLUDE 'QUAL_PAR'
*
*    Status :
*
      INTEGER STATUS
*
*    Local Constants :
*
      INTEGER            MAXHTXT                ! Maximum amount of history
         PARAMETER       (MAXHTXT = 10)
*
*    Local variables :
*
      CHARACTER*132      HTXT(MAXHTXT)          ! History text
      CHARACTER*80       UNITS                  ! Value of AXIS(1) UNITS.

      DOUBLE PRECISION   BASE_TAI               ! Atomic time
      DOUBLE PRECISION   ZEROEPOCH              ! Epoch of phase zero
      DOUBLE PRECISION   ZEROOFFSET             ! offset of zeroepoch from base_tai

      REAL               BASE, SCALE            ! Axis attributes
      REAL               PERIOD                 ! Folding period
      REAL               CHISQ                  ! Chi-squared fit to constant
                                                ! level (i.e.phase indept.)

      INTEGER			FFID			! Folded dataset id
      INTEGER			IFID			! Input dataset id
      INTEGER			TIMID			! Timing info
      INTEGER            HLEN                   ! Length of a history string
      INTEGER            I                      ! Dummy variable for loops.
      INTEGER            N
      INTEGER            L
      INTEGER            NDATA                  ! No.of data points
      INTEGER            IERR                   ! Error flag from TIM_FOLD
      INTEGER            NBINS                  ! No. of phase bins
      INTEGER            NELM
      INTEGER            NELM2
      INTEGER            NACTDIM                ! Dimensionality of data
      INTEGER            VNDIM                  ! Dimensions of variance
      INTEGER            VDIMS(ADI__MXDIM)      ! size of variance
      INTEGER            VDIM(ADI__MXDIM)
      INTEGER            QNDIM                  ! dimensions of quality
      INTEGER            QDIMS(ADI__MXDIM)      ! size of quality
      INTEGER            QDIM(ADI__MXDIM)
      INTEGER            DPTR                   ! Pointer to data array
      INTEGER            XPTR                   ! Pointer to axis data
      INTEGER            XWPTR                  ! Pointer to width data
      INTEGER            VPTR                   ! Pointer to variances
      INTEGER            QPTR                   ! Pointer to quality array
      INTEGER            FDPTR                  ! Pointer to folded data array
      INTEGER            FXPTR                  ! Pointer to folded axis data
      INTEGER            FVPTR                  ! Pointer to folded error array
      INTEGER            FNPTR                  ! Pointer to folded no. array
      INTEGER            FQPTR                  ! Pointer to folded quality array
      INTEGER            FFPTR
      INTEGER            FCPTR
      INTEGER            WPTR                   ! Pointer to work array
      INTEGER            PTR
      INTEGER            QAPTR
      INTEGER            VAPTR
      INTEGER            YBARPTR
      INTEGER            SUMPTR
      INTEGER            NDOF                   ! No. of d.o.f. for chi-squared
      INTEGER            IDIM(ADI__MXDIM)       ! Size of each dimension
      INTEGER            ODIM(ADI__MXDIM)
      INTEGER            OAX(ADI__MXDIM)
      INTEGER            AXDIM
      INTEGER            INBINS                 ! Number of input bins/ period.
      INTEGER            SIZ
      INTEGER            USED                   ! # history text lines

      BYTE               MASK                   ! Quality mask

      LOGICAL            OK                 	! things ok?
      LOGICAL            VOK                    ! Variance present?
      LOGICAL            QOK                    ! Data quality present?
      LOGICAL            REGULAR                ! Is input regularly binned?
      LOGICAL            BOK                    ! BASE_TAI present
      LOGICAL            BAD                    ! Any BAD data?
      LOGICAL            WEIGHT
*
*    Version id
*
      CHARACTER*30		VERSION
        PARAMETER       	( VERSION = 'FOLDBIN Version 1.8-0' )
*-

*    Check status.
      IF ( STATUS .NE. SAI__OK ) RETURN

*    Initialise
      CALL AST_INIT

*    Version
      CALL MSG_PRNT( VERSION )

*    Obtain data object, access and check it.
      CALL USI_TASSOCI( 'INP', '*', 'READ', IFID, STATUS )

*    Check status - drop out if bad
      IF ( STATUS .NE. SAI__OK ) GOTO 99

*    Check and map input data
      CALL BDI_CHKDATA( IFID, OK, NACTDIM, ODIM, STATUS )
      IF ( OK ) THEN
         CALL BDI_MAPDATA( IFID, 'READ', PTR, STATUS )

*    Swap axes if necessary
         CALL AXIS_TFIND(IFID,'TIME',NACTDIM,AXDIM,STATUS)
         IF(AXDIM.NE.1)THEN
            OAX(1)=AXDIM
            DO I=2,AXDIM
               OAX(I)=I-1
            ENDDO
            DO I=AXDIM+1,ADI__MXDIM
               OAX(I)=I
            ENDDO
            IDIM(1)=ODIM(AXDIM)
            DO I=2,AXDIM
               IDIM(I)=ODIM(I-1)
            ENDDO
            DO I=AXDIM+1,NACTDIM
               IDIM(I)=ODIM(I)
            ENDDO
            CALL DYN_MAPR(NACTDIM,IDIM,DPTR,STATUS)
            DO I=NACTDIM,ADI__MXDIM
               IF(ODIM(I).EQ.0)ODIM(I)=1
               IF(IDIM(I).EQ.0)IDIM(I)=1
            ENDDO
            CALL AR7_AXSWAP_R(ODIM,%VAL(PTR),OAX,IDIM,%VAL(DPTR),STATUS)
            CALL DYN_UNMAP(PTR,STATUS)
         ELSE
            DPTR=PTR
            DO I=1,NACTDIM
              OAX(I)=I
              IDIM(I)=ODIM(I)
            ENDDO
         ENDIF
         NDATA=IDIM(1)
      ELSE
         CALL MSG_PRNT( 'FATAL ERROR: No data!' )
      END IF

*  Check number of dimensions
      CALL MSG_SETI('DIMS',NACTDIM)
      CALL MSG_PRNT( 'Data is ^DIMS dimensional' )

*  Create a folded data object.
      CALL USI_TASSOCO( 'OUT', 'FOLDED_SERIES', FFID, STATUS )
      IF (STATUS .NE. SAI__OK) GOTO 99

*    If AXIS(1) UNITS are defined, then display them.
      UNITS = 'Units'
      CALL BDI_GETAXUNITS( IFID, AXDIM, UNITS, STATUS )
      CALL MSG_SETC( 'UNITS', UNITS )
      CALL MSG_PRNT( 'TIME UNITS are ^UNITS' )

*    Get period
      CALL USI_GET0R( 'PERIOD', PERIOD, STATUS )

*    Define base epoch
      CALL TCI_GETID( IFID, TIMID, STATUS )
      CALL ADI_THERE( TIMID, 'TAIObs', BOK, STATUS )
      IF ( BOK ) THEN
        CALL ADI_CGET0D( TIMID, 'TAIObs', BASE_TAI, STATUS )
      ELSE
        CALL MSG_PRNT( 'No atomic time: set to zero' )
        BASE_TAI = 0.0D0
      ENDIF
      CALL USI_DEF0D( 'EPOCH', BASE_TAI, STATUS )
      CALL USI_GET0D( 'EPOCH', ZeroEpoch, STATUS )
      ZEROOFFSET = (ZEROEPOCH - BASE_TAI) * 86400.D0

*    Check axis and data match
      CALL BDI_CHKAXVAL( IFID, AXDIM, OK, REGULAR, SIZ, STATUS )
      IF ( SIZ .NE. NDATA ) THEN
         STATUS=SAI__ERROR
         CALL ERR_REP('NBAD','Axis and Data do not match',STATUS)
         CALL ERR_FLUSH(STATUS)
      END IF
      IF ( OK ) THEN

*       Find number of input bins within period.
         IF ( REGULAR ) THEN
            CALL BDI_GETAXVAL( IFID, AXDIM, BASE, SCALE, SIZ, STATUS )
            INBINS = INT( PERIOD / ABS(SCALE) )
            CALL MSG_SETI( 'INBINS', INBINS )
C            CALL MSG_PRNT( 'Maximum number of phase bins is ^INBINS' )
            CALL USI_DEF0I( 'BINS', INBINS, STATUS )
            CALL DYN_MAPR( 1, NDATA, XPTR, STATUS )
            CALL DYN_MAPR( 1, NDATA, XWPTR, STATUS )
            CALL ARR_REG1R( BASE, SCALE, NDATA, %VAL(XPTR), STATUS )
            CALL ARR_INIT1R( SCALE, NDATA, %VAL(XWPTR), STATUS )
         ELSE

*          Map time AXIS data
            CALL BDI_MAPAXVAL( IFID, 'READ', AXDIM, XPTR, STATUS )
            CALL BDI_MAPAXWID( IFID, 'READ', AXDIM, XWPTR, STATUS )
         END IF

      ELSE
         CALL MSG_PRNT( 'No AXIS(1) data'
     :                //' - proceeding assuming unit spacing' )

*       Set up scratch area & fill with 0,1,2...
         CALL DYN_MAPR( 1, NDATA, XPTR, STATUS )
         CALL ARR_REG1R( 0.0, 1.0, NDATA, %VAL(XPTR), STATUS )

      END IF

      CALL USI_GET0I( 'BINS', NBINS, STATUS )

      IF ( REGULAR .AND. (NBINS .GT. INBINS) ) THEN
         CALL MSG_PRNT('The data is being oversampled - this is'/
     &               /' only valid if the time series is long compared'/
     &               /' to the folding period')
      END IF

*    Ask if weighted mean required
      CALL USI_GET0L('WEIGHT',WEIGHT,STATUS)
      IF ( STATUS .NE. SAI__OK ) GOTO 99

*    Data error ... set up scratch area for variances
      CALL BDI_CHKVAR(IFID,VOK,VNDIM,VDIMS,STATUS)
      IF ( VOK ) THEN
         IF(VNDIM.NE.NACTDIM) THEN
            STATUS=SAI__ERROR
            CALL ERR_REP('DIM','Variance does not match Data',STATUS)
            CALL ERR_FLUSH(STATUS)
         END IF
         CALL BDI_MAPVAR(IFID,'READ',VAPTR,STATUS)
         IF(AXDIM.NE.1)THEN
            CALL DYN_MAPR(NACTDIM,IDIM,VPTR,STATUS)
            CALL AR7_AXSWAP_R(VDIMS,%VAL(VAPTR),OAX,VDIM,
     :                                 %VAL(VPTR),STATUS)
            CALL DYN_UNMAP(VAPTR,STATUS)
         ELSE
            VPTR=VAPTR
         ENDIF
      ELSE
         CALL DYN_MAPR(NACTDIM,IDIM,VPTR,STATUS)
         CALL ARR_SUMDIM(NACTDIM,IDIM,NELM)
         CALL ARR_INIT1R(0.0,NELM,%VAL(VPTR),STATUS)
      ENDIF

*    and data quality
      CALL BDI_CHKQUAL(IFID,QOK,QNDIM,QDIMS,STATUS)
      IF ( QOK ) THEN
         IF(QNDIM.NE.NACTDIM) THEN
            CALL ERR_REP('QIM','Quality does not match Data',STATUS)
            CALL ERR_FLUSH(STATUS)
            STATUS=SAI__ERROR
         END IF
         CALL BDI_GETMASK(IFID,MASK,STATUS)
         CALL BDI_MAPLQUAL(IFID,'READ',BAD,QAPTR,STATUS)
         IF(AXDIM.NE.1)THEN
           CALL DYN_MAPR(NACTDIM,IDIM,QPTR,STATUS)
           CALL AR7_AXSWAP_B(QDIMS,%VAL(QAPTR),OAX,QDIM,
     :                              %VAL(QPTR),STATUS)
           CALL DYN_UNMAP(QAPTR,STATUS)
         ELSE
           QPTR=QAPTR
         ENDIF
      ELSE
         CALL STR_CTOB('11111111',MASK,STATUS)
      END IF
      IF(STATUS.NE.SAI__OK) THEN
          CALL ERR_FLUSH(STATUS)
      END IF

*    Map workspace
      IDIM(1)=NBINS
      CALL DYN_MAPR(NACTDIM,IDIM,WPTR,STATUS)
      CALL DYN_MAPR(NACTDIM,IDIM,YBARPTR,STATUS)
      CALL DYN_MAPR(NACTDIM,IDIM,SUMPTR,STATUS)

*    Set up and map components in output object
      CALL BDI_CREDATA(FFID,NACTDIM,IDIM,STATUS)
      CALL BDI_CREVAR(FFID,NACTDIM,IDIM,STATUS)
      CALL BDI_CREAXES(FFID,NACTDIM,STATUS)
      DO L=1,NACTDIM
        CALL BDI_CREAXVAL(FFID,L,.FALSE.,IDIM(L),STATUS)
      ENDDO
      CALL BDI_CREQUAL(FFID,NACTDIM,IDIM,STATUS)
      CALL BDI_MAPDATA(FFID,'WRITE',FDPTR,STATUS)
      CALL BDI_MAPVAR(FFID,'WRITE',FVPTR,STATUS)
      CALL BDI_MAPAXVAL(FFID,'WRITE',1,FXPTR,STATUS)
      CALL BDI_MAPQUAL(FFID,'WRITE',FQPTR,STATUS)
      CALL BDI_PUTMASK(FFID,MASK,STATUS)
      IF ( STATUS.NE.SAI__OK ) THEN
         CALL ERR_FLUSH(STATUS)
      END IF

*  Workspace
      CALL DYN_MAPI( NACTDIM, IDIM, FNPTR, STATUS )
      IDIM(1)=1
      CALL DYN_MAPR( NACTDIM, IDIM, FCPTR, STATUS )
      CALL DYN_MAPI( NACTDIM, IDIM, FFPTR, STATUS )
      IDIM(1)=NBINS

*    Set output quality to GOOD (as all bad data has been excluded)
      CALL ARR_SUMDIM(NACTDIM,IDIM,NELM2)
      CALL ARR_INIT1B(QUAL__GOOD,NELM2,%VAL(FQPTR),STATUS)

      IDIM(1)=NDATA

      DO I=2,7
        IF(IDIM(I).LT.1)IDIM(I)=1
      END DO

*    Fold
      IF(WEIGHT)THEN
      CALL FOLDBIN_FOLDW(%VAL(XPTR),%VAL(XWPTR),%VAL(DPTR),
     :             %VAL(VPTR),NACTDIM,
     :             IDIM,IDIM(1),IDIM(2),IDIM(3),IDIM(4),IDIM(5),
     :             IDIM(6),IDIM(7),QOK,%VAL(QPTR),PERIOD,
     :             ZEROOFFSET,NBINS,%VAL(WPTR),
     :             %VAL(YBARPTR),%VAL(SUMPTR),%VAL(FNPTR),
     :             %VAL(FDPTR),%VAL(FVPTR),%VAL(FCPTR),
     :             %VAL(FFPTR),%VAL(FQPTR),IERR,CHISQ,NDOF,STATUS)
      ELSE
      CALL FOLDBIN_FOLDU(%VAL(XPTR),%VAL(XWPTR),%VAL(DPTR),
     :             %VAL(VPTR),NACTDIM,
     :             IDIM,IDIM(1),IDIM(2),IDIM(3),IDIM(4),IDIM(5),
     :             IDIM(6),IDIM(7),QOK,%VAL(QPTR),PERIOD,
     :             ZEROOFFSET,NBINS,%VAL(WPTR),
     :             %VAL(YBARPTR),%VAL(SUMPTR),%VAL(FNPTR),
     :             %VAL(FDPTR),%VAL(FVPTR),%VAL(FCPTR),
     :             %VAL(FFPTR),%VAL(FQPTR),IERR,CHISQ,NDOF,STATUS)
      ENDIF
      IF ( IERR .NE. 0 ) THEN
          CALL MSG_SETI( 'IERR', IERR )
          CALL MSG_PRNT( 'Bad phase bin - IERR = ^IERR' )
      END IF
      CALL ARR_REG1R( 0.5/REAL(NBINS), 1.0/REAL(NBINS), NBINS,
     :                                   %VAL(FXPTR), STATUS )

*    Write components to output object
      N=2
      DO WHILE ( N .LE. NACTDIM )
        CALL BDI_COPAXIS(IFID,FFID,OAX(N),N,STATUS )
        N=N+1
      END DO
      CALL BDI_COPTEXT( IFID, FFID, STATUS )
      IF (STATUS .NE. SAI__OK) THEN
         CALL ERR_FLUSH(STATUS)
      END IF
      CALL BDI_PUTAXLABEL(FFID,1,'Phase',STATUS)
      CALL BDI_PUTAXNORM(FFID,1,.TRUE.,STATUS)

*  Copy other info from input to output file
      CALL BDI_COPMORE( IFID, FFID, STATUS )

*  Write auxilliary stuff
      CALL AUI_PUTNI( FFID, 'N_OCCUPANTS', NACTDIM, IDIM, %VAL(FNPTR),
     :                STATUS )
      IDIM(1) = 1
      CALL AUI_PUTNR( FFID, 'CHISQUARED', NACTDIM, IDIM, %VAL(FCPTR),
     :                STATUS )
      CALL AUI_PUTNI( FFID, 'DEG_OF_FREEDOM', NACTDIM, IDIM,
     :                %VAL(FFPTR), STATUS )
      IDIM(1)=NBINS

*    Add history records
      CALL HSI_COPY( IFID, FFID, STATUS )
      CALL HSI_ADD( FFID, VERSION, STATUS )
      HTXT(1) = 'Input {INP}'
      HTXT(2) = 'Folded output {OUT}'
      CALL MSG_SETR( 'PERIOD', PERIOD )
      CALL MSG_SETC( 'UNITS', UNITS )
      CALL MSG_MAKE( 'The data has been folded into a period of '/
     :                          /'^PERIOD ^UNITS', HTXT(3), HLEN )
      CALL MSG_SETD( 'EPOCH', ZEROEPOCH )
      CALL MSG_MAKE('The epoch of phase zero was ^EPOCH', HTXT(4),
     :                                                       HLEN)
      USED = MAXHTXT
      CALL USI_TEXT( 4, HTXT, USED, STATUS )
      CALL HSI_PTXT(FFID,USED,HTXT,STATUS)

*  Output CHISQR value to user if 1D dataset
      IF ( NACTDIM .EQ. 1 ) THEN
        CALL MSG_SETR('CHISQR',CHISQ )
        CALL MSG_SETI('NDOF',NDOF )
        CALL MSG_PRNT( 'Chi squared value of fit to mean = ^CHISQR'/
     :                 /' with ^NDOF degrees of freedom')
      END IF

*  Release datasets
      CALL BDI_RELEASE( IFID, STATUS )
      CALL BDI_RELEASE( FFID, STATUS )

*   Tidy up
 99   CALL AST_CLOSE()
      CALL AST_ERR( STATUS )

      END



*+  FOLDBIN_FOLDW - folds data Y(X) about a period in X
      SUBROUTINE FOLDBIN_FOLDW(X,AXWID,Y,VAR,NACTDIM,IDIM,L1,L2,L3,
     :                    L4,L5,L6,L7,QOK,QUAL,P,X0,NBINS,W,YBAR,
     :                    SUMVAR,IB,BMN,BMNVAR,CHISQ,NDOF,FQUAL,
     :                    IERR,CHI,ND,STATUS)
*    Description :
*
*	Folds the data Y(X) about a period P in X. Data are binned into
*	NBINS phase bins and the mean and standard deviation of Y computed
*	for each bin. X0 corresponds to phase zero. Weighted means used.
*
*    Method :
*     <description of how the subroutine works - for programmer info>
*    Deficiencies :
*     <description of any deficiencies>
*    Bugs :
*     <description of any "bugs" which have not been fixed>
*    Authors :
*     David J. Allan
*    History :
*     12 Aug 83 : FOLDS V.1
*     26 Mar 86 : TIM_FOLD V.1
*     16 Sep 86 : Phases corrected for negative X
*     13 Jan 90 : Recode in FORTRAN 77!!! (BHVAD::DJA)
*      4 Apr 90 : Accept nD datasets (BHVAD::SRD)
*    Type definitions :
      IMPLICIT NONE
*    Global constants :
      INCLUDE 'SAE_PAR'
      INCLUDE 'ADI_PAR'
*
*    Import :
*
      INTEGER            NACTDIM                ! Number of input data dims
      INTEGER            IDIM(ADI__MXDIM)       ! Dimensions of data points
      INTEGER            L1,L2,L3,L4,L5,L6,L7
      INTEGER            L8
      INTEGER            A,B,C,D,E,F,G
      REAL               X(L1)                   ! Independent variable
      REAL               AXWID(L1)
      REAL               Y(L1,L2,L3,L4,L5,L6,L7) ! Dependent variable
      REAL               VAR(L1,L2,L3,L4,L5,L6,L7)! Variance in Y
      LOGICAL            QUAL(L1,L2,L3,L4,L5,L6,L7)! Quality
      LOGICAL            QOK
      INTEGER            NBINS                  ! Number of phase bins
      REAL               P                      ! Folding period
      DOUBLE PRECISION   X0                     ! offset of Epoch of phase zero
      REAL               W(NBINS,L2,L3,L4,L5,L6,L7)! work space
*
*    Export :
*
      REAL               CHI
      INTEGER            IB(NBINS,L2,L3,L4,L5,L6,L7)! Number of points per bin
      INTEGER            IERR                   ! 0=O.K. 1=less than 2 points
                                                ! in some bins. For 1 point
                                                ! BMNVAR is zero, for 0 BMN too.
      REAL               BMN(NBINS,L2,L3,L4,L5,L6,L7)! Mean Y value weighted
      REAL               BMNVAR(NBINS,L2,L3,L4,L5,L6,L7)! Standard deviation per bin
      REAL               CHISQ(1,L2,L3,L4,L5,L6,L7)! Chi-squared fit of means to
                                                ! constants intensity.
      INTEGER            NDOF(1,L2,L3,L4,L5,L6,L7)! Number of degrees of freedom
                                                ! of CHISQ - NBINS-1 unless
                                                ! some bins contain <2 points
      BYTE               FQUAL(NBINS,L2,L3,L4,L5,L6,L7)
      INTEGER            ND
*    Status :
      INTEGER            STATUS
*
*    Local variables
*
      INTEGER            NB,NUM
      REAL               DVAR, YBAR(1,L2,L3,L4,L5,L6,L7), PHA
      REAL               SUMVAR(1,L2,L3,L4,L5,L6,L7)
*-

*    Check status
      IF ( STATUS .NE. SAI__OK ) RETURN

*    Initialise
      L8=NBINS
      NUM=NBINS*L2*L3*L4*L5*L6*L7
      CALL ARR_INIT1I(0,NUM,IB,STATUS)
      CALL ARR_INIT1R(0.0,NUM,BMN,STATUS)
      CALL ARR_INIT1R(0.0,NUM,BMNVAR,STATUS)
      CALL ARR_INIT1R(0.0,NUM,W,STATUS)
      IERR=0

*    Bin up data
      DO A=1,L1
        PHA=REAL(DMOD((DBLE(X(A))-X0)/DBLE(P),1.D0))
        IF ( PHA .LT. 0.0 ) PHA = PHA + 1.0
        NB = INT( PHA*NBINS ) + 1
        DO B=1,L2
          DO C=1,L3
            DO D=1,L4
              DO E=1,L5
                DO F=1,L6
                  DO G=1,L7
                    IF(.NOT.QOK.OR.(QOK.AND.QUAL(A,B,C,D,E,F,G)))THEN
                      IB(NB,B,C,D,E,F,G) = IB(NB,B,C,D,E,F,G) + 1
                      IF ( VAR(A,B,C,D,E,F,G).LE.0.0 ) THEN
                        DVAR=1/(AXWID(A)*AXWID(A))
                      ELSE
                        DVAR=VAR(A,B,C,D,E,F,G)
	              END IF
                      BMN(NB,B,C,D,E,F,G)=BMN(NB,B,C,D,E,F,G)+
     :                                       Y(A,B,C,D,E,F,G)/DVAR
                      W(NB,B,C,D,E,F,G)=W(NB,B,C,D,E,F,G)+1.0/DVAR
                    ENDIF
                  END DO
                END DO
              END DO
            END DO
          END DO
        END DO
      END DO
*    Analyse phase bins
      DO B=1,L2
        DO C=1,L3
          DO D=1,L4
            DO E=1,L5
              DO F=1,L6
                DO G=1,L7
                  YBAR(1,B,C,D,E,F,G)=0.0
                  SUMVAR(1,B,C,D,E,F,G)=0.0
                  NDOF(1,B,C,D,E,F,G)=NBINS-1
                  DO A=1,NBINS
                    IF ( IB(A,B,C,D,E,F,G) .LT. 2 )THEN
                      IERR=1
                      FQUAL(A,B,C,D,E,F,G)=1
	              NDOF(1,B,C,D,E,F,G)=NDOF(1,B,C,D,E,F,G)-1
                    ELSE
                      BMN(A,B,C,D,E,F,G)=BMN(A,B,C,D,E,F,G)/
     :                                            W(A,B,C,D,E,F,G)
                      BMNVAR(A,B,C,D,E,F,G) = 1.0/W(A,B,C,D,E,F,G)
                      YBAR(1,B,C,D,E,F,G)=YBAR(1,B,C,D,E,F,G)+
     :                      BMN(A,B,C,D,E,F,G)/BMNVAR(A,B,C,D,E,F,G)
                      SUMVAR(1,B,C,D,E,F,G)=SUMVAR(1,B,C,D,E,F,G)+
     :                                     1.0/BMNVAR(A,B,C,D,E,F,G)
	            END IF
                  END DO
                  YBAR(1,B,C,D,E,F,G)=YBAR(1,B,C,D,E,F,G)/
     :                                          SUMVAR(1,B,C,D,E,F,G)
                END DO
              END DO
            END DO
          END DO
        END DO
      END DO

*    Find CHISQ
      DO B=1,L2
        DO C=1,L3
          DO D=1,L4
            DO E=1,L5
              DO F=1,L6
                DO G=1,L7
                  CHISQ(1,B,C,D,E,F,G)=0.0
                  DO A=1,NBINS
                  IF ( IB(A,B,C,D,E,F,G) .GE. 2 )THEN
                  CHISQ(1,B,C,D,E,F,G)=CHISQ(1,B,C,D,E,F,G)
     :            +((BMN(A,B,C,D,E,F,G)-YBAR(1,B,C,D,E,F,G))**2
     :            /BMNVAR(A,B,C,D,E,F,G))
                  END IF
                  END DO
                END DO
              END DO
            END DO
          END DO
        END DO
      END DO
*
      IF (NACTDIM.EQ.1) THEN
         CHI=CHISQ(1,1,1,1,1,1,1)
         ND = NDOF(1,1,1,1,1,1,1)
      ENDIF
*
      END



*+  FOLDBIN_FOLDU - folds data Y(X) about a period in X
      SUBROUTINE FOLDBIN_FOLDU(X,AXWID,Y,VAR,NACTDIM,IDIM,L1,L2,L3,
     :                    L4,L5,L6,L7,QOK,QUAL,P,X0,NBINS,W,YBAR,
     :                    SUMVAR,IB,BINAV,VARB,CHISQ,NDOF,FQUAL,
     :                    IERR,CHI,ND,STATUS)
*    Description :
*
*	Folds the data Y(X) about a period P in X. Data are binned into
*	NBINS phase bins and the mean and standard deviation of Y computed
*	for each bin. X0 corresponds to phase zero. Non-weighted means used.
*
*    Method :
*     <description of how the subroutine works - for programmer info>
*    Deficiencies :
*     <description of any deficiencies>
*    Bugs :
*     <description of any "bugs" which have not been fixed>
*    Authors :
*     David J. Allan
*    History :
*     12 Aug 83 : FOLDS V.1
*     26 Mar 86 : TIM_FOLD V.1
*     16 Sep 86 : Phases corrected for negative X
*     13 Jan 90 : Recode in FORTRAN 77!!! (BHVAD::DJA)
*      4 Apr 90 : Accept nD datasets (BHVAD::SRD)
*     23 Jul 90 : array initialisation bugs fixed (SOTON::AJN)
*     24 Jul 90 : epoch of phase zero bug fixed (SOTON::AJN)
*    Type definitions :
      IMPLICIT NONE
*    Global constants :
      INCLUDE 'SAE_PAR'
      INCLUDE 'ADI_PAR'
*
*    Import :
*
      INTEGER            NACTDIM                ! Number of input data dims
      INTEGER            IDIM(ADI__MXDIM)       ! Dimensions of data points
      INTEGER            L1,L2,L3,L4,L5,L6,L7
      INTEGER            L8
      INTEGER            A,B,C,D,E,F,G
      REAL               X(L1)                   ! Independent variable
      REAL               AXWID(L1)
      REAL               Y(L1,L2,L3,L4,L5,L6,L7) ! Dependent variable
      REAL               VAR(L1,L2,L3,L4,L5,L6,L7)! Variance in Y
      LOGICAL            QUAL(L1,L2,L3,L4,L5,L6,L7)! Quality
      LOGICAL            QOK
      INTEGER            NBINS                  ! Number of phase bins
      REAL               P                      ! Folding period
      DOUBLE PRECISION   X0                     ! offset of Epoch of phase zero
      REAL               W(NBINS,L2,L3,L4,L5,L6,L7)! work space
      REAL               SUMVAR(1,L2,L3,L4,L5,L6,L7)
*
*    Export :
*
      REAL               CHI
      INTEGER            IB(NBINS,L2,L3,L4,L5,L6,L7)! Number of points per bin
      INTEGER            IERR                   ! 0=O.K. 1=less than 2 points
                                                ! in some bins. For 1 point
                                                ! BMNVAR is zero, for 0 BMN too.
      REAL               BINAV(NBINS,L2,L3,L4,L5,L6,L7)! Mean Y value weighted
      REAL               VARB(NBINS,L2,L3,L4,L5,L6,L7)
      REAL               CHISQ(1,L2,L3,L4,L5,L6,L7)! Chi-squared fit of means to
                                                ! constants intensity.
      INTEGER            NDOF(1,L2,L3,L4,L5,L6,L7)! Number of degrees of freedom
                                                ! of CHISQ - NBINS-1 unless
                                                ! some bins contain <2 points
      BYTE FQUAL(NBINS,L2,L3,L4,L5,L6,L7)
      INTEGER ND
*    Status :
      INTEGER            STATUS
*
*    Local variables
*
      INTEGER            NB,NUM,NUM1,NBAD
      REAL               YBAR(1,L2,L3,L4,L5,L6,L7), PHA
      LOGICAL LMESS
*-

*    Check status
      IF ( STATUS .NE. SAI__OK ) RETURN

*    Initialise
      L8=NBINS
      NUM=NBINS*L2*L3*L4*L5*L6*L7
      NUM1=1*L2*L3*L4*L5*L6*L7
      CALL ARR_INIT1I(0,NUM,IB,STATUS)
      CALL ARR_INIT1R(0.0,NUM,W,STATUS)
      CALL ARR_INIT1R(0.0,NUM,VARB,STATUS)
      CALL ARR_INIT1R(0.0,NUM1,YBAR,STATUS)
      IERR=0

*    Bin up data
      DO A=1,L1
        PHA=REAL(DMOD((DBLE(X(A))-X0)/DBLE(P),1.D0))
*       PHA=AMOD((X(A)-X0)/P,1.0)
        IF ( PHA .LT. 0.0 ) PHA = PHA + 1.0
        NB = INT( PHA*NBINS ) + 1
        DO B=1,L2
          DO C=1,L3
            DO D=1,L4
              DO E=1,L5
                DO F=1,L6
                  DO G=1,L7
                    IF(.NOT.QOK.OR.(QOK.AND.QUAL(A,B,C,D,E,F,G)))THEN
                      IB(NB,B,C,D,E,F,G) = IB(NB,B,C,D,E,F,G) + 1
                      W(NB,B,C,D,E,F,G)=W(NB,B,C,D,E,F,G)+
     :                                   Y(A,B,C,D,E,F,G)
                    ENDIF
                  END DO
                END DO
              END DO
            END DO
          END DO
        END DO
      END DO


      DO NB=1,NBINS
         DO B=1,L2
           DO C=1,L3
             DO D=1,L4
               DO E=1,L5
                 DO F=1,L6
                   DO G=1,L7
                     IF(IB(NB,B,C,D,E,F,G).LT.2)THEN
                       IERR=1
                     ELSE
                       BINAV(NB,B,C,D,E,F,G)=W(NB,B,C,D,E,F,G)/
     :                                     IB(NB,B,C,D,E,F,G)
                     ENDIF
                     NDOF(1,B,C,D,E,F,G)=NBINS-1
                   END DO
                 END DO
              END DO
            END DO
          END DO
        END DO
      END DO
      DO A=1,L1
        PHA=REAL(DMOD((DBLE(X(A))-X0)/DBLE(P),1.D0))
*       PHA=AMOD((X(A)-X0)/P,1.0)
        IF ( PHA .LT. 0.0 ) PHA = PHA + 1.0
        NB = INT( PHA*NBINS ) + 1
        DO B=1,L2
          DO C=1,L3
            DO D=1,L4
              DO E=1,L5
                DO F=1,L6
                  DO G=1,L7
                    IF(.NOT.QOK.OR.(QOK.AND.QUAL(A,B,C,D,E,F,G)))THEN
                      IF(IB(NB,B,C,D,E,F,G).GE.2)THEN
                       VARB(NB,B,C,D,E,F,G)=VARB(NB,B,C,D,E,F,G)+
     :                 ((Y(A,B,C,D,E,F,G)-BINAV(NB,B,C,D,E,F,G))**2)/
     :                 (IB(NB,B,C,D,E,F,G)*(IB(NB,B,C,D,E,F,G)-1))
                      ENDIF
                    ENDIF
                  END DO
                END DO
              END DO
            END DO
          END DO
        END DO
      END DO

      DO B=1,L2
        DO C=1,L3
          DO D=1,L4
            DO E=1,L5
              DO F=1,L6
                DO G=1,L7
                  NBAD = 0
                  DO A=1,NBINS
                    IF(IB(A,B,C,D,E,F,G).LT.2)THEN
                      IERR=1
                      NDOF(1,B,C,D,E,F,G)=NDOF(1,B,C,D,E,F,G)-1
                      FQUAL(A,B,C,D,E,F,G)=1
                      NBAD = NBAD + 1
                    ELSE
                      YBAR(1,B,C,D,E,F,G)=YBAR(1,B,C,D,E,F,G)+
     :                          BINAV(A,B,C,D,E,F,G)
                    ENDIF
                  ENDDO
*
                  IF ( (NBINS - NBAD) .GT. 0 ) THEN
                    YBAR(1,B,C,D,E,F,G)=YBAR(1,B,C,D,E,F,G)/(NBINS-NBAD)
                  ENDIF
*
                END DO
              END DO
            END DO
          END DO
        END DO
      END DO

      DO B=1,L2
        DO C=1,L3
          DO D=1,L4
            DO E=1,L5
              DO F=1,L6
                DO G=1,L7
                  CHISQ(1,B,C,D,E,F,G)=0.0
                  DO A=1,NBINS
                    IF(IB(A,B,C,D,E,F,G).GT.2)THEN
                      IF (VARB(A,B,C,D,E,F,G) .GT. 0.0) THEN
                        CHISQ(1,B,C,D,E,F,G)=CHISQ(1,B,C,D,E,F,G)
     :                  +(((BINAV(A,B,C,D,E,F,G)-
     :                  YBAR(1,B,C,D,E,F,G))**2)/VARB(A,B,C,D,E,F,G))
                      ELSE
*
*         If variances are zero output message once
                        IF (.NOT. LMESS) THEN
                           CALL MSG_PRNT('** VARB is zero - CHISQ may'/
     &                           /' not be correct in output file ** ')
*
                           LMESS = .TRUE.
                         ENDIF
*
                      ENDIF
                    ENDIF
                  ENDDO
                END DO
              END DO
            END DO
          END DO
        END DO
      END DO
*
      IF (NACTDIM.EQ.1) THEN
         CHI=CHISQ(1,1,1,1,1,1,1)
         ND=NDOF(1,1,1,1,1,1,1)
      ENDIF
*
      END
