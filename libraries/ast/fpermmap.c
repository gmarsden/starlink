/*
*+
*  Name:
*     fpermmap.c

*  Purpose:
*     Define a FORTRAN 77 interface to the AST PermMap class.

*  Type of Module:
*     C source file.

*  Description:
*     This file defines FORTRAN 77-callable C functions which provide
*     a public FORTRAN 77 interface to the PermMap class.

*  Routines Defined:
*     AST_ISAPERMMAP
*     AST_PERMMAP

*  Copyright:
*     <COPYRIGHT_STATEMENT>

*  Authors:
*     RFWS: R.F. Warren-Smith (Starlink)

*  History:
*     26-SEP-1996 (RFWS):
*        Original version.
*/

/* Define the astFORTRAN77 macro which prevents error messages from
   AST C functions from reporting the file and line number where the
   error occurred (since these would refer to this file, they would
   not be useful). */
#define astFORTRAN77

/* Header files. */
/* ============= */
#include "f77.h"                 /* FORTRAN <-> C interface macros (SUN/209) */
#include "c2f77.h"               /* F77 <-> C support functions/macros */
#include "error.h"               /* Error reporting facilities */
#include "memory.h"              /* Memory handling facilities */
#include "permmap.h"             /* C interface to the PermMap class */

F77_LOGICAL_FUNCTION(ast_isapermmap)( INTEGER(THIS),
                                      INTEGER(STATUS) ) {
   GENPTR_INTEGER(THIS)
   F77_LOGICAL_TYPE(RESULT);

   astAt( "AST_ISAPERMMAP", NULL, 0 );
   astWatchSTATUS(
      RESULT = astIsAPermMap( astI2P( *THIS ) ) ? F77_TRUE : F77_FALSE;
   )
   return RESULT;
}

F77_INTEGER_FUNCTION(ast_permmap)( INTEGER(NIN),
                                   INTEGER_ARRAY(INPERM),
                                   INTEGER(NOUT),
                                   INTEGER_ARRAY(OUTPERM),
                                   DOUBLE_ARRAY(CONSTANT),
                                   CHARACTER(OPTIONS),
                                   INTEGER(STATUS)
                                   TRAIL(OPTIONS) ) {
   GENPTR_INTEGER(NIN)
   GENPTR_INTEGER_ARRAY(INPERM)
   GENPTR_INTEGER(NOUT)
   GENPTR_INTEGER_ARRAY(OUTPERM)
   GENPTR_DOUBLE_ARRAY(CONSTANT)
   GENPTR_CHARACTER(OPTIONS)
   F77_INTEGER_TYPE(RESULT);
   int i;
   char *options;

   astAt( "AST_PERMMAP", NULL, 0 );
   astWatchSTATUS(
      options = astString( OPTIONS, OPTIONS_length );

/* Change ',' to '\n' (see AST_SET in fobject.c for why). */
      if ( astOK ) {
         for ( i = 0; options[ i ]; i++ ) {
            if ( options[ i ] == ',' ) options[ i ] = '\n';
         }
      }
      RESULT = astP2I( astPermMap( *NIN, INPERM, *NOUT, OUTPERM, CONSTANT,
                                   "%s", options ) );
      astFree( options );
   )
   return RESULT;
}
