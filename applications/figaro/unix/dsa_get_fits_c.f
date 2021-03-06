*+
*   D S A _ G E T _ F I T S _ C
*
      subroutine dsa_get_fits_c(refnam,item,el,val,comment,status)
*
*  Description:
*
*
*  Arguments:
*      REFNAM = CHARACTER*4 (Given)
*        ref name
*      ITEM = CHARACTER*8 (Given)
*        item
*      EL = INTEGER (Given)
*        element
*      STATUS = INTEGER (Given and returned)
*        status
*      VAL = CHARACTER*(*) (Returned)
*        value
*      COMMENT = CHARACTER*(*) (Returned)
*        comment
*  Subroutines/functions referenced:
*
*  Author:
*    T.N.Wilkins, Cambridge, 25-JUL-1991
*    A.J.Holloway, Manchester, 4-Dec-1997
*  History:
*    AJH Set refnam to be 4 characters, item 8 characters.
*    AJH Removed use of dta_rdvarc
*-
      implicit none
      include 'SAE_PAR'
      include 'DAT_PAR'
      include 'DAT_ERR'
      include 'CMP_ERR'
      CHARACTER*(DAT__SZLOC) NLOC, MLOC
      character*4 refnam
      character*8 item
      integer el
      character*(*) comment,val
      integer status,i,ilen

*

      call dta_szvar(refnam//'.fits.'//item,1,i,ilen,status)

*      call dta_rdvarc(refnam//'.fits.'//item,ilen,val,status)

      call dta_loc(refnam//'.fits.'//item,nloc,status)
      call dat_get0c(nloc,val,status)

      call dta_annul(nloc,status)

      call dta_szvar(refnam//'.comments.'//item,1,i,ilen,status)
      if(status.eq.0) then
*     call dta_rdvarc(refnam//'.comments.'//item,ilen,
*     :     comment,status)

      call dta_loc(refnam//'.comments.'//item,mloc,status)
      call dat_get0c(mloc,comment,status)

      call dta_annul(mloc,status)


      else
        status = 0
      end if
      end
