*
*  Description from GAMS:
*
*  Standard normal generator with zero mean and unit standard deviation. Uses
*  ziggaraut algorithm. Fast, excellent statistical properties and portable.
*
*  Classes:   L6a14. Negative binomial and normal random numbers, random normal
*                  order statistics
*
*  Type:      Fortran software in NMS collection.
*  Access:    Public domain. Portable.
*  Precision: Double.
*
*  Usage:     R = DRNOR()

      DOUBLE PRECISION FUNCTION PDA_DRANN()
C***BEGIN PROLOGUE  PDA_DRANN
C***DATE WRITTEN   810915 (YYMMDD)
C***REVISION DATE  870419 (YYMMDD)
C***CATEGORY NO.  L6A14
C***KEYWORDS  RANDOM NUMBERS, NORMAL DEVIATES
C***AUTHOR    KAHANER, DAVID, SCIENTIFIC COMPUTING DIVISION, NBS
C             MARSAGLIA, GEORGE, SUPERCOMPUTER RES. INST., FLORIDA ST. U.
C
C***PURPOSE  GENERATES NORMAL RANDOM NUMBERS, WITH MEAN ZERO AND
C             UNIT STANDARD DEVIATION, OFTEN DENOTED N(0,1).
C
C***DESCRIPTION
C
C       DRNOR GENERATES NORMAL RANDOM NUMBERS WITH ZERO MEAN AND
C       UNIT STANDARD DEVIATION, OFTEN DENOTED N(0,1).
C           FROM THE BOOK, "NUMERICAL METHODS AND SOFTWARE" BY
C                D. KAHANER, C. MOLER, S. NASH
C                PRENTICE HALL, 1988
C   USE
C       FIRST TIME....
C                   Z = PDA_DRANS(ISEED)
C                     HERE ISEED IS ANY  N O N - Z E R O  INTEGER.
C                     THIS CAUSES INITIALIZATION OF THE PROGRAM.
C                     PDA_DRANS RETURNS A DOUBLE PRECISION ECHO OF ISEED.
C
C       SUBSEQUENT TIMES...
C                   Z = PDA_DRANN()
C                     CAUSES THE NEXT DOUBLE PRECISION RANDOM NUMBER
C                           TO BE RETURNED AS Z.
C
C.....................................................................
C                 TYPICAL USAGE
C
C                    DOUBLE PRECISION PDA_DRANS,PDA_DRANN,Z
C                    INTEGER ISEED,I
C                    ISEED = 305
C                    Z = PDA_DRANS(ISEED)
C                    DO 1 I = 1,10
C                       Z = PDA_DRANN()
C                       WRITE(*,'(1X,D20.15)') Z
C                 1  CONTINUE
C                    END
C
C
C***REFERENCES  MARSAGLIA & TSANG, "A FAST, EASILY IMPLEMENTED
C                 METHOD FOR SAMPLING FROM DECREASING OR
C                 SYMMETRIC UNIMODAL DENSITY FUNCTIONS",
C                 PUBLISHED IN SIAM J SISC, JUNE 1984.
C***ROUTINES CALLED  (NONE)
C***END PROLOGUE  PDA_DRANN
      DOUBLE PRECISION AA,B,C,C1,C2,PC,X,Y,XN,V(65),PDA_DRANS,U(17),
     *S,T,UN,VNI
      INTEGER J,IA,IB,IC,II,JJ,ID,III,JJJ,L
      SAVE U,II,JJ
C
      DATA AA,B,C/0.123758602991705622657860318807D+02
     +,0.487899177760378968003825536710D+00
     +,0.126770580788654778410032042685D+02/
      DATA C1,C2,PC,XN/0.9689279D0,1.301198D0
     +,0.195830333955546914251231662871D-01
     +,0.277699426966287548981739308903D+01/
      DATA (V(L), L=1,18)
     +/0.340945028703977890881422029377D+00
     +,0.457314591866933536170242641762D+00
     +,0.539779281611666939596931167481D+00
     +,0.606242679653048904661174385559D+00
     +,0.663169062764524854743428299352D+00
     +,0.713697459056025891183276150202D+00
     +,0.759612474933920604605610034675D+00
     +,0.802035600355531312751497342081D+00
     +,0.841722667978955421276418428136D+00
     +,0.879210223208313639290346470191D+00
     +,0.914894804386750587541168254518D+00
     +,0.949079113753090250631877133376D+00
     +,0.982000481239888200218207508382D+00
     +,0.101384923802994173461911276018D+01
     +,0.104478103674017351825847605485D+01
     +,0.107492538202855351339149779813D+01
     +,0.110439170226812581109973656162D+01
     +,0.113327377624394079212251428682D+01/
      DATA (V(L), L=19,37)
     +/0.116165303013393172842858957666D+01
     +,0.118960104083873798956793871425D+01
     +,0.121718147070087121538258873613D+01
     +,0.124445158789824683238161436879D+01
     +,0.127146348057211969375402099579D+01
     +,0.129826504188319751190626947962D+01
     +,0.132490078218086109537654808436D+01
     +,0.135141250993337129690631764473D+01
     +,0.137783991287001181384096757263D+01
     +,0.140422106355997540689484486002D+01
     +,0.143059286850269131403410180874D+01
     +,0.145699147613767157824869156623D+01
     +,0.148345265660321931119703498108D+01
     +,0.151001216431851991531882612256D+01
     +,0.153670609335952099134607533122D+01
     +,0.156357123503769104042967185962D+01
     +,0.159064544701425352365935513885D+01
     +,0.161796804367444698360816323707D+01
     +,0.164558021836908161542865488149D+01/
      DATA (V(L), L=38,55)
     +/0.167352550956703867146009214486D+01
     +,0.170185032506274055254533570699D+01
     +,0.173060454131778319060975251429D+01
     +,0.175984219903830120010946138955D+01
     +,0.178962232156657450014351836107D+01
     +,0.182000989013069176863519209140D+01
     +,0.185107702023027589942628767312D+01
     +,0.188290439759287281399927405628D+01
     +,0.191558305194303202395065401364D+01
     +,0.194921657491636060191700129156D+01
     +,0.198392392890568577258506733664D+01
     +,0.201984305290623555306662745946D+01
     +,0.205713555999009616804474181513D+01
     +,0.209599295624939161758467989658D+01
     +,0.213664502254438986524966622832D+01
     +,0.217937134039813565892460111431D+01
     +,0.222451750721601784110056845259D+01
     +,0.227251855485014779874266158018D+01/
      DATA (V(L), L=56,65)
     +/0.232393382009430256940425938218D+01
     +,0.237950077408282829688673722776D+01
     +,0.244022179797994340264326423618D+01
     +,0.250751170186531701106382130475D+01
     +,0.258346583522542956831304962942D+01
     +,0.267139159032083601869533973173D+01
     +,0.277699426966286466722522163764D+01
     +,0.277699426966286466722522163764D+01
     +,0.277699426966286466722522163764D+01
     +,0.277699426966286466722522163764D+01/
C      LOAD DATA ARRAY IN CASE USER FORGETS TO INITIALIZE.
C      THIS ARRAY IS THE RESULT OF CALLING DUNI 100000 TIMES
C         WITH SEED 305.
      DATA U/
     *0.47196 09815 77884 75583 77897 24978D+00,
     *0.93032 34532 05669 57843 36396 32431D+00,
     *0.11016 17909 33730 83658 71279 44899D+00,
     *0.57150 19962 73139 51836 26387 57010D-01,
     *0.40246 75547 79738 26623 75385 03137D+00,
     *0.45118 19534 27459 48945 82794 56915D+00,
     *0.29607 61523 42721 10217 41299 54053D+00,
     *0.12820 21893 25888 11646 68796 22359D-01,
     *0.31427 46938 50973 60398 08532 59266D+00,
     *0.33552 13667 52294 93246 81635 94171D-02,
     *0.48868 50452 00439 37160 78503 67840D+00,
     *0.19547 04268 65656 75869 38606 13516D+00,
     *0.86416 27067 91773 55690 15993 26053D+00,
     *0.33550 59558 15259 20359 63811 70316D+00,
     *0.37719 02001 99058 08546 95264 70541D+00,
     *0.40078 03921 14818 31467 16765 25916D+00,
     *0.37422 42141 82207 46626 27503 07281D+00/
C
      DATA II,JJ / 17, 5 /
C
C***FIRST EXECUTABLE STATEMENT  PDA_DRANN
C
C FAST PART...
C
C
C   BASIC GENERATOR IS FIBONACCI
C
      UN = U(II)-U(JJ)
      IF(UN.LT.0.0D0) UN = UN+1.
      U(II) = UN
C           U(II) AND UN ARE UNIFORM ON [0,1)
C           VNI IS UNIFORM ON [-1,1)
      VNI = UN + UN -1.
      II = II-1
      IF(II.EQ.0)II = 17
      JJ = JJ-1
      IF(JJ.EQ.0)JJ = 17
C        INT(UN(II)*128) IN RANGE [0,127],  J IS IN RANGE [1,64]
      J = MOD(INT(U(II)*128),64)+1
C        PICK SIGN AS VNI IS POSITIVE OR NEGATIVE
      PDA_DRANN = VNI*V(J+1)
      IF(ABS(PDA_DRANN).LE.V(J))RETURN
C
C SLOW PART; AA IS A*F(0)
      X = (ABS(PDA_DRANN)-V(J))/(V(J+1)-V(J))
C          Y IS UNIFORM ON [0,1)
      Y = U(II)-U(JJ)
      IF(Y.LT.0.0D0) Y = Y+1.
      U(II) = Y
      II = II-1
      IF(II.EQ.0)II = 17
      JJ = JJ-1
      IF(JJ.EQ.0)JJ = 17
C
      S = X+Y
      IF(S.GT.C2)GO TO 11
      IF(S.LE.C1)RETURN
      IF(Y.GT.C-AA*EXP(-.5D0*(B-B*X)**2))GO TO 11
      IF(EXP(-.5D0*V(J+1)**2)+Y*PC/V(J+1).LE.EXP(-.5D0*PDA_DRANN**2))
     *RETURN
C
C TAIL PART; .36010157... IS 1.0D0/XN
C       Y IS UNIFORM ON [0,1)
   22 Y = U(II)-U(JJ)
      IF(Y.LE.0.0D0) Y = Y+1.
      U(II) = Y
      II = II-1
      IF(II.EQ.0)II = 17
      JJ = JJ-1
      IF(JJ.EQ.0)JJ = 17
C
      X = 0.360101571301190680192994239651D+00*LOG(Y)
C       Y IS UNIFORM ON [0,1)
      Y = U(II)-U(JJ)
      IF(Y.LE.0.0D0) Y = Y+1.
      U(II) = Y
      II = II-1
      IF(II.EQ.0)II = 17
      JJ = JJ-1
      IF(JJ.EQ.0)JJ = 17
      IF( -2.0D0*LOG(Y).LE.X**2 )GO TO 22
      PDA_DRANN = SIGN(XN-X,PDA_DRANN)
      RETURN
   11 PDA_DRANN = SIGN(B-B*X,PDA_DRANN)
      RETURN
C
C
C  FILL
      ENTRY PDA_DRANS(ISEED)
      IF(ISEED.NE.0) THEN
C
C          SET UP ...
C              GENERATE RANDOM BIT PATTERN IN ARRAY BASED ON GIVEN SEED
C
        II = 17
        JJ = 5
        IA = MOD(ABS(ISEED),32707)
        IB = 1111
        IC = 1947
        DO 2 III = 1,17
          S = 0.0D0
          T = 0.5D0
C             DO FOR EACH OF THE BITS OF MANTISSA OF WORD
C             LOOP  OVER 95 BITS, ENOUGH FOR MOST MACHINES
C                   IN DOUBLE PRECISION.
          DO 3 JJJ = 1,95
                  ID = IC-IA
                  IF(ID.GE.0)GOTO 4
                  ID = ID+32707
                  S = S+T
    4             IA = IB
                  IB = IC
                  IC = ID
    3     T = 0.5D0*T
    2     U(III) = S
      ENDIF
C       RETURN FLOATING ECHO OF ISEED
      PDA_DRANS=ISEED
      RETURN
      END

