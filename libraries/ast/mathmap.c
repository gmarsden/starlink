/* To do:
      o Allow intermediate expressions.
      o Prototype user docs for expression syntax.
      o Write Fortran interface.
      o Tidy .h file.
      o Write user documentation.
      o Sort out how to implement x ? a : b efficiently (using a mask stack
        maybe?).
*/

/* Module Macros. */
/* ============== */
/* Set the name of the class we are implementing. This indicates to
   the header files that define class interfaces that they should make
   "protected" symbols available. */
#define astCLASS MathMap

/* This macro allocates an array of pointers. If successful, each element
   of the array is initialised to NULL. */
#define MALLOC_POINTER_ARRAY(array_name,array_type,array_size) \
\
/* Allocate the array. */ \
   (array_name) = astMalloc( sizeof(array_type) * (size_t) (array_size) ); \
   if ( astOK ) { \
\
/* If successful, loop to initialise each element. */ \
      int array_index_; \
      for ( array_index_ = 0; array_index_ < (array_size); array_index_++ ) { \
         (array_name)[ array_index_ ] = NULL; \
      } \
   }

/* This macro frees a dynamically allocated array of pointers, each of
   whose elements may point at a further dynamically allocated array
   (which is also to be freed). It also allows for the possibility of any
   of the pointers being NULL. */
#define FREE_POINTER_ARRAY(array_name,array_size) \
\
/* Check thet the main array pointer is not NULL. */ \
   if ( (array_name) ) { \
\
/* If OK, loop to free each of the sub-arrays. */ \
      int array_index_; \
      for ( array_index_ = 0; array_index_ < (array_size); array_index_++ ) { \
\
/* Check that each sub-array pointer is not NULL before freeing it. */ \
         if ( (array_name)[ array_index_ ] ) { \
            (array_name)[ array_index_ ] = \
               astFree( (array_name)[ array_index_ ] ); \
         } \
      } \
\
/* Free the main pointer array. */ \
      (array_name) = astFree( (array_name) ); \
   }

/* Header files. */
/* ============= */
/* Interface definitions. */
/* ---------------------- */
#include "channel.h"             /* I/O channels */
#include "error.h"               /* Error reporting facilities */
#include "mapping.h"             /* Coordinate mappings (parent class) */
#include "mathmap.h"             /* Interface definition for this class */
#include "memory.h"              /* Memory allocation facilities */
#include "object.h"              /* Base Object class */
#include "pointset.h"            /* Sets of points */
#include "unitmap.h"             /* Unit Mapping */

/* Error code definitions. */
/* ----------------------- */
#include "ast_err.h"             /* AST error codes */

/* C header files. */
/* --------------- */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Module Variables. */
/* ================= */

typedef AstMathMapRandContext_ Rcontext;

/* Define the class virtual function table and its initialisation flag
   as static variables. */
static AstMathMapVtab class_vtab; /* Virtual function table */
static int class_init = 0;       /* Virtual function table initialised? */

/* Pointers to parent class methods which are extended by this class. */
static AstPointSet *(* parent_transform)( AstMapping *, AstPointSet *, int, AstPointSet * );
static const char *(* parent_getattrib)( AstObject *, const char * );
static int (* parent_testattrib)( AstObject *, const char * );
static void (* parent_clearattrib)( AstObject *, const char * );
static void (* parent_setattrib)( AstObject *, const char * );

/* This declaration enumerates the operation codes recognised by the
   virtual machine which evaluates arithmetic expressions. */
typedef enum {

/* User-supplied constants and variables. */
   OP_LDCON,                     /* Load constant */
   OP_LDVAR,                     /* Load variable */

/* System constants. */
   OP_LDBAD,                     /* Load bad value (AST__BAD) */
   OP_LDDIG,                     /* Load # decimal digits (DBL_DIG) */
   OP_LDEPS,                     /* Load relative precision (DBL_EPSILON) */
   OP_LDMAX,                     /* Load largest value (DBL_MAX) */
   OP_LDMAX10E,                  /* Max. decimal exponent (DBL_MAX_10_EXP) */
   OP_LDMAXE,                    /* Load maximum exponent (DBL_MAX_EXP) */
   OP_LDMDIG,                    /* Load # mantissa digits (DBL_MANT_DIG) */
   OP_LDMIN,                     /* Load smallest value (DBL_MIN) */
   OP_LDMIN10E,                  /* Min. decimal exponent (DBL_MIN_10_EXP) */
   OP_LDMINE,                    /* Load minimum exponent (DBL_MIN_EXP) */
   OP_LDRAD,                     /* Load floating radix (FLT_RADIX) */
   OP_LDRND,                     /* Load rounding mode (FLT_ROUNDS) */

/* Mathematical constants. */
   OP_LDE,                       /* Load e (base of natural logarithms) */
   OP_LDPI,                      /* Load pi */

/* Functions with one argument. */
   OP_ABS,                       /* Absolute value (sign removal) */
   OP_ACOS,                      /* Inverse cosine (radians) */
   OP_ACOSD,                     /* Inverse cosine (degrees) */
   OP_ASIN,                      /* Inverse sine (radians) */
   OP_ASIND,                     /* Inverse sine (degrees) */
   OP_ATAN,                      /* Inverse tangent (radians) */
   OP_ATAND,                     /* Inverse tangent (degrees) */
   OP_CEIL,                      /* C ceil function (round up) */
   OP_COS,                       /* Cosine (radians) */
   OP_COSD,                      /* Cosine (degrees) */
   OP_COSH,                      /* Hyperbolic cosine */
   OP_EXP,                       /* Exponential function */
   OP_EXP2,                      /* Exponent of 2 from C frexp function */
   OP_FLOOR,                     /* C floor function (round down) */
   OP_FR2,                       /* Normalised fraction from C frexp fn */
   OP_INT,                       /* Integer value (round towards zero) */
   OP_ISBAD,                     /* Test for bad value */
   OP_LOG,                       /* Natural logarithm */
   OP_LOG10,                     /* Base 10 logarithm */
   OP_NINT,                      /* Fortran NINT function (round to nearest) */
   OP_POISS,                     /* Poisson random number */
   OP_SIN,                       /* Sine (radians) */
   OP_SIND,                      /* Sine (degrees) */
   OP_SINH,                      /* Hyperbolic sine */
   OP_SQRT,                      /* Square root */
   OP_TAN,                       /* Tangent (radians) */
   OP_TAND,                      /* Tangent (degrees) */
   OP_TANH,                      /* Hyperbolic tangent */

/* Functions with two arguments. */
   OP_ATAN2,                     /* Inverse tangent (2 arguments, radians) */
   OP_ATAN2D,                    /* Inverse tangent (2 arguments, degrees) */
   OP_DIM,                       /* Fortran DIM (positive difference) fn. */
   OP_GAUSS,                     /* Gaussian random number */
   OP_MOD,                       /* Modulus function */
   OP_POW,                       /* Raise to power */
   OP_RAND,                      /* Uniformly distributed random number */
   OP_SIGN,                      /* Transfer of sign function */

/* Functions with variable numbers of arguments. */
   OP_MAX,                       /* Maximum of 2 or more values */
   OP_MIN,                       /* Minimum of 2 or more values */

/* Unary arithmetic operators. */
   OP_NEG,                       /* Negate (change sign) */

/* Unary boolean operators. */
   OP_NOT,                       /* Boolean NOT */

/* Binary arithmetic operators. */
   OP_ADD,                       /* Add */
   OP_DIV,                       /* Divide */
   OP_MUL,                       /* Multiply */
   OP_SUB,                       /* Subtract */

/* Bit-shift operators. */
   OP_SHFTL,                     /* Shift bits left */
   OP_SHFTR,                     /* Shift bits right */

/* Relational operators. */
   OP_EQ,                        /* Relational equal */
   OP_GE,                        /* Greater than or equal */
   OP_GT,                        /* Greater than */
   OP_LE,                        /* Less than or equal */
   OP_LT,                        /* Less than */
   OP_NE,                        /* Not equal */

/* Bit-wise operators. */
   OP_BITAND,                    /* Bit-wise AND */
   OP_BITOR,                     /* Bit-wise OR */
   OP_BITXOR,                    /* Bit-wise exclusive OR */

/* Binary boolean operators. */
   OP_AND,                       /* Boolean AND */
   OP_EQV,                       /* Fortran logical .EQV. operation */
   OP_OR,                        /* Boolean OR */
   OP_XOR,                       /* Boolean exclusive OR */

/* Null operation. */
   OP_NULL,                      /* Null operation */
} Oper;

/* This structure holds a description of each symbol which may appear
   in an expression. */
typedef struct {
   const char *text;             /* Symbol text as it appears in expressions */
   const int size;               /* Size of symbol text */
   const int operleft;           /* An operator when seen from the left? */
   const int operright;          /* An operator when seen from the right? */
   const int unarynext;          /* May be followed by a unary +/- ? */
   const int unaryoper;          /* Is a unary +/- ? */
   const int leftpriority;       /* Priority when seen from the left */
   const int rightpriority;      /* Priority when seen from the right */
   const int parincrement;       /* Change in parenthesis level */
   const int stackincrement;     /* Change in evaluation stack size */
   const int nargs;              /* Number of function arguments */
   const Oper opcode;            /* Resulting operation code */
} Symbol;

/* This initialises an array of Symbol structures to hold data on all
   the supported symbols. The order is not important, but symbols are
   arranged here in approximate order of descending evaluation
   priority. The end of the array is indicated by an element with a NULL
   "text" component. */
static const Symbol symbol[] = {

/* User-supplied constants and variables. */
   { ""            ,  0,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDCON    },
   { ""            ,  0,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDVAR    },

/* System constants. */
   { "<bad>"       ,  5,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDBAD    },
   { "<dig>"       ,  5,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDDIG    },
   { "<epsilon>"   ,  9,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDEPS    },
   { "<mant_dig>"  , 10,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDMDIG   },
   { "<max>"       ,  5,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDMAX    },
   { "<max_10_exp>", 12,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDMAX10E },
   { "<max_exp>"   ,  9,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDMAXE   },
   { "<min>"       ,  5,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDMIN    },
   { "<min_10_exp>", 12,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDMIN10E },
   { "<min_exp>"   ,  9,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDMINE   },
   { "<radix>"     ,  7,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDRAD    },
   { "<rounds>"    ,  8,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDRND    },

/* Mathematical constants. */
   { "<e>"         ,  3,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDE      },
   { "<pi>"        ,  4,  0,  0,  0,  0, 19, 19,  0,  1,  0,  OP_LDPI     },

/* Functions with one argument. */
   { "abs("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ABS      },
   { "acos("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ACOS     },
   { "acosd("      ,  6,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ACOSD    },
   { "aint("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_INT      },
   { "asin("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ASIN     },
   { "asind("      ,  6,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ASIND    },
   { "atan("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ATAN     },
   { "atand("      ,  6,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ATAND    },
   { "ceil("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_CEIL     },
   { "cos("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_COS      },
   { "cosd("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_COSD     },
   { "cosh("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_COSH     },
   { "exp("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_EXP      },
   { "exp2("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_EXP2     },
   { "fabs("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ABS      },
   { "floor("      ,  6,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_FLOOR    },
   { "fr2("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_FR2      },
   { "int("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_INT      },
   { "isbad("      ,  6,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_ISBAD    },
   { "log("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_LOG      },
   { "log10("      ,  6,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_LOG10    },
   { "nint("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_NINT     },
   { "poisson("    ,  8,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_POISS    },
   { "sin("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_SIN      },
   { "sind("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_SIND     },
   { "sinh("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_SINH     },
   { "sqrt("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_SQRT     },
   { "tan("        ,  4,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_TAN      },
   { "tand("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_TAND     },
   { "tanh("       ,  5,  0,  1,  1,  0, 19,  1,  1,  0,  1,  OP_TANH     },

/* Functions with two arguments. */
   { "atan2("      ,  6,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_ATAN2    },
   { "atan2d("     ,  7,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_ATAN2D   },
   { "dim("        ,  4,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_DIM      },
   { "fmod("       ,  5,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_MOD      },
   { "gauss("      ,  6,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_GAUSS    },
   { "mod("        ,  4,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_MOD      },
   { "pow("        ,  4,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_POW      },
   { "rand("       ,  5,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_RAND     },
   { "sign("       ,  5,  0,  1,  1,  0, 19,  1,  1, -1,  2,  OP_SIGN     },

/* Functions with variable numbers of arguments. */
   { "max("        ,  4,  0,  1,  1,  0, 19,  1,  1, -1, -2,  OP_MAX      },
   { "min("        ,  4,  0,  1,  1,  0, 19,  1,  1, -1, -2,  OP_MIN      },

/* Parenthesised expressions. */
   { ")"           ,  1,  1,  0,  0,  0,  2, 19, -1,  0,  0,  OP_NULL     },
   { "("           ,  1,  0,  1,  1,  0, 19,  1,  1,  0,  0,  OP_NULL     },

/* Unary arithmetic operators. */
   { "+"           ,  1,  0,  1,  1,  1, 17, 16,  0,  0,  0,  OP_NULL     },
   { "-"           ,  1,  0,  1,  1,  1, 17, 16,  0,  0,  0,  OP_NEG      },

/* Unary boolean operators. */
   { "!"           ,  1,  0,  1,  1,  0, 17, 16,  0,  0,  0,  OP_NOT      },
   { ".not."       ,  5,  0,  1,  1,  0, 17, 16,  0,  0,  0,  OP_NOT      },

/* Binary arithmetic operators. */
   { "**"          ,  2,  1,  1,  1,  0, 18, 15,  0, -1,  0,  OP_POW      },
   { "*"           ,  1,  1,  1,  1,  0, 14, 14,  0, -1,  0,  OP_MUL      },
   { "/"           ,  1,  1,  1,  1,  0, 14, 14,  0, -1,  0,  OP_DIV      },
   { "+"           ,  1,  1,  1,  1,  0, 13, 13,  0, -1,  0,  OP_ADD      },
   { "-"           ,  1,  1,  1,  1,  0, 13, 13,  0, -1,  0,  OP_SUB      },

/* Bit-shift operators. */
   { "<<"          ,  2,  1,  1,  1,  0, 12, 12,  0, -1,  0,  OP_SHFTL    },
   { ">>"          ,  2,  1,  1,  1,  0, 12, 12,  0, -1,  0,  OP_SHFTR    },

/* Relational operators. */
   { "<"           ,  1,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_LT       },
   { ".lt."        ,  4,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_LT       },
   { "<="          ,  2,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_LE       },
   { ".le."        ,  4,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_LE       },
   { ">"           ,  1,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_GT       },
   { ".gt."        ,  4,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_GT       },
   { ">="          ,  2,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_GE       },
   { ".ge."        ,  4,  1,  1,  1,  0, 11, 11,  0, -1,  0,  OP_GE       },
   { "=="          ,  2,  1,  1,  1,  0, 10, 10,  0, -1,  0,  OP_EQ       },
   { ".eq."        ,  4,  1,  1,  1,  0, 10, 10,  0, -1,  0,  OP_EQ       },
   { "!="          ,  2,  1,  1,  1,  0, 10, 10,  0, -1,  0,  OP_NE       },
   { ".ne."        ,  4,  1,  1,  1,  0, 10, 10,  0, -1,  0,  OP_NE       },

/* Bit-wise operators. */
   { "&"           ,  1,  1,  1,  1,  0,  9,  9,  0, -1,  0,  OP_BITAND   },
   { "^"           ,  1,  1,  1,  1,  0,  8,  8,  0, -1,  0,  OP_BITXOR   },
   { "|"           ,  1,  1,  1,  1,  0,  7,  7,  0, -1,  0,  OP_BITOR    },

/* Binary boolean operators. */
   { "&&"          ,  2,  1,  1,  1,  0,  6,  6,  0, -1,  0,  OP_AND      },
   { ".and."       ,  5,  1,  1,  1,  0,  6,  6,  0, -1,  0,  OP_AND      },
   { "^^"          ,  2,  1,  1,  1,  0,  5,  5,  0, -1,  0,  OP_XOR      },
   { "||"          ,  2,  1,  1,  1,  0,  4,  4,  0, -1,  0,  OP_OR       },
   { ".or."        ,  4,  1,  1,  1,  0,  4,  4,  0, -1,  0,  OP_OR       },
   { ".eqv."       ,  5,  1,  1,  1,  0,  3,  3,  0, -1,  0,  OP_EQV      },
   { ".neqv."      ,  6,  1,  1,  1,  0,  3,  3,  0, -1,  0,  OP_XOR      },
   { ".xor."       ,  5,  1,  1,  1,  0,  3,  3,  0, -1,  0,  OP_XOR      },

/* Separators. */
   { ","           ,  1,  1,  1,  1,  0,  2,  2,  0,  0,  0,  OP_NULL     },

/* End of symbol data. */
   { NULL          ,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  OP_NULL     }
};

/* These variables identify indices in the above array which hold
   special symbols used explicitly in the code. */
static const int symbol_ldcon = 0; /* Load a constant */
static const int symbol_ldvar = 1; /* Load a variable */

/* External Interface Function Prototypes. */
/* ======================================= */
/* The following functions have public prototypes only (i.e. no
   protected prototypes), so we must provide local prototypes for use
   within this module. */
AstMathMap *astMathMapId_( int, int, const char *[], const char *[], const char *, ... );

/* Prototypes for Private Member Functions. */
/* ======================================== */
static AstPointSet *Transform( AstMapping *, AstPointSet *, int, AstPointSet * );
static const char *GetAttrib( AstObject *, const char * );
static double Gauss( Rcontext * );
static double LogGamma( double );
static double Poisson( Rcontext *, double );
static double Rand( Rcontext * );
static int DefaultSeed( const Rcontext * );
static int GetSeed( AstMathMap * );
static int GetSimpFI( AstMathMap * );
static int GetSimpIF( AstMathMap * );
static int MapMerge( AstMapping *, int, int, int *, AstMapping ***, int ** );
static int TestAttrib( AstObject *, const char * );
static int TestSeed( AstMathMap * );
static int TestSimpFI( AstMathMap * );
static int TestSimpIF( AstMathMap * );
static void CleanFunctions( int, const char *[], char *** );
static void ClearAttrib( AstObject *, const char * );
static void ClearSeed( AstMathMap * );
static void ClearSimpFI( AstMathMap * );
static void ClearSimpIF( AstMathMap * );
static void CompileExpression( const char *, const char *, int, const char *[], int **, double **, int * );
static void CompileMapping( const char *, int, int, const char *[], const char *[], int ***, int ***, double ***, double ***, int *, int * );
static void Copy( const AstObject *, AstObject * );
static void Delete( AstObject * );
static void Dump( AstObject *, AstChannel * );
static void EvaluationSort( const double [], int, int [], int **, int * );
static void ExtractExpressions( const char *, int, const char *[], char *** );
static void ExtractVariables( const char *, int, const char *[], char *** );
static void InitVtab( AstMathMapVtab * );
static void ParseConstant( const char *, const char *, int, int *, double * );
static void ParseName( const char *, int, int * );
static void ParseVariable( const char *, const char *, int, int, const char *[], int *, int * );
static void SetAttrib( AstObject *, const char * );
static void SetSeed( AstMathMap *, int );
static void SetSimpFI( AstMathMap *, int );
static void SetSimpIF( AstMathMap *, int );
static void ValidateSymbol( const char *, const char *, int, int, int *, int **, int **, int *, double ** );
static void VirtualMachine( Rcontext *, int, int, const double **, const int *, const double *, int, double * );

/* Member functions. */
/* ================= */
static void CleanFunctions( int nfun, const char *fun[], char ***clean ) {
/*
*  Name:
*     CleanFunctions

*  Purpose:
*     Make a clean copy of a set of functions.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void CleanFunctions( int nfun, const char *fun[], char ***clean )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function copies an array of strings, eliminating any white space
*     characters and converting to lower case. It is intended for cleaning
*     up arrays of function definitions prior to compilation. The returned
*     copy is stored in dynamically allocated memory.

*  Parameters:
*     nfun
*        The number of functions to be cleaned.
*     fun
*        Pointer to an array, with "nfun" elements, of pointers to null
*        terminated strings which contain each of the functions.
*     clean
*        Address in which to return a pointer to an array (with "nfun"
*        elements) of pointers to null terminated strings containing the
*        cleaned functions (i.e. this returns an array of strings).
*
*        Both the returned array of pointers, and the strings to which they
*        point, will be dynamically allocated and should be freed by the
*        caller (using astFree) when no longer required.

*  Notes:
*        - A NULL value will be returned for "*clean" if this function is
*        invoked with the global error status set, or if it should fail for
*        any reason.
*/

/* Local Variables: */
   char c;                       /* Character from function string */
   int i;                        /* Loop counter for characters */
   int ifun;                     /* Loop counter for functions */
   int nc;                       /* Count of non-blank characters */
   
/* Initialise. */
   *clean = NULL;

/* Check the global error status. */
   if ( !astOK ) return;

/* Allocate and initialise an array to hold the returned pointers. */
   MALLOC_POINTER_ARRAY( *clean, char *, nfun )

/* Loop through all the input functions. */
   if ( astOK ) {
      for ( ifun = 0; ifun < nfun; ifun++ ) {

/* Count the number of non-blank characters in each function string. */
         nc = 0;
         for ( i = 0; ( c = fun[ ifun ][ i ] ); i++ ) nc += !isspace( c );

/* Allocate a string long enough to hold the function with all the
   white space removed, storing its pointer in the array allocated
   earlier. Check for errors. */
         ( *clean )[ ifun ] = astMalloc( sizeof( char ) *
                                         (size_t) ( nc + 1 ) );
         if ( !astOK ) break;

/* Loop to copy the non-blank function characters into the new
   string. */
         nc = 0;
         for ( i = 0; ( c = fun[ ifun ][ i ] ); i++ ) {
            if ( !isspace( c ) ) ( *clean )[ ifun ][ nc++ ] = tolower( c );
         }

/* Null-terminate the result. */
         ( *clean )[ ifun ][ nc ] = '\0';
      }

/* If an error occurred, then free the main pointer array together
   with any strings that have been allocated, resetting the output
   value. */
      if ( !astOK ) {
         FREE_POINTER_ARRAY( *clean, nfun )
      }
   }
}

static void ClearAttrib( AstObject *this_object, const char *attrib ) {
/*
*  Name:
*     ClearAttrib

*  Purpose:
*     Clear an attribute value for a MathMap.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void ClearAttrib( AstObject *this, const char *attrib )

*  Class Membership:
*     MathMap member function (over-rides the astClearAttrib protected
*     method inherited from the Mapping class).

*  Description:
*     This function clears the value of a specified attribute for a
*     MathMap, so that the default value will subsequently be used.

*  Parameters:
*     this
*        Pointer to the MathMap.
*     attrib
*        Pointer to a null terminated string specifying the attribute
*        name.  This should be in lower case with no surrounding white
*        space.
*/

/* Local Variables: */
   AstMathMap *this;             /* Pointer to the MathMap structure */

/* Check the global error status. */
   if ( !astOK ) return;

/* Obtain a pointer to the MathMap structure. */
   this = (AstMathMap *) this_object;

/* Check the attribute name and clear the appropriate attribute. */

/* Seed. */
/* ----- */
   if ( !strcmp( attrib, "seed" ) ) {
      astClearSeed( this );

/* SimpFI. */
/* ------- */
   } else if ( !strcmp( attrib, "simpfi" ) ) {
      astClearSimpFI( this );

/* SimpIF. */
/* ------- */
   } else if ( !strcmp( attrib, "simpif" ) ) {
      astClearSimpIF( this );
   
/* If the attribute is not recognised, pass it on to the parent method
   for further interpretation. */
   } else {
      (*parent_clearattrib)( this_object, attrib );
   }
}

static void CompileExpression( const char *method, const char *exprs,
                               int nvar, const char *var[],
                               int **code, double **con, int *stacksize ) {
/*
*  Name:
*     CompileExpression

*  Purpose:
*     Compile a mathematical expression.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void CompileExpression( const char *method, const char *exprs,
*                             int nvar, const char *var[],
*                             int **code, double **con, int *stacksize )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function checks and compiles a mathematical expression. It
*     produces a sequence of operation codes (opcodes) and a set of
*     numerical constants which may subsequently be used to evaluate the
*     expression on a push-down stack.

*  Parameters:
*     method
*        Pointer to a constant null-terminated character string
*        containing the name of the method that invoked this function.
*        This method name is used solely for constructing error messages.
*     exprs
*        Pointer to a null-terminated string containing the expression
*        to be compiled. This is case sensitive and should contain no white
*        space.
*     nvar
*        The number of variable names defined for use in the expression.
*     var
*        An array of pointers (with "nvar" elements) to null-terminated
*        strings. Each of these should contain a variable name which may
*        appear in the expression. These strings are case sensitive and
*        should contain no white space.
*     code
*        Address of a pointer which will be set to point at a dynamically
*        allocated array of int containing the set of opcodes (cast to int)
*        produced by this function. The first element of this array will
*        contain a count of the number of opcodes which follow.
*
*        The allocated space must be freed by the caller (using astFree) when
*        no longer required.
*     con
*        Address of a pointer which will be set to point at a dynamically
*        allocated array of double containing the set of constants
*        produced by this function (this may be NULL if no constants are
*        produced).
*
*        The allocated space must be freed by the caller (using astFree) when
*        no longer required.
*     stacksize
*        Pointer to an int in which to return the size of the push-down stack
*        required to evaluate the expression using the returned opcodes and
*        constants.

*  Algorithm:
*     The function passes through the input expression searching for
*     symbols. It looks for standard symbols (arithmetic operators,
*     parentheses, function calls and delimiters) in the next part of the
*     expression to be parsed, using identification information stored in
*     the static "symbol" array. It ignores certain symbols, according to
*     whether they appear to be operators or operands. The choice depends on
*     what the previous symbol was; for instance, two operators may not
*     occur in succession. Unary +/- operators are also ignored in
*     situations where they are not permitted.
*
*     If a standard symbol is found, it is passed to the ValidateSymbol
*     function, which keeps track of the current level of parenthesis in the
*     expression and of the number of arguments supplied to any (possibly
*     nested) function calls. This function then accepts or rejects the
*     symbol according to whether it is valid within the current context. An
*     error is reported if it is rejected.
*
*     If the part of the expression currently being parsed did not contain a
*     standard symbol, an attempt is made to parse it first as a constant,
*     then as a variable name. If either of these succeeds, an appropriate
*     symbol number is added to the list of symbols identified so far, and a
*     value is added to the list of constants - this is either the value of
*     the constant itself, or the identification number of the variable. If
*     the expression cannot be parsed, an error is reported.
*
*     When the entire expression has been analysed as a sequence of symbols
*     (and associated constants), the EvaluationSort function is
*     invoked. This sorts the symbols into evaluation order, which is the
*     order in which the associated operations must be performed on a
*     push-down arithmetic stack to evaluate the expression. This routine
*     also substitutes operation codes (defined in the "Oper" enum) for the
*     symbol numbers and calculates the size of evaluation stack which will
*     be required.

*  Notes:
*     - A value of NULL will be returned for the "*code" and "*con" pointers
*     and a value of zero will be returned for the "*stacksize" value if this
*     function is invoked with the global error status set, or if it should
*     fail for any reason.
*/

/* Local Variables: */
   double c;                     /* Value of parsed constant */
   int *argcount;                /* Array of argument count information */
   int *opensym;                 /* Array of opening parenthesis information */
   int *symlist;                 /* Array of symbol indices */
   int found;                    /* Standard symbol identified? */
   int iend;                     /* Ending index in the expression string */
   int istart;                   /* Staring index in the expression string */
   int isym;                     /* Loop counter for symbols */
   int ivar;                     /* Index of variable name */
   int lpar;                     /* Parenthesis level */
   int ncon;                     /* Number of constants generated */
   int nsym;                     /* Number of symbols identified */
   int opernext;                 /* Next symbol an operator (from left)? */
   int size;                     /* Size of symbol matched */
   int sym;                      /* Index of symbol in static "symbol" array */
   int unarynext;                /* Next symbol may be unary +/- ? */

/* Initialise. */
   *code = NULL;
   *con = NULL;
   *stacksize = 0;

/* Check the global error status. */
   if ( !astOK ) return;

/* Further initialisation. */
   argcount = NULL;
   lpar = 0;
   ncon = 0;
   nsym = 0;
   opensym = NULL;
   symlist = NULL;

/* The first symbol to be encountered must not look like an operator
   from the left. It may be a unary + or - operator. */
   opernext = 0;
   unarynext = 1;

/* Search through the expression to classify each symbol which appears
   in it. Stop when there are no more input characters or an error is
   detected. */
   istart = 0;
   for ( istart = 0; astOK && exprs[ istart ]; istart = iend + 1 ) {

/* Compare each of the symbols in the symbol data with the next
   section of the expression, looking for the longest symbol text which
   will match. Stop if a NULL "text" value is found, which acts as the
   end flag. */
      found = 0;
      size = 0;
      for ( isym = 0; symbol[ isym ].text; isym++ ) {

/* Only consider symbols which have text associated with them and
   which look like operators or operands from the left, according to the
   setting of the "opernext" flag. Thus, if an operator or operand is
   missing from the input expression, the next symbol will not be
   identified, because it will be of the wrong type. Also exclude unary
   +/- operators if they are out of context. */
         if ( symbol[ isym ].size &&
              ( symbol[ isym ].operleft == opernext ) &&
              ( !symbol[ isym ].unaryoper || unarynext ) ) {

/* Test if the text of the symbol matches the expression at the
   current position. If so, note that a match has been found. */
            if ( !strncmp( exprs + istart, symbol[ isym ].text,
                           (size_t) symbol[ isym ].size ) ) {
               found = 1;

/* If this symbol matches more characters than any previous symbol,
   then store the symbol's index and note its size. */
               if ( symbol[ isym ].size > size ) {
                  sym = isym;
                  size = symbol[ isym ].size;

/* Calculate the index of the last symbol character in the expression
   string. */
                  iend = istart + size - 1;
               }
            }
         }
      }

/* If the symbol was identified as one of the standard symbols, then
   validate it, updating the parenthesis level and argument count
   information at the same time. */
      if ( found ) {
         ValidateSymbol( method, exprs, iend, sym, &lpar, &argcount, &opensym,
                         &ncon, con );

/* If it was not one of the standard symbols, then check if the next
   symbol was expected to be an operator. If so, then there is a missing
   operator, so report an error. */
      } else {
         if ( opernext ) {
            astError( AST__MIOPR,
                      "%s: Missing or invalid operator in the expression "
                      "\"%.*s\".",
                      method, istart + 1, exprs );

/* If the next symbol was expected to be an operand, then it may be a
   constant, so try to parse it as one. */
         } else {
            ParseConstant( method, exprs, istart, &iend, &c );
            if ( astOK ) {

/* If successful, set the symbol number to "symbol_ldcon" (load
   constant) and extend the "*con" array to accommodate a new
   constant. Check for errors. */
               if ( iend >= istart ) {
                  sym = symbol_ldcon;
                  *con = astGrow( *con, ncon + 1, sizeof( double ) );
                  if ( astOK ) {

/* Append the constant to the "*con" array. */
                     ( *con )[ ncon++ ] = c;
                  }

/* If the symbol did not parse as a constant, then it may be a
   variable name, so try to parse it as one. */
               } else {
                  ParseVariable( method, exprs, istart, nvar, var,
                                 &ivar, &iend );
                  if ( astOK ) {

/* If successful, set the symbol to "symbol_ldvar" (load variable) and
   extend the "*con" array to accommodate a new constant. Check for
   errors. */
                     if ( ivar != -1 ) {
                        sym = symbol_ldvar;
                        *con = astGrow( *con, ncon + 1, sizeof( double ) );
                        if ( astOK ) {

/* Append the variable identification number as a constant to the
   "*con" array. */
                           ( *con )[ ncon++ ] = (double) ivar;
                        }

/* If the expression did not parse as a variable name, then there is a
   missing operand in the expression, so report an error. */
                     } else {
                        astError( AST__MIOPA,
                                  "%s: Missing or invalid operand in the "
                                  "expression \"%.*s\".",
                                  method, istart + 1, exprs );
                     }
                  }
               }
            }
         }
      }

/* If there has been no error, then the next symbol in the input
   expression has been identified and is valid. */
      if ( astOK ) {

/* Decide whether the next symbol should look like an operator or an
   operand from the left. This is determined by the nature of the symbol
   just identified (seen from the right) - two operands or two operators
   cannot be adjacent. */
         opernext = !symbol[ sym ].operright;

/* Also decide whether the next symbol may be a unary +/- operator,
   according to the "unarynext" symbol data entry for the symbol just
   identified. */
         unarynext = symbol[ sym ].unarynext;

/* Extend the "symlist" array to accommodate the symbol just
   identified. Check for errors. */
         symlist = astGrow( symlist, nsym + 1, sizeof( int ) );
         if ( astOK ) {

/* Append the symbol's index to the end of this list. */
            symlist[ nsym++ ] = sym;
         }
      }
   }

/* If there has been no error, check the final context after
   identifying all the symbols... */
   if ( astOK ) {

/* If an operand is still expected, then there is an unsatisfied
   operator on the end of the expression, so report an error. */
      if ( !opernext ) {
         astError( AST__MIOPA,
                   "%s: Missing or invalid operand in the expression \"%s\".",
                   method, exprs );

/* If the final parenthesis level is positive, then there is a missing
   right parenthesis, so report an error. */
      } else if ( lpar > 0 ) {
         astError( AST__MRPAR,
                   "%s: Missing right parenthesis in the expression \"%s\".",
                   method, exprs );
      }
   }

/* Sort the symbols into evaluation order to produce output opcodes. */
   EvaluationSort( *con, nsym, symlist, code, stacksize );

/* Free any memory used as workspace. */
   if ( argcount ) argcount = astFree( argcount );
   if ( opensym ) opensym = astFree( opensym );
   if ( symlist ) symlist = astFree( symlist );

/* If OK, re-allocate the "*con" array to have the correct size (since
   astGrow may have over-allocated space). */
   if ( astOK && *con ) {
      *con = astRealloc( *con, sizeof( double ) * (size_t) ncon );
   }

/* If an error occurred, free any allocated memory and reset the
   output values. */
   if ( !astOK ) {
      *code = astFree( *code );
      *con = astFree( *con );
      *stacksize = 0;
   }
}

static void CompileMapping( const char *method, int nin, int nout,
                            const char *fwdfun[], const char *invfun[],
                            int ***fwdcode, int ***invcode,
                            double ***fwdcon, double ***invcon,
                            int *fwdstack, int *invstack ) {
/*
*  Name:
*     CompileMapping

*  Purpose:
*     Compile the transformation functions for a MathMap.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void CompileMapping( const char *method, int nin, int nout,
*                          const char *fwdfun[], const char *invfun[],
*                          int ***fwdcode, int ***invcode,
*                          double ***fwdcon, double ***invcon,
*                          int *fwdstack, int *invstack )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function checks and compiles the transformation functions required
*     to create a MathMap. It produces sequences of operation codes (opcodes)
*     and numerical constants which may subsequently be used to evaluate the
*     functions on a push-down stack.

*  Parameters:
*     method
*        Pointer to a constant null-terminated character string
*        containing the name of the method that invoked this function.
*        This method name is used solely for constructing error messages.
*     nin
*        Number of input variables for the MathMap.
*     nout
*        Number of output variables for the MathMap.
*     fwdfun
*        Pointer to an array, with "nout" elements, of pointers to null
*        terminated strings which contain each of the forward transformation
*        functions. These must be in lower case and should contain no white
*        space.
*     invfun
*        Pointer to an array, with "nin" elements, of pointers to null
*        terminated strings which contain each of the inverse transformation
*        functions. These must be in lower case and should contain no white
*        space.
*     fwdcode
*        Address in which to return a pointer to an array (with "nout"
*        elements) of pointers to arrays of int containing the set of opcodes
*        (cast to int) for each forward transformation function. The number
*        of opcodes produced for each function is given by the first element
*        of the opcode array.
*
*        Both the returned array of pointers, and the arrays of int to which
*        they point, will be stored in dynamically allocated memory and should
*        be freed by the caller (using astFree) when no longer required.
*
*        If the right hand sides (including the "=" sign) of all the supplied
*        functions are absent, then this indicates an undefined transformation
*        and the returned pointer value will be NULL. An error results if
*        an "=" sign is present but no expression follows it.
*     invcode
*        Address in which to return a pointer to an array (with "nin"
*        elements) of pointers to arrays of int containing the set of opcodes
*        (cast to int) for each inverse transformation function. The number
*        of opcodes produced for each function is given by the first element
*        of the opcode array.
*
*        Both the returned array of pointers, and the arrays of int to which
*        they point, will be stored in dynamically allocated memory and should
*        be freed by the caller (using astFree) when no longer required.
*
*        If the right hand sides (including the "=" sign) of all the supplied
*        functions are absent, then this indicates an undefined transformation
*        and the returned pointer value will be NULL. An error results if
*        an "=" sign is present but no expression follows it.
*     fwdcon
*        Address in which to return a pointer to an array (with "nout"
*        elements) of pointers to arrays of double containing the set of
*        constants for each forward transformation function.
*
*        Both the returned array of pointers, and the arrays of double to which
*        they point, will be stored in dynamically allocated memory and should
*        be freed by the caller (using astFree) when no longer required. Note
*        that any of the pointers to the arrays of double may be NULL if no
*        constants are associated with a particular function.
*
*        If the forward transformation is undefined, then the returned pointer
*        value will be NULL.
*     invcon
*        Address in which to return a pointer to an array (with "nin"
*        elements) of pointers to arrays of double containing the set of
*        constants for each inverse transformation function.
*
*        Both the returned array of pointers, and the arrays of double to which
*        they point, will be stored in dynamically allocated memory and should
*        be freed by the caller (using astFree) when no longer required. Note
*        that any of the pointers to the arrays of double may be NULL if no
*        constants are associated with a particular function.
*
*        If the inverse transformation is undefined, then the returned pointer
*        value will be NULL.
*     fwdstack
*        Pointer to an int in which to return the size of the push-down stack
*        required to evaluate the forward transformation functions.
*     invstack
*        Pointer to an int in which to return the size of the push-down stack
*        required to evaluate the inverse transformation functions.

*  Notes:
*     - A value of NULL will be returned for the "*fwdcode", "*invcode",
*     "*fwdcon" and "*invcon" pointers and a value of zero will be returned
*     for the "*fwdstack" and "*invstack" values if this function is invoked
*     with the global error status set, or if it should fail for any reason.
*/

/* Local Variables: */
   char **exprs;                 /* Pointer to array of expressions */
   char **var;                   /* Pointer to array of variable names */
   int ifun;                     /* Loop counter for functions */
   int stacksize;                /* Required stack size */

/* Initialise. */
   *fwdcode = NULL;
   *invcode = NULL;
   *fwdcon = NULL;
   *invcon = NULL;
   *fwdstack = 0;
   *invstack = 0;

/* Check the global error status. */
   if ( !astOK ) return;

/* Further initialisation. */
   exprs = NULL;
   var = NULL;

/* Compile the forward transformation. */
/* ----------------------------------- */   
/* Extract the names of the input variables from the left hand sides
   of the inverse transformation functions. Report a contextual error if
   anything is wrong. */
   if ( astOK ) {
      ExtractVariables( method, nin, invfun, &var );
      if ( !astOK ) astError( astStatus,
                              "Error in inverse transformation function." );
   }

/* Extract the expressions from the right hand sides of the forward
   transformation functions. Report a contextual error if anything is
   wrong. */
   if ( astOK ) {
      ExtractExpressions( method, nout, fwdfun, &exprs );
      if ( !astOK ) astError( astStatus,
                              "Error in forward transformation function." );
   }

/* If OK, and the forward transformation is defined, then allocate and
   initialise space for an array of pointers to the opcodes for each
   expression and, similarly, for the constants for each expression. */
   if ( astOK && exprs ) {
      MALLOC_POINTER_ARRAY( *fwdcode, int *, nout )
      MALLOC_POINTER_ARRAY( *fwdcon, double *, nout )

/* If OK, loop to compile each of the expressions, storing pointers to
   the resulting opcodes and constants in the arrays allocated above. */
      if ( astOK ) {
         for ( ifun = 0; ifun < nout; ifun++ ) {
            CompileExpression( method, exprs[ ifun ],
                               nin, (const char **) var,
                               &( *fwdcode )[ ifun ], &( *fwdcon )[ ifun ],
                               &stacksize );

/* If an error occurs, then report contextual information and quit. */
            if ( !astOK ) {
               astError( astStatus,
                         "Error in forward transformation function number "
                         "%d.",
                         ifun + 1 );
               break;
            }

/* If OK, calculate the maximum evaluation stack size required by any
   of the expressions. */
            *fwdstack = ( *fwdstack > stacksize ) ? *fwdstack : stacksize;
         }
      }
   }

/* Free the memory containing the extracted expressions and variables. */
   FREE_POINTER_ARRAY( exprs, nout )
   FREE_POINTER_ARRAY( var, nin )

/* Compile the inverse transformation. */
/* ----------------------------------- */   
/* Extract the names of the output variables from the left hand sides
   of the forward transformation functions. Report a contextual error if
   anything is wrong. */
   if ( astOK ) {
      ExtractVariables( method, nout, fwdfun, &var );
      if ( !astOK ) astError( astStatus,
                              "Error in forward transformation function." );
   }

/* Extract the expressions from the right hand sides of the inverse
   transformation functions. Report a contextual error if anything is
   wrong. */
   if ( astOK ) {
      ExtractExpressions( method, nin, invfun, &exprs );
      if ( !astOK ) astError( astStatus,
                              "Error in inverse transformation function." );
   }

/* If OK, and the forward transformation is defined, then allocate and
   initialise space for an array of pointers to the opcodes for each
   expression and, similarly, for the constants for each expression. */
   if ( astOK && exprs ) {
      MALLOC_POINTER_ARRAY( *invcode, int *, nin )
      MALLOC_POINTER_ARRAY( *invcon, double *, nin )

/* If OK, loop to compile each of the expressions, storing pointers to
   the resulting opcodes and constants in the arrays allocated above. */
      if ( astOK ) {
         for ( ifun = 0; ifun < nin; ifun++ ) {
            CompileExpression( method, exprs[ ifun ],
                               nout, (const char **) var,
                               &( *invcode )[ ifun ], &( *invcon )[ ifun ],
                               &stacksize );

/* If an error occurs, then report contextual information and quit. */
            if ( !astOK ) {
               astError( astStatus,
                         "Error in inverse transformation function number "
                         "%d.",
                         ifun + 1 );
               break;
            }

/* If OK, calculate the maximum evaluation stack size required by any
   of the expressions. */
            *invstack = ( *invstack > stacksize ) ? *invstack : stacksize;
         }
      }
   }

/* Free the memory containing the extracted expressions and variables. */
   FREE_POINTER_ARRAY( exprs, nin )
   FREE_POINTER_ARRAY( var, nout )

/* If an error occurred, then free all remaining allocated memory and
   reset the output values. */
   if ( !astOK ) {
      FREE_POINTER_ARRAY( *fwdcode, nout )
      FREE_POINTER_ARRAY( *invcode, nin )
      FREE_POINTER_ARRAY( *fwdcon, nout )
      FREE_POINTER_ARRAY( *invcon, nin )
      *fwdstack = 0;
      *invstack = 0;
   }
}

static int DefaultSeed( const Rcontext *context ) {
/*
*  Name:
*     DefaultSeed

*  Purpose:
*     Generate an unpredictable seed for a random number generator.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     int DefaultSeed( Rcontext *context )

*  Class Membership:
*     MathMap member function.

*  Description:
*     On each invocation this function returns an integer value which is
*     highly unpredictable. This value may be used as a default seed for the
*     random number generator associated with a MathMap, so that it
*     generates a different sequence on each occasion.

*  Parameters:
*     context
*        Pointer to the random number generator context associated with
*        the MathMap.

*  Returned Value:
*     The unpredictable integer.

*  Notes:
*     - This function does not perform error checking and will execute even
*     if the global error status is set.
*/

/* Local Constants: */
   const int nwarm = 5;          /* Number of warm-up iterations */
   const long int a = 8121L;     /* Constants for random number generator... */
   const long int c = 28411L;
   const long int m = 134456L;

/* Local Variables; */
   int iwarm;                    /* Loop counter for warm-up iterations */
   static long init = 0;         /* Local initialisation performed? */
   static long int rand;         /* Local random integer */
   unsigned long int bits;       /* Bit pattern for producing result */

/* On the first invocation, initialise a local random number generator
   to a value derived by combining bit patterns obtained from the system
   clock and the processor time used. The result needs to be positive and
   lie in the range 0 to "m-1". */
   if ( !init ) {
      rand = (long int) ( ( (unsigned long int) time( NULL ) ^
                            (unsigned long int) clock() ) %
                          (unsigned long int) m );

/* These values will typically only change in their least significant
   bits between programs run successively, but by using the bit pattern
   as a seed, we ensure that these differences are rapidly propagated to
   other bits. To hasten this process, we "warm up" the local generator
   with a few iterations. This is a quick and dirty generator using
   constants from Press et al. (Numerical recipes). */
      for ( iwarm = 0; iwarm < nwarm; iwarm++ ) {
         rand = ( rand * a + c ) % m;
      }

/* Note that this initialisation has been performed. */
      init = 1;
   }

/* Generate a new bit pattern from the system time. Apart from the
   first invocation, this will be a different time to that used above. */
   bits = (unsigned long int) time( NULL );

/* Mask in a pattern derived from the CPU time used. */
   bits ^= (unsigned long int) clock();

/* The system time may change quite slowly (e.g. every second), so
   also mask in the address of the random number generator context
   supplied. This makes the seed depend on which MathMap is in use. */
   bits ^= (unsigned long int) context;

/* Now mask in the last random integer produced by the random number
   generator whose context has been supplied. This makes the seed depend
   on the MathMap's past use of random numbers. */
   bits ^= (unsigned long int) context->random_int;

/* Finally, in order to produce different seeds when this function is
   invoked twice in rapid succession on the same object (with no
   intermediate processing), we also mask in a pseudo-random value
   generated here. Generate the next local random integer. */
   rand = ( rand * a + c ) % m;

/* We then scale this value to give an integer in the range 0 to
   ULONG_MAX and mask the corresponding bit pattern into our seed. */
   bits ^= (unsigned long int) ( ( (double) rand / (double) ( m - 1UL ) ) *
                                 ( ( (double) ULONG_MAX + 1.0 ) *
                                   ( 1.0 - DBL_EPSILON ) ) );

/* Return the integer value of the seed (which may involve discarding
   some unwanted bits). */
   return (int) bits;
}

static void EvaluationSort( const double con[], int nsym, int symlist[],
                            int **code, int *stacksize ) {
/*
*  Name:
*     EvaluationSort

*  Purpose:
*     Perform an evaluation-order sort on parsed expression symbols.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void EvaluationSort( const double con[], int nsym, int symlist[],
*                          int **code, int *stacksize )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function sorts a sequence of numbers representing symbols
*     identified in an expression. The symbols (i.e. the expression syntax)
*     must have been fully validated beforehand, as no validation is
*     performed here.
*
*     The symbols are sorted into the order in which corresponding
*     operations must be performed on a push-down arithmetic stack in order
*     to evaluate the expression. Operation codes (opcodes), as defined in
*     the "Oper" enum, are then substituted for the symbol numbers.

*  Parameters:
*     con
*        Pointer to an array of double containing the set of constants
*        generated while parsing the expression (these are required in order
*        to determine the number of arguments associated with functions which
*        take a variable number of arguments).
*     nsym
*        The number of symbols identified while parsing the expression.
*     symlist
*        Pointer to an array of int, with "nsym" elements. On entry, this
*        should contain the indices in the static "symbol" array of the
*        symbols identified while parsing the expression. On exit, the
*        contents are undefined.
*     code
*        Address of a pointer which will be set to point at a dynamically
*        allocated array of int containing the set of opcodes (cast to int)
*        produced by this function. The first element of this array will
*        contain a count of the number of opcodes which follow.
*
*        The allocated space must be freed by the caller (using astFree) when
*        no longer required.
*     stacksize
*        Pointer to an int in which to return the size of the push-down stack
*        required to evaluate the expression using the returned opcodes.

*  Notes:
*     - A value of NULL will be returned for the "*code" pointer and a value
*     of zero will be returned for the "*stacksize" value if this function is
*     invoked with the global error status set, or if it should fail for any
*     reason.
*/

/* Local Variables: */
   int flush;                    /* Flush parenthesised symbol sequence? */
   int icon;                     /* Input constant counter */
   int isym;                     /* Input symbol counter */
   int ncode;                    /* Number of opcodes generated */
   int nstack;                   /* Evaluation stack size */
   int push;                     /* Push a new symbol on to stack? */
   int sym;                      /* Variable for symbol number */
   int tos;                      /* Top of sort stack index */

/* Initialise */
   *code = NULL;
   *stacksize = 0;

/* Check the global error status. */
   if ( !astOK ) return;

/* Further initialisation. */
   flush = 0;
   icon = 0;
   isym = 0;
   ncode = 0;
   nstack = 0;
   tos = -1;

/* Loop to generate output opcodes until the sort stack is empty and
   there are no further symbols to process, or an error is detected.  */
   while ( astOK && ( ( tos > -1 ) || ( isym < nsym ) ) ) {

/* Decide whether to push a symbol on to the sort stack (which
   "diverts" it so that higher-priority symbols can be output), or to pop
   the top symbol off the sort stack and send it to the output
   stream... */

/* We must push a symbol on to the sort stack if the stack is
   currently empty. */
      if ( tos == -1 ) {
         push = 1;

/* We must pop the top symbol off the sort stack if there are no more
   input symbols to process. */
      } else if ( isym >= nsym ) {
         push = 0;

/* If the sort stack is being flushed to complete the evaluation of a
   parenthesised expression, then the top symbol (which will be the
   opening parenthesis or function call) must be popped. This is only
   done once, so reset the "flush" flag before the next loop. */
      } else if ( flush ) {
         push = 0;
         flush = 0;

/* In all other circumstances, we must push a symbol on to the sort
   stack if its evaluation priority (seen from the left) is higher than
   that of the current top of stack symbol (seen from the right). This
   means it will eventually be sent to the output stream ahead of the
   current top of stack symbol. */
      } else {
         push = ( symbol[ symlist[ isym ] ].leftpriority >
                  symbol[ symlist[ tos ] ].rightpriority );
      }

/* If a symbol is being pushed on to the sort stack, then get the next
   input symbol which is to be used. */
      if ( push ) {
         sym = symlist[ isym++ ];

/* If the symbol decreases the parenthesis level (a closing
   parenthesis), then all the sort stack entries down to the symbol which
   opened the current level of parenthesis (the matching opening
   parenthesis or function call) will already have been sent to the
   output stream as a consequence of the evaluation priority defined for
   a closing parenthesis in the symbol data. The opening parenthesis (or
   function call) must next be flushed from the sort stack, so set the
   "flush" flag which is interpreted on the next loop. Ignore the current
   symbol, which cancels with the opening parenthesis on the stack. */
         if ( symbol[ sym ].parincrement < 0 ) {
            flush = 1;

/* All other symbols are pushed on to the sort stack. The stack
   occupies that region of the "symlist" array from which the input
   symbol numbers have already been extracted. */
         } else {
            symlist[ ++tos ] = sym;
         }

/* If a symbol is being popped from the top of the sort stack, then
   the top of stack entry is transferred to the output stream. Obtain the
   symbol number from the stack. Increment the local constant counter if
   the associated operation will use a constant. */
      } else {
         sym = symlist[ tos-- ];
         icon += ( ( sym == symbol_ldvar ) || ( sym == symbol_ldcon ) );

/* If the output symbol does not represent a "null" operation,
   increase the size of the output opcode array to accommodate it,
   checking for errors. Note that we allocate one extra array element
   (the first) which will eventually hold a count of all the opcodes
   generated. */
         if ( symbol[ sym ].opcode != OP_NULL ) {
            *code = astGrow( *code, ncode + 2, sizeof( int ) );
            if ( astOK ) {

/* Append the new opcode to the end of this array. */
               ( *code )[ ++ncode ] = (int) symbol[ sym ].opcode;

/* Increment/decrement the counter representing the stack size
   required for evaluation of the expression.  If the symbol is a
   function with a variable number of arguments (indicated by a negative
   "nargs" entry in the symbol data table), then the change in stack size
   must be determined from the argument number stored in the constant
   table. */
               if ( symbol[ sym ].nargs >= 0 ) {
                  nstack += symbol[ sym ].stackincrement;
               } else {
                  nstack -= (int) ( con[ icon++ ] + 0.5 ) - 1;
               }

/* Note the maximum size of the stack. */
               *stacksize = ( nstack > *stacksize ) ? nstack : *stacksize;
            }
         }
      }
   }

/* If no "*code" array has been allocated, then allocate one simply to
   store the number of opcodes generated, i.e. zero (this shouldn't
   normally happen as this represents an invalid expression). */
   if ( !*code ) *code = astMalloc( sizeof( int ) );

/* If no error has occurred, store the count of opcodes generated in
   the first element of the "*code" array and re-allocate the array to
   its final size (since astGrow may have over-allocated space). */
   if ( astOK ) {
      ( *code )[ 0 ] = ncode;
      *code = astRealloc( *code, sizeof( int ) * (size_t) ( ncode + 1 ) );
   }

/* If an error occurred, free any memory that was allocated and reset
   the output values. */
   if ( !astOK ) {
      *code = astFree( *code );
      *stacksize = 0;
   }
}

static void ExtractExpressions( const char *method, int nfun,
                                const char *fun[], char ***exprs ) {
/*
*  Name:
*     ExtractExpressions

*  Purpose:
*     Extract and validate expressions.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void ExtractExpressions( const char *method, int nfun,
*                              const char *fun[], char ***exprs )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function extracts expressions from the right hand sides of a set
*     of functions. These expressions are then validated to check that they
*     are either all present, or all absent (absence indicating an undefined
*     transformation). An error is reported if anything is found to be
*     wrong.
*
*     Note that the syntax of the expressions is not checked by this function
*     (i.e. they are not compiled).

*  Parameters:
*     method
*        Pointer to a constant null-terminated character string
*        containing the name of the method that invoked this function.
*        This method name is used solely for constructing error messages.
*     nfun
*        The number of functions to be analysed.
*     fun
*        Pointer to an array, with "nfun" elements, of pointers to null
*        terminated strings which contain each of the functions. These
*        strings should contain no white space.
*     exprs
*        Address in which to return a pointer to an array (with "nfun"
*        elements) of pointers to null terminated strings containing the
*        extracted expressions (i.e. this returns an array of strings).
*
*        Both the returned array of pointers, and the strings to which they
*        point, will be stored in dynamically allocated memory and should
*        be freed by the caller (using astFree) when no longer required.
*
*        If the right hand sides (including the "=" sign) of all the supplied
*        functions are absent, then this indicates an undefined transformation
*        and the returned pointer value will be NULL. An error results if
*        an "=" sign is present but no expression follows it.

*  Notes:
*        - A NULL value will be returned for "*exprs" if this function is
*        invoked with the global error status set, or if it should fail for
*        any reason.
*/

/* Local Variables: */
   char *ex;                     /* Pointer to start of expression string */
   int ifun;                     /* Loop counter for functions */
   int iud;                      /* Index of first undefined function */
   int nud;                      /* Number of undefined expressions */

/* Initialise. */
   *exprs = NULL;

/* Check the global error status. */
   if ( !astOK ) return;

/* Further initialisation. */
   nud = 0;
      
/* Allocate and initialise memory for the returned array of pointers. */
   MALLOC_POINTER_ARRAY( *exprs, char *, nfun )

/* Loop to inspect each function in turn. */
   if ( astOK ) {
      for ( ifun = 0; ifun < nfun; ifun++ ) {

/* Search for the first "=" sign. */
         if ( ( ex = strchr( fun[ ifun ], '=' ) ) ) {

/* If found, and there are more characters after the "=" sign, then
   find the length of the expression which follows. Allocate a string to
   hold this expression, storing its pointer in the array allocated
   above. Check for errors. */
            if ( *++ex ) {
               ( *exprs )[ ifun ] = astMalloc( strlen( ex ) + (size_t) 1 );
               if ( !astOK ) break;

/* If OK, extract the expression string. */
               (void) strcpy( ( *exprs )[ ifun ], ex );

/* If an "=" sign was found but there are no characters following it,
   then there is a missing right hand side to a function, so report an
   error and quit. */
            } else {
               astError( AST__NORHS,
                         "%s: Missing right hand side in function %d: \"%s\".",
                         method, ifun + 1, fun[ ifun ] );
               break;
            }

/* If no "=" sign was found, then the transformation may be undefined,
   in which case each function should only contain a variable name. Count
   the number of times this happens and record the index of the first
   instance. */
         } else {
            nud++;
            if ( nud == 1 ) iud = ifun;
         }
      }
   }

/* Either all functions should have an "=" sign (in which case the
   transformation is defined), nor none of them should have (in which
   case it is undefined). If some do and some don't, then report an
   error, citing the first instance of a missing "=" sign. */
   if ( astOK && ( nud != 0 ) && ( nud != nfun ) ) {
      astError( AST__NORHS,
                "%s: Missing right hand side in function %d: \"%s\".",
                method, iud + 1, fun[ iud ] );
   }

/* If an error occurred, or all the expressions were absent, then free any
   allocated memory and reset the output value. */
   if ( !astOK || nud ) {
      FREE_POINTER_ARRAY( *exprs, nfun )
   }
}

static void ExtractVariables( const char *method, int nfun, const char *fun[],
                              char ***var ) {
/*
*  Name:
*     ExtractVariables

*  Purpose:
*     Extract and validate variable names.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void ExtractVariables( const char *method, int nfun, const char *fun[],
*                            char ***var )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function extracts variable names from the left hand sides of a
*     set of functions. These variable names are then validated to check for
*     correct syntax and no duplication. An error is reported if anything is
*     wrong with the variable names obtained.

*  Parameters:
*     method
*        Pointer to a constant null-terminated character string
*        containing the name of the method that invoked this function.
*        This method name is used solely for constructing error messages.
*     nfun
*        The number of functions to be analysed.
*     fun
*        Pointer to an array, with "nfun" elements, of pointers to null
*        terminated strings which contain each of the functions. These
*        strings are case sensitive and should contain no white space.
*     var
*        Address in which to return a pointer to an array (with "nfun"
*        elements) of pointers to null terminated strings containing the
*        extracted variable names (i.e. this returns an array of strings).
*
*        Both the returned array of pointers, and the strings to which they
*        point, will be stored in dynamically allocated memory and should
*        be freed by the caller (using astFree) when no longer required.

*  Notes:
*        - A NULL value will be returned for "*var" if this function is
*        invoked with the global error status set, or if it should fail for
*        any reason.
*/

/* Local Variables: */
   char c;                       /* Extracted character */
   int i1;                       /* Loop counter for detecting duplicates */
   int i2;                       /* Loop counter for detecting duplicates */
   int i;                        /* Loop counter for characters */
   int iend;                     /* Last character index in parsed name */
   int ifun;                     /* Loop counter for functions */
   int nc;                       /* Character count */

/* Initialise. */
   *var = NULL;

/* Check the global error status. */
   if ( !astOK ) return;

/* Allocate and initialise memory for the returned array of pointers. */
   MALLOC_POINTER_ARRAY( *var, char *, nfun )

/* Loop to process each function in turn. */
   if ( astOK ) {
      for ( ifun = 0; ifun < nfun; ifun++ ) {

/* Count the number of characters appearing before the "=" sign (or in
   the entire string if the "=" is absent). */
         for ( nc = 0; ( c = fun[ ifun ][ nc ] ); nc++ ) if ( c == '=' ) break;

/* If no characters were counted, then report an appropriate error
   message, depending on whether the function string was entirely
   blank. */
         if ( !nc ) {
            if ( c ) {
               astError( AST__MISVN,
                         "%s: Function %d has no left hand side: \"%s\".",
                         method, ifun + 1, fun[ ifun ] );
            } else {
               astError( AST__MISVN,
                         "%s: Variable name number %d is missing.",
                         method, ifun + 1 );
            }
            break;
         }

/* If OK, allocate memory to hold the output string and check for
   errors. */
         ( *var )[ ifun ] = astMalloc( sizeof( char ) * (size_t) ( nc + 1 ) ) ;
         if ( !astOK ) break;
         
/* If OK, copy the characters before the "=" sign to the new
   string. */
         nc = 0;
         for ( i = 0; ( c = fun[ ifun ][ i ] ); i++ ) {
            if ( c == '=' ) break;
            ( *var )[ ifun ][ nc++] = c;
         }

/* Null terminate the result. */
         ( *var )[ ifun ][ nc ] = '\0';

/* Try to parse the contents of the extracted string as a name. */
         ParseName( ( *var )[ ifun ], 0, &iend );

/* If unsuccessful, or if all the characters were not parsed, then we
   have an invalid variable name, so report an error and quit. */
         if ( ( iend < 0 ) || ( *var )[ ifun ][ iend + 1 ] ) {
            astError( AST__VARIN,
                      "%s: Variable name number %d is invalid: \"%s\".",
                      method, ifun + 1, ( *var )[ ifun ] );
            break;
         }
      }
   }

/* If there has been no error, loop to compare all the variable names
   with each other to detect duplication. */
   if ( astOK ) {
      for ( i2 = 1; i2 < nfun; i2++ ) {
         for ( i1 = 0; i1 < i2; i1++ ) {

/* If a duplicate variable name is found, report an error and quit. */
            if ( !strcmp( ( *var )[ i1 ], ( *var )[ i2 ] ) ) {
               astError( AST__DUVAR,
                         "%s: Duplicate variable name \"%s\" in functions "
                         "%d and %d.",
                         method, ( *var )[ i1 ], i1 + 1, i2 + 1 );
               break;
            }
         }
         if ( !astOK ) break;
      }
   }

/* If an error occurred, free any allocated memory and reset the
   output value. */
   if ( !astOK ) {
      FREE_POINTER_ARRAY( *var, nfun )
   }
}

static double Gauss( Rcontext *context ) {
/*
*  Name:
*     Gauss

*  Purpose:
*     Produce a pseudo-random value from a standard Gaussian distribution.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     double Gauss( Rcontext *context )

*  Class Membership:
*     MathMap member function.

*  Description:
*     On each invocation, this function returns a pseudo-random number
*     selected from a standard Gaussian distribution with mean zero and
*     standard deviation unity. The Box-Muller method is used.

*  Parameters:
*     context
*        Pointer to an Rcontext structure which holds the random number
*        generator's context between invocations.

*  Returned Value:
*     A sample from a standard Gaussian distribution.

*  Notes:
*     - The sequence of numbers returned is determined by the "seed"
*     value in the Rcontext structure supplied.
*     - If the seed value is changed, the "active" flag must also be cleared
*     so that this function can re-initiallise the Rcontext structure before
*     generating the next pseudo-random number. The "active" flag should
*     also be clear to force initialisation the first time an Rcontext
*     structure is used.
*     - This function does not perform error checking and does not generate
*     errors. It will execute even if the global error status is set.
*/

/* Local Variables: */
   double rsq;                   /* Squared radius */
   double s;                     /* Scale factor */
   double x;                     /* First result value */
   static double y;              /* Second result value */
   static int ysaved = 0;        /* Previously-saved value available? */

/* If the random number generator context is not active, then it will
   be (re)initialised on the first invocation of Rand (below). Ensure
   that any previously-saved value within this function is first
   discarded. */
   if ( !context->active ) ysaved = 0;

/* If there is a previously-saved value available, then use it and
   mark it as no longer available. */
   if ( ysaved ) {
      x = y;
      ysaved = 0;

/* Otherwise, loop until a suitable new pair of values has been
   obtained. */
   } else {
      while ( 1 ) {

/* Loop to obtain two random values uniformly distributed inside the
   unit circle, while avoiding the origin (which maps to an infinite
   result). */
         do {
            x = 2.0 * Rand( context ) - 1.0;
            y = 2.0 * Rand( context ) - 1.0;
            rsq = x * x + y * y;
         } while ( ( rsq >= 1.0 ) || ( rsq == 0.0 ) );

/* Perform the Box-Muller transformation, checking that this will not
   produce overflow (which is extremely unlikely). If overflow would
   occur, we simply repeat the above steps with a new pair of random
   numbers. */
         s = -2.0 * log( rsq );
         if ( ( DBL_MAX * rsq ) >= s ) {
            s = sqrt( s / rsq );

/* Scale the original random values to give a pair of results. One will be
   returned and the second kept until next time. */
            x *= s;
            y *= s;
            break;
         }
      }

/* Note that a saved value is available. */
      ysaved = 1;
   }

/* Return the current result. */
   return x;
}

static const char *GetAttrib( AstObject *this_object, const char *attrib ) {
/*
*  Name:
*     GetAttrib

*  Purpose:
*     Get the value of a specified attribute for a MathMap.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     const char *GetAttrib( AstObject *this, const char *attrib )

*  Class Membership:
*     MathMap member function (over-rides the protected astGetAttrib
*     method inherited from the Mapping class).

*  Description:
*     This function returns a pointer to the value of a specified
*     attribute for a MathMap, formatted as a character string.

*  Parameters:
*     this
*        Pointer to the MathMap.
*     attrib
*        Pointer to a null-terminated string containing the name of
*        the attribute whose value is required. This name should be in
*        lower case, with all white space removed.

*  Returned Value:
*     - Pointer to a null-terminated string containing the attribute
*     value.

*  Notes:
*     - The returned string pointer may point at memory allocated
*     within the MathMap, or at static memory. The contents of the
*     string may be over-written or the pointer may become invalid
*     following a further invocation of the same function or any
*     modification of the MathMap. A copy of the string should
*     therefore be made if necessary.
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*/

/* Local Constants: */
#define BUFF_LEN 50              /* Max. characters in result buffer */

/* Local Variables: */
   AstMathMap *this;             /* Pointer to the MathMap structure */
   const char *result;           /* Pointer value to return */
   int ival;                     /* Integer attribute value */
   static char buff[ BUFF_LEN + 1 ]; /* Buffer for string result */

/* Initialise. */
   result = NULL;

/* Check the global error status. */   
   if ( !astOK ) return result;

/* Obtain a pointer to the MathMap structure. */
   this = (AstMathMap *) this_object;

/* Compare "attrib" with each recognised attribute name in turn,
   obtaining the value of the required attribute. If necessary, write
   the value into "buff" as a null-terminated string in an appropriate
   format.  Set "result" to point at the result string. */

/* Seed. */
/* ----- */
   if ( !strcmp( attrib, "seed" ) ) {
      ival = astGetSeed( this );
      if ( astOK ) {
         (void) sprintf( buff, "%d", ival );
         result = buff;
      }

/* SimpFI. */
/* ------- */
   } else if ( !strcmp( attrib, "simpfi" ) ) {
      ival = astGetSimpFI( this );
      if ( astOK ) {
         (void) sprintf( buff, "%d", ival );
         result = buff;
      }

/* SimpIF. */
/* ------- */
   } else if ( !strcmp( attrib, "simpif" ) ) {
      ival = astGetSimpIF( this );
      if ( astOK ) {
         (void) sprintf( buff, "%d", ival );
         result = buff;
      }

/* If the attribute name was not recognised, pass it on to the parent
   method for further interpretation. */
   } else {
      result = (*parent_getattrib)( this_object, attrib );
   }

/* Return the result. */
   return result;

/* Undefine macros local to this function. */
#undef BUFF_LEN
}

static void InitVtab( AstMathMapVtab *vtab ) {
/*
*  Name:
*     InitVtab

*  Purpose:
*     Initialise a virtual function table for a MathMap.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void InitVtab( AstMathMapVtab *vtab )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function initialises the component of a virtual function
*     table which is used by the MathMap class.

*  Parameters:
*     vtab
*        Pointer to the virtual function table. The components used by
*        all ancestral classes should already have been initialised.
*/

/* Local Variables: */
   AstMappingVtab *mapping;      /* Pointer to Mapping component of Vtab */
   AstObjectVtab *object;        /* Pointer to Object component of Vtab */

/* Check the local error status. */
   if ( !astOK ) return;

/* Store a unique "magic" value in the virtual function table. This
   will be used (by astIsAMathMap) to determine if an object belongs
   to this class.  We can conveniently use the address of the (static)
   class_init variable to generate this unique value. */
   vtab->check = &class_init;

/* Initialise member function pointers. */
/* ------------------------------------ */
/* Store pointers to the member functions (implemented here) that
   provide virtual methods for this class. */
   vtab->ClearSeed = ClearSeed;
   vtab->ClearSimpFI = ClearSimpFI;
   vtab->ClearSimpIF = ClearSimpIF;
   vtab->GetSeed = GetSeed;
   vtab->GetSimpFI = GetSimpFI;
   vtab->GetSimpIF = GetSimpIF;
   vtab->SetSeed = SetSeed;
   vtab->SetSimpFI = SetSimpFI;
   vtab->SetSimpIF = SetSimpIF;
   vtab->TestSeed = TestSeed;
   vtab->TestSimpFI = TestSimpFI;
   vtab->TestSimpIF = TestSimpIF;

/* Save the inherited pointers to methods that will be extended, and
   replace them with pointers to the new member functions. */
   object = (AstObjectVtab *) vtab;
   mapping = (AstMappingVtab *) vtab;

   parent_clearattrib = object->ClearAttrib;
   object->ClearAttrib = ClearAttrib;
   parent_getattrib = object->GetAttrib;
   object->GetAttrib = GetAttrib;
   parent_setattrib = object->SetAttrib;
   object->SetAttrib = SetAttrib;
   parent_testattrib = object->TestAttrib;
   object->TestAttrib = TestAttrib;

   parent_transform = mapping->Transform;
   mapping->Transform = Transform;

/* Store replacement pointers for methods which will be over-ridden by
   new member functions implemented here. */
   mapping->MapMerge = MapMerge;

/* Declare the copy constructor, destructor and class dump function. */
   astSetCopy( vtab, Copy );
   astSetDelete( vtab, Delete );
   astSetDump( vtab, Dump, "MathMap",
               "Transformation using mathematical functions" );
}

static double LogGamma( double x ) {
/*
*  Name:
*     LogGamma

*  Purpose:
*     Calculate the logarithm of the gamma function.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     double LogGamma( double x )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function returns the natural logarithm of the gamma function
*     for real arguments x>0. It uses the approximation of Lanczos, with
*     constants from Press et al. (Numerical Recipes), giving a maximum
*     fractional error (on the gamma function) of less than 2e-10.

*  Parameters:
*     x
*        The function argument, which must be greater than zero.

*  Returned Value:
*     The natural logarithm of the gamma function with "x" as argument,
*     or AST__BAD if "x" is not greater than zero.

*  Notes:
*     - This function does not generate errors and does not perform error
*     reporting. It will execute even if the global error status is set.
*/

/* Local Constants: */
   const double c0 = 1.000000000190015;
   const double c1 = 76.18009172947146;
   const double c2 = -86.50532032941677;
   const double c3 = 24.01409824083091;
   const double c4 = -1.231739572450155;
   const double c5 = 0.1208650973866179e-2;
   const double c6 = -0.5395239384953e-5;
   const double gamma = 5.0;

/* Local Variables: */
   double result;                /* Result value to return */
   double sum;                   /* Series sum */
   double xx;                    /* Denominator for summing series */
   static double root_twopi;     /* sqrt( 2.0 * pi ) */
   static int init = 0;          /* Initialisation performed? */

/* If initialisation has not yet been performed, calculate the
   constant required below. */
   if ( !init ) {
      root_twopi = sqrt( 2.0 * acos( -1.0 ) );

/* Note that initialisation has been performed. */
      init = 1;
   }

/* Return a bad value if "x" is not greater than zero. */
   if ( x <= 0.0 ) {
      result = AST__BAD;

/* Otherwise, form the series sum. Since we only use 6 terms, the loop
   that would normally be used has been completely unrolled here. */
   } else {
      xx = x;
      sum = c0;
      sum += c1 / ++xx;
      sum += c2 / ++xx;
      sum += c3 / ++xx;
      sum += c4 / ++xx;
      sum += c5 / ++xx;
      sum += c6 / ++xx;

/* Calculate the result. */
      result = x + gamma + 0.5;
      result -= ( x + 0.5 ) * log( result );
      result = -result + log( root_twopi * sum / x );
   }

/* Return the result. */
   return result;
}

static int MapMerge( AstMapping *this, int where, int series, int *nmap,
                     AstMapping ***map_list, int **invert_list ) {
/*
*  Name:
*     MapMerge

*  Purpose:
*     Simplify a sequence of Mappings containing a MathMap.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mapping.h"
*     int MapMerge( AstMapping *this, int where, int series, int *nmap,
*                   AstMapping ***map_list, int **invert_list )

*  Class Membership:
*     MathMap method (over-rides the protected astMapMerge method
*     inherited from the Mapping class).

*  Description:
*     This function attempts to simplify a sequence of Mappings by
*     merging a nominated MathMap in the sequence with its neighbours,
*     so as to shorten the sequence if possible.
*
*     In many cases, simplification will not be possible and the
*     function will return -1 to indicate this, without further
*     action.
*
*     In most cases of interest, however, this function will either
*     attempt to replace the nominated MathMap with one which it
*     considers simpler, or to merge it with the Mappings which
*     immediately precede it or follow it in the sequence (both will
*     normally be considered). This is sufficient to ensure the
*     eventual simplification of most Mapping sequences by repeated
*     application of this function.
*
*     In some cases, the function may attempt more elaborate
*     simplification, involving any number of other Mappings in the
*     sequence. It is not restricted in the type or scope of
*     simplification it may perform, but will normally only attempt
*     elaborate simplification in cases where a more straightforward
*     approach is not adequate.

*  Parameters:
*     this
*        Pointer to the nominated MathMap which is to be merged with
*        its neighbours. This should be a cloned copy of the MathMap
*        pointer contained in the array element "(*map_list)[where]"
*        (see below). This pointer will not be annulled, and the
*        MathMap it identifies will not be modified by this function.
*     where
*        Index in the "*map_list" array (below) at which the pointer
*        to the nominated MathMap resides.
*     series
*        A non-zero value indicates that the sequence of Mappings to
*        be simplified will be applied in series (i.e. one after the
*        other), whereas a zero value indicates that they will be
*        applied in parallel (i.e. on successive sub-sets of the
*        input/output coordinates).
*     nmap
*        Address of an int which counts the number of Mappings in the
*        sequence. On entry this should be set to the initial number
*        of Mappings. On exit it will be updated to record the number
*        of Mappings remaining after simplification.
*     map_list
*        Address of a pointer to a dynamically allocated array of
*        Mapping pointers (produced, for example, by the astMapList
*        method) which identifies the sequence of Mappings. On entry,
*        the initial sequence of Mappings to be simplified should be
*        supplied.
*
*        On exit, the contents of this array will be modified to
*        reflect any simplification carried out. Any form of
*        simplification may be performed. This may involve any of: (a)
*        removing Mappings by annulling any of the pointers supplied,
*        (b) replacing them with pointers to new Mappings, (c)
*        inserting additional Mappings and (d) changing their order.
*
*        The intention is to reduce the number of Mappings in the
*        sequence, if possible, and any reduction will be reflected in
*        the value of "*nmap" returned. However, simplifications which
*        do not reduce the length of the sequence (but improve its
*        execution time, for example) may also be performed, and the
*        sequence might conceivably increase in length (but normally
*        only in order to split up a Mapping into pieces that can be
*        more easily merged with their neighbours on subsequent
*        invocations of this function).
*
*        If Mappings are removed from the sequence, any gaps that
*        remain will be closed up, by moving subsequent Mapping
*        pointers along in the array, so that vacated elements occur
*        at the end. If the sequence increases in length, the array
*        will be extended (and its pointer updated) if necessary to
*        accommodate any new elements.
*
*        Note that any (or all) of the Mapping pointers supplied in
*        this array may be annulled by this function, but the Mappings
*        to which they refer are not modified in any way (although
*        they may, of course, be deleted if the annulled pointer is
*        the final one).
*     invert_list
*        Address of a pointer to a dynamically allocated array which,
*        on entry, should contain values to be assigned to the Invert
*        attributes of the Mappings identified in the "*map_list"
*        array before they are applied (this array might have been
*        produced, for example, by the astMapList method). These
*        values will be used by this function instead of the actual
*        Invert attributes of the Mappings supplied, which are
*        ignored.
*
*        On exit, the contents of this array will be updated to
*        correspond with the possibly modified contents of the
*        "*map_list" array.  If the Mapping sequence increases in
*        length, the "*invert_list" array will be extended (and its
*        pointer updated) if necessary to accommodate any new
*        elements.

*  Returned Value:
*     If simplification was possible, the function returns the index
*     in the "map_list" array of the first element which was
*     modified. Otherwise, it returns -1 (and makes no changes to the
*     arrays supplied).

*  Notes:
*     - A value of -1 will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*/

/* Local Variables: */
   AstMapping *new;              /* Pointer to replacement Mapping */
   AstMathMap *mathmap1;         /* Pointer to first MathMap */
   AstMathMap *mathmap2;         /* Pointer to second MathMap */
   char **fwd1;                  /* Pointer to first forward function array */
   char **fwd2;                  /* Pointer to second forward function array */
   char **inv1;                  /* Pointer to first inverse function array */
   char **inv2;                  /* Pointer to second inverse function array */
   int ifun;                     /* Loop counter for functions */
   int imap1;                    /* Index of first Mapping */
   int imap2;                    /* Index of second Mapping */
   int imap;                     /* Loop counter for Mappings */
   int invert1;                  /* Invert flag for first MathMap */
   int invert2;                  /* Invert flag for second MathMap */
   int nin1;                     /* Number input coords for first MathMap */
   int nout1;                    /* Number output coords for first MathMap */
   int nout2;                    /* Number output coords for second MathMap */
   int result;                   /* Result value to return */
   int simplify;                 /* Mappings may simplify? */

/* Initialise the returned result. */
   result = -1;

/* Check the global error status. */
   if ( !astOK ) return result;

/* MathMaps are only worth simplifying if they occur in series. */
   simplify = series;
   
/* If simplification appears possible, then obtain the indices of the
   nominated mapping and of the one which follows it. Check that a
   mapping exists for the second index. */
   if ( simplify ) {
      imap1 = where;
      imap2 = imap1 + 1;
      simplify = ( imap2 < *nmap );
   }

/* If OK, check whether the class of both Mappings is "MathMap" (a
   MathMap can only combine with another MathMap). */
   if ( simplify ) {
      simplify = !strcmp( astGetClass( ( *map_list )[ imap1 ] ), "MathMap" );
   }
   if ( astOK && simplify ) {
      simplify = !strcmp( astGetClass( ( *map_list )[ imap2 ] ), "MathMap" );
   }
      
/* If still OK, obtain pointers to the two MathMaps and the associated
   invert flag values. */
   if ( astOK && simplify ) {
      mathmap1 = (AstMathMap *) ( *map_list )[ imap1 ];
      mathmap2 = (AstMathMap *) ( *map_list )[ imap2 ];
      invert1 = ( *invert_list )[ imap1 ];
      invert2 = ( *invert_list )[ imap2 ];

/* Depending on the invert flag values, obtain the SimpFI or SimpIF
   attribute value from each MathMap and check whether they are set so as
   to permit simplification. */
      simplify = ( ( invert1 ? astGetSimpIF( mathmap1 ) :
                               astGetSimpFI( mathmap1 ) ) &&
                   ( invert2 ? astGetSimpFI( mathmap2 ) :
                               astGetSimpIF( mathmap2 ) ) );
   }

/* If still OK, obtain the effective numbers of input coordinates for
   the first MathMap and output coordinates for the second. Take account
   of the associated invert flags and the way the Invert attribute of
   each MathMap is currently set. */
   if ( astOK && simplify ) {
      nin1 = ( invert1 == astGetInvert( mathmap1 ) ) ?
             astGetNin( mathmap1 ) : astGetNout( mathmap1 );
      nout2 = ( invert2 == astGetInvert( mathmap2 ) ) ?
              astGetNout( mathmap2 ) : astGetNin( mathmap2 );

/* Simplification is only possible if these two numbers are equal
   (otherwise the the two MathMaps cannot be identical). */
      simplify = ( nin1 == nout2 );
   }

/* If still OK, obtain the effective number of output coordinates for
   the first MathMap, using the same technique as above. */
   if ( astOK && simplify ) {
      nout1 = ( invert1 == astGetInvert( mathmap1 ) ) ?
              astGetNout( mathmap1 ) : astGetNin( mathmap1 );
         
/* Obtain a pointer to the array of effective forward transformation
   functions for the first MathMap (allowing for the associated invert
   flag). Similarly, obtain a pointer to the effective inverse
   transformation functions for the second MathMap. */
      fwd1 = !invert1 ? mathmap1->fwdfun : mathmap1->invfun;
      inv2 = !invert2 ? mathmap2->invfun : mathmap2->fwdfun;

/* Loop to check whether these two sets of functions are
   identical. The MathMaps cannot be merged unless they are. */
      for ( ifun = 0; ( ifun < nout1 ) && astOK; ifun++ ) {
         simplify = !strcmp( fwd1[ ifun ], inv2[ ifun ] );
         if ( !simplify ) break;
      }
   }

/* If OK, repeat the above process to compare the effective inverse
   transformation functions of the first MathMap with the forward
   functions of the second one. */
   if ( astOK && simplify ) {
      inv1 = !invert1 ? mathmap1->invfun : mathmap1->fwdfun;
      fwd2 = !invert2 ? mathmap2->fwdfun : mathmap2->invfun;
      for ( ifun = 0; ( ifun < nout2 ) && astOK; ifun++ ) {
         simplify = !strcmp( inv1[ ifun ], fwd2[ ifun ] );
         if ( !simplify ) break;
      }
   }

/* If the two MathMaps can be merged, create a UnitMap as a
   replacement. */
   if ( astOK && simplify ) {
      new = (AstMapping *) astUnitMap( nin1, "" );

/* If OK, annul the pointers to the original MathMaps. */
      if ( astOK ) {
         ( *map_list )[ imap1 ] = astAnnul( ( *map_list )[ imap1 ] );
         ( *map_list )[ imap2 ] = astAnnul( ( *map_list )[ imap2 ] );

/* Insert the pointer to the replacement UnitMap and store the
   associated invert flag. */
         ( *map_list )[ imap1 ] = new;
         ( *invert_list )[ imap1 ] = 0;

/* Loop to move the following Mapping pointers and invert flags down
   in their arrays to close the gap. */
         for ( imap = imap2 + 1; imap < *nmap; imap++ ) {
            ( *map_list )[ imap - 1 ] = ( *map_list )[ imap ];
            ( *invert_list )[ imap - 1 ] = ( *invert_list )[ imap ];
         }

/* Clear the final entry in each array. */
         ( *map_list )[ *nmap - 1 ] = NULL;
         ( *invert_list )[ *nmap - 1 ] = 0;

/* Decrement the Mapping count and return the index of the first
   modified element. */
         ( *nmap )--;
         result = imap1;
      }
   }

/* If an error occurred, clear the returned value. */
   if ( !astOK ) result = -1;

/* Return the result. */
   return result;
}

static void ParseConstant( const char *method, const char *exprs,
                           int istart, int *iend, double *con ) {
/*
*  Name:
*     ParseConstant

*  Purpose:
*     Parse a constant.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void ParseConstant( const char *method, const char *exprs,
*                         int istart, int *iend, double *con )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This routine parses an expression, looking for a constant starting at
*     the character with index "istart" in the string "exprs". If it
*     identifies the constant successfully, "*con" it will return its value
*     and "*iend" will be set to the index of the final constant character
*     in "exprs".
*
*     If the characters encountered are clearly not part of a constant (it
*     does not begin with a numeral or decimal point) the function returns
*     with "*con" set to zero and "*iend" set to -1, but without reporting
*     an error. However, if the first character appears to be a constant but
*     its syntax proves to be invalid, then an error is reported.
*
*     The expression must be in lower case with no embedded white space.
*     The constant must not have a sign (+ or -) in front of it.

*  Parameters:
*     method
*        Pointer to a constant null-terminated character string
*        containing the name of the method that invoked this function.
*        This method name is used solely for constructing error messages.
*     exprs
*        Pointer to a null-terminated string containing the expression
*        to be parsed.
*     istart
*        Index of the first character in "exprs" to be considered by this
*        function.
*     iend
*        Pointer to an int in which to return the index in "exprs" of the
*        final character which forms part of the constant. If no constant is
*        found, a value of -1 is returned.
*     con
*        Pointer to a double, in which the value of the constant, if found,
*        will be returned.
*/

/* Local Variables: */
   char *str;                    /* Pointer to temporary string */
   char c;                       /* Single character from the expression */
   int dpoint;                   /* Decimal point encountered? */
   int expon;                    /* Exponent character encountered? */
   int i;                        /* Loop counter for characters */
   int iscon;		         /* Character is part of the constant? */
   int n;                        /* Number of values read by sscanf */
   int nc;                       /* Number of characters read by sscanf */
   int numer;                    /* Numeral encountered in current field? */
   int sign;                     /* Sign encountered? */
   int valid;		         /* Constant syntax valid? */

/* Check the global error status. */
   if ( !astOK ) return;

/* Initialise. */
   *con = 0.0;
   *iend = -1;

/* Check if the expression starts with a numeral or a decimal point. */
   c = exprs[ istart ];
   numer = isdigit( c );
   dpoint = ( c == '.' );

/* If it begins with any of these, the expression is clearly intended
   to be a constant, so any failure beyond this point will result in an
   error. Otherwise, failure to find a constant is not an error. */
   if ( numer || dpoint ) {

/* Initialise remaining variables specifying the parser context. */
      expon = 0;
      sign = 0;
      valid = 1;

/* Loop to increment the last constant character position until the
   following character in the expression does not look like part of the
   constant. */
      *iend = istart;
      iscon = 1;
      while ( ( c = exprs[ *iend + 1 ] ) && iscon ) {
         iscon = 0;

/* It may be part of a numerical constant if it is a numeral, wherever
   it occurs. */
         if ( isdigit( c ) ) {
            numer = 1;
            iscon = 1;

/* Or a decimal point, so long as it is the first one and is not in
   the exponent field. Otherwise it is invalid. */
         } else if ( c == '.' ) {
            if ( !( dpoint || expon ) ) {
               dpoint = 1;
               iscon = 1;
            } else {
               valid = 0;
            }

/* Or if it is a 'd' or 'e' exponent character, so long as it is the
   first one and at least one numeral has been encountered first.
   Otherwise it is invalid. */
          } else if ( ( c == 'd' ) || ( c == 'e' ) ) {
             if ( !expon && numer ) {
                expon = 1;
                numer = 0;
                iscon = 1;
             } else {
                valid = 0;
             }

/* Or if it is a sign, so long as it is in the exponent field and is
   the first sign with no previous numerals in the same field. Otherwise
   it is invalid (unless numerals have been encountered, in which case it
   marks the end of the constant). */
          } else if ( ( c == '+' ) || ( c == '-' ) ) {
             if ( expon && !sign && !numer ) {
                sign = 1;
                iscon = 1;
             } else if ( !numer ) {
                valid = 0;
             }
          }

/* Increment the character count if the next character may be part of
   the constant, or if it was invalid (it will then form part of the
   error message). */
          if ( iscon || !valid ) ( *iend )++;
      }

/* Finally, check that the last field contained a numeral. */
      valid = ( valid && numer );

/* If the constant appears valid, allocate a temporary string to hold
   it. */
      if ( valid ) {
         str = astMalloc( (size_t) ( *iend - istart + 2 ) );
         if ( astOK ) {

/* Copy the constant's characters, changing 'd' to 'e' so that
   "sscanf" will recognise it as an exponent character. */
            for ( i = istart; i <= *iend; i++ ) {
               str[ i - istart ] = ( exprs[ i ] == 'd' ) ? 'e' : exprs[ i ];
            }
            str[ *iend - istart + 1 ] = '\0';
            
/* Attempt to read the constant as a double, noting how many values
   are read and how many characters consumed. */
            n = sscanf( str, "%lf%n", con, &nc );

/* Check that one value was read and all the characters consumed. If
   not, then the constant's syntax is invalid. */
            if ( ( n != 1 ) || ( nc != ( *iend - istart + 1 ) ) ) valid = 0;
         }

/* Free the temporary string. */
         str = astFree( str );
      }

/* If the constant syntax is invalid, and no other error has occurred,
   then report an error. */
      if ( astOK && !valid ) {
         astError( AST__CONIN,
                   "%s: Invalid constant syntax in the expression \"%.*s\".",
                   method, *iend + 1, exprs );
      }

/* If an error occurred, reset the output values. */
      if ( !astOK ) {
         *iend = -1;
         *con = 0.0;
      }
   }
}

static void ParseName( const char *exprs, int istart, int *iend ) {
/*
*  Name:
*     ParseName

*  Purpose:
*     Parse a name.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void ParseName( const char *exprs, int istart, int *iend )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This routine parses an expression, looking for a name starting at the
*     character with index "istart" in the string "exprs". If it identifies
*     a name successfully, "*iend" will return the index of the final name
*     character in "exprs". A name must begin with an alphabetic character
*     and subsequently contain only alphanumeric characters or underscores.
*
*     If the expression does not contain a name at the specified location,
*     "*iend" is set to -1. No error results.
*
*     The expression should not contain embedded white space.

*  Parameters:
*     exprs
*        Pointer to a null-terminated string containing the expression
*        to be parsed.
*     istart
*        Index of the first character in "exprs" to be considered by this
*        function.
*     iend
*        Pointer to an int in which to return the index in "exprs" of the
*        final character which forms part of the name. If no name is
*        found, a value of -1 is returned.
*/

/* Local Variables: */
   char c;                       /* Single character from expression */

/* Check the global error status. */
   if ( !astOK ) return;

/* Initialise. */
   *iend = -1;

/* Check the first character is valid for a name (alphabetic). */
   if ( isalpha( exprs[ istart ] ) ) {

/* If so, loop to inspect each subsequent character until one is found
   which is not part of a name (not alphanumeric or underscore). */
      for ( *iend = istart; ( c = exprs[ *iend + 1 ] ); ( *iend )++ ) {
         if ( !( isalnum( c ) || ( c == '_' ) ) ) break;
      }
   }
}

static void ParseVariable( const char *method, const char *exprs,
                           int istart, int nvar,
                           const char *var[], int *ivar, int *iend ) {
/*
*  Name:
*     ParseVariable

*  Purpose:
*     Parse a variable name.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void ParseVariable( const char *method, const char *exprs,
*                         int istart, int nvar,
*                         const char *var[], int *ivar, int *iend )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This routine parses an expression, looking for a recognised variable
*     name starting at the character with index "istart" in the string
*     "exprs". If it identifies a variable name successfully, "*ivar" will
*     return a value identifying it and "*iend" will return the index of the
*     final variable name character in "exprs". To be recognised, a name
*     must begin with an alphabetic character and subsequently contain only
*     alphanumeric characters or underscores. It must also appear in the
*     list of defined variable names supplied to this function.
*
*     If the expression does not contain a name at the specified location,
*     "*ivar" and "*iend" are set to -1 and no error results. However, if
*     the expression contains a name but it is not in the list of defined
*     variable names supplied, then an error is also reported.
*
*     This function is case sensitive. The expression should not contain
*     embedded white space.

*  Parameters:
*     method
*        Pointer to a constant null-terminated character string
*        containing the name of the method that invoked this function.
*        This method name is used solely for constructing error messages.
*     exprs
*        Pointer to a null-terminated string containing the expression
*        to be parsed.
*     istart
*        Index of the first character in "exprs" to be considered by this
*        function.
*     nvar
*        The number of defined variable names.
*     var
*        An array of pointers (with "nvar" elements) to null-terminated
*        strings. Each of these should contain a variable name to be
*        recognised. These strings are case sensitive and should contain
*        no white space.
*     ivar
*        Pointer to an int in which to return the index in "vars" of the
*        variable name found. If no variable name is found, a value of -1
*        is returned.
*     iend
*        Pointer to an int in which to return the index in "exprs" of the
*        final character which forms part of the variable name. If no variable
*        name is found, a value of -1 is returned.
*/

/* Local Variables: */
   int found;                    /* Variable name recognised? */
   int nc;                       /* Number of characters in variable name */

/* Check the global error status. */
   if ( !astOK ) return;

/* Initialise. */
   *ivar = -1;
   *iend = -1;

/* Determine if the characters in the expression starting at index
   "istart" constitute a valid name. */
   ParseName( exprs, istart, iend );

/* If so, calculate the length of the name. */
   if ( *iend >= istart ) {
      nc = *iend - istart + 1;

/* Loop to compare the name with the list of variable names
   supplied. */
      found = 0;
      for ( *ivar = 0; *ivar < nvar; ( *ivar )++ ) {
         found = ( nc == (int) strlen( var[ *ivar ] ) );
         found = found && !strncmp( exprs + istart, var[ *ivar ],
                                    (size_t) nc );

/* Break if the name is recognised. */
         if ( found ) break;
      }

/* If it was not recognised, then report an error and reset the output
   values. */
      if ( !found ) {
         astError( AST__UDVOF,
                   "%s: Undefined variable or function in the expression "
                   "\"%.*s\".",
                   method, *iend + 1, exprs );
         *ivar = -1;
         *iend = -1;
      }
   }
}

static double Poisson( Rcontext *context, double mean ) {
/*
*  Name:
*     Poisson

*  Purpose:
*     Produce a pseudo-random value from a Poisson distribution.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     double Poisson( Rcontext *context, double mean )

*  Class Membership:
*     MathMap member function.

*  Description:
*     On each invocation, this function returns a pseudo-random number
*     selected from a Poisson distribution with a specified mean. A
*     combination of methods is used, depending on the value of the
*     mean. The algorithm is based on that given by Press et al.
*     (Numerical Recipes), but re-implemented and extended.

*  Parameters:
*     context
*        Pointer to an Rcontext structure which holds the random number
*        generator's context between invocations.
*     mean
*        The mean of the Poisson distribution, which should not be
*        negative.

*  Returned Value:
*     A sample (which will only take integer values) from the Poisson
*     distribution, or AST__BAD if the mean supplied is negative.

*  Notes:
*     - The sequence of numbers returned is determined by the "seed"
*     value in the Rcontext structure supplied.
*     - If the seed value is changed, the "active" flag must also be cleared
*     so that this function can re-initiallise the Rcontext structure before
*     generating the next pseudo-random number. The "active" flag should
*     also be clear to force initialisation the first time an Rcontext
*     structure is used.
*     - This function does not perform error checking and does not generate
*     errors. It will execute even if the global error status is set.
*/

/* Local Constants: */
   const double small = 9.3;     /* "Small" distribution mean value */

/* Local Variables: */
   double pfract;                /* Probability of accepting sample */
   double product;               /* Product of random samples */
   double ran;                   /* Sample from Lorentzian distribution */
   double result;                /* Result value to return */
   static double beta;           /* Constant for forming acceptance ratio */
   static double huge;           /* Large mean where std. dev. is negligible */
   static double last_mean;      /* Value of "mean" on last invocation */
   static double log_mean;       /* Logarithm of "mean" */
   static double pi;             /* Value of pi */
   static double ranmax;         /* Maximum safe value of "ran" */
   static double root_2mean;     /* sqrt( 2.0 * mean ) */
   static double sqrt_point9;    /* Square root of 0.9 */
   static double thresh;         /* Threshold for product of samples */
   static int init = 0;          /* Local initialisation performed? */

/* If initialisation has not yet been performed, then perform it
   now. */
   if ( !init ) {

/* Initialise the mean value from the previous invocation. */
      last_mean = -1.0;

/* Calculate simple constants. */
      pi = acos( -1.0 );
      sqrt_point9 = sqrt( 0.9 );

/* Calculate the value of the distribution mean for which the smallest
   representable deviation from the mean permitted by the machine
   precision is one million standard deviations. */
      huge = pow( 1.0e6 / DBL_EPSILON, 2.0 );

/* Calculate the largest value such that
   (0.9+(sqrt_point9*ranmax)*(sqrt_point9*ranmax)) doesn't overflow,
   allowing a small margin for rounding error. */
      ranmax = ( sqrt( DBL_MAX - 0.9 ) / sqrt( 0.9 ) ) *
               ( 1.0 - 4.0 * DBL_EPSILON );

/* Note that initialisation has been performed. */
      init = 1;
   }

/* If the distribution mean is less than zero, then return a bad
   result. */
   if ( mean < 0.0 ) {
      result = AST__BAD;

/* If the mean is zero, then the result can only be zero. */
   } else if ( mean == 0.0 ) {
      result = 0.0;

/* Otherwise, if the mean is sufficiently small, we can use the direct
   method of summing a series of exponentially distributed random samples
   and counting the number which occur before the mean is exceeded. This
   is equivalent to multiplying a series of uniformly distributed
   samples and counting the number which occur before the product
   becomes less then an equivalent threshold. */
   } else if ( mean <= small ) {

/* If the mean has changed since the last invocation, store the new
   mean and calculate a new threshold. */
      if ( mean != last_mean ) {
         last_mean = mean;
         thresh = exp( -mean );
      }

/* Initialise the product and the result. */
      product = 1.0;
      result = -1.0;

/* Multiply the random samples, counting the number needed to reach
   the threshold. */
      do {
         product *= Rand( context );
         result += 1.0;
      } while ( product > thresh );

/* Otherwise, if the distribution mean is large (but not huge), we
   must use an indirect rejection method. */
   } else if ( mean <= huge ) {

/* If the mean has changed since the last invocation, then
   re-calculate the constants required below. Note that because of the
   restrictions we have placed on "mean", these calculations are safe
   against overflow. */
      if ( mean != last_mean ) {
         last_mean = mean;
         log_mean = log( mean );
         root_2mean = sqrt( 2.0 * mean );
         beta = mean * log_mean - LogGamma( mean + 1.0 );
      }

/* Loop until a suitable random sample has been generated. */
      do {
         do {

/* First transform a sample from a uniform distribution to obtain a
   sample from a Lorentzian distribution. Check that the result is not so
   large as to cause overflow later. Also check for overflow in the maths
   library. If necessary, obtain a new sample. */
            do {
               errno = 0;
               ran = tan( pi * Rand( context ) );
            } while ( ( ran > ranmax ) ||
                      ( ( errno == ERANGE ) &&
                        ( ( ( ran >= 0.0 ) ? ran : -ran ) == HUGE_VAL ) ) );

/* If OK, scale the sample and add a constant so that the sample's
   distribution approximates the Poisson distribution we
   require. Overflow is prevented by the check on "ran" above, together
   with the restricted value of "mean". */
            result = ran * root_2mean + mean;

/* If the result is less than zero (where the Poisson distribution has
   value zero), then obtain a new sample. */
         } while ( result < 0.0 );

/* Round down to an integer, so that the sample is valid for a Poisson
   distribution. */
         result = floor( result );

/* Calculate the ratio between the required Poisson distribution and
   the Lorentzian from which we have sampled (the factor of 0.9 prevents
   this exceeding 1.0, and overflow is again prevented by the checks
   performed above). */
         ran *= sqrt_point9;
         pfract = ( 0.9 + ran * ran ) *
                  exp( result * log_mean - LogGamma( result + 1.0 ) - beta );

/* Accept the sample with this fractional probability, otherwise
   obtain a new sample. */
      } while ( Rand( context ) > pfract );

/* If the mean is huge, the relative standard deviation will be
   negligible compared to the machine precision. In such cases, the
   probability of getting a result that differs from the mean is
   effectively zero, so we can simply return the mean. */
   } else {
      result = mean;
   }

/* Return the result. */
   return result;
}

static double Rand( Rcontext *context ) {
/*
*  Name:
*     Rand

*  Purpose:
*     Produce a uniformly distributed pseudo-random number.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     double Rand( Rcontext *context )

*  Class Membership:
*     MathMap member function.

*  Description:
*     On each invocation, this function returns a pseudo-random number
*     uniformly distributed in the range 0.0 to 1.0 (inclusive). The
*     underlying algorithm is that used by the "ran2" function of Press et
*     al. (Numerical Recipes), which has a long period and good statistical
*     properties. This independent implementation returns double precision
*     values.

*  Parameters:
*     context
*        Pointer to an Rcontext structure which holds the random number
*        generator's context between invocations.

*  Notes:
*     - The sequence of numbers returned is determined by the "seed"
*     value in the Rcontext structure supplied.
*     - If the seed value is changed, the "active" flag must also be cleared
*     so that this function can re-initiallise the Rcontext structure before
*     generating the next pseudo-random number. The "active" flag should
*     also be clear to force initialisation the first time an Rcontext
*     structure is used.
*     - This function does not perform error checking and does not generate
*     errors. It will execute even if the global error status is set.
*/

/* Local Constants: */
   const long int a1 = 40014L;   /* Random number generator constants... */
   const long int a2 = 40692L;
   const long int m1 = 2147483563L;
   const long int m2 = 2147483399L;
   const long int q1 = 53668L;
   const long int q2 = 52774L;
   const long int r1 = 12211L;
   const long int r2 = 3791L;
   const int ntab =              /* Size of shuffle table */
      AST_MATHMAP_RAND_CONTEXT_NTAB_;
   const int nwarm = 8;          /* Number of warm-up iterations */

/* Local Variables: */
   double result;                /* Result value to return */
   double scale;                 /* Scale factor for random integers */
   double sum;                   /* Sum for forming normalisation constant */
   int dbits;                    /* Approximate bits in double mantissa */
   int irand;                    /* Loop counter for random integers */
   int itab;                     /* Loop counter for shuffle table */
   int lbits;                    /* Approximate bits used by generators */
   long int seed;                /* Random number seed */
   long int tmp;                 /* Temporary variable */
   static double norm;           /* Normalisation constant */
   static double scale0;         /* Scale decrement for successive integers */
   static int init = 0;          /* Local initialisation performed? */
   static int nrand;             /* Number of random integers to use */

/* If the random number generator context is not active, then
   initialise it. */
   if ( !context->active ) {

/* First, perform local initialisation for this function, if not
   already done. */
      if ( !init ) {

/* Obtain the approximate number of bits used by the random integer
   generator from the value "m1". */
         (void) frexp( (double) m1, &lbits );

/* Obtain the approximate number of bits used by the mantissa of the
   double value we want to produce, allowing for the (unlikely)
   possibility that the mantissa's radix isn't 2. */
         dbits = (int) ceil( (double) DBL_MANT_DIG *
                             log( (double) FLT_RADIX ) / log( 2.0 ) );

/* Hence determine how many random integers we need to combine to
   produce each double value, so that all the mantissa's bits will be
   used. */
         nrand = ( dbits + lbits - 1 ) / lbits;

/* Calculate the scale factor by which each successive random
   integer's contribution to the result is reduced so as to generate
   progressively less significant bits. */
         scale0 = 1.0 / (double) ( m1 - 1L );

/* Loop to sum the maximum contributions from each random integer
   (assuming that each takes the largest possible value, of "m1-1",
   from which we will later subtract 1). This produces the normalisation
   factor by which the result must be scaled so as to lie between 0.0 and
   1.0 (inclusive). */
         sum = 0.0;
         scale = 1.0;
         for ( irand = 0; irand < nrand; irand++ ) {
            scale *= scale0;
            sum += scale;
         }
         norm = 1.0 / ( sum * (double) ( m1 - 2L ) );

/* Note that local initialisation has been done. */
         init = 1;
      }

/* Obtain the seed value, enforcing positivity. */
      seed = (long int) context->seed;
      if ( seed < 1 ) seed = seed + LONG_MAX;
      if ( seed < 1 ) seed = LONG_MAX;

/* Initialise the random number generators with this seed. */
      context->rand1 = context->rand2 = seed;

/* Now loop to initialise the shuffle table with an initial set of
   random values. We generate more values than required in order to "warm
   up" the generator before recording values in the table. */
      for ( itab = ntab + nwarm - 1; itab >= 0; itab-- ) {

/* Repeatedly update "rand1" from the expression "(rand1*a1)%m1" while
   avoiding overflow. */
         tmp = context->rand1 / q1;
         context->rand1 = a1 * ( context->rand1 - tmp * q1 ) - tmp * r1;
         if ( context->rand1 < 0L ) context->rand1 += m1;

/* After warming up, start recording values in the table. */
         if ( itab < ntab ) context->table[ itab ] = context->rand1;
      }

/* Record the last entry in the table as the "previous" random
   integer. */
      context->random_int = context->table[ 0 ];

/* Note the random number generator context is active. */
      context->active = 1;
   }

/* Generate a random value. */
/* ------------------------ */
/* Initialise. */
   result = 0.0;

/* Loop to generate sufficient random integers to combine into a
   single double value. */
   scale = norm;
   for ( irand = 0; irand < nrand; irand++ ) {

/* Update the first generator "rand1" from the expression
   "(a1*rand1)%m1" while avoiding overflow. */
      tmp = context->rand1 / q1;
      context->rand1 = a1 * ( context->rand1 - tmp * q1 ) - tmp * r1;
      if ( context->rand1 < 0L ) context->rand1 += m1;

/* Similarly, update the second generator "rand2" from the expression
   "(a2*rand2)%m2". */
      tmp = context->rand2 / q2;
      context->rand2 = a2 * ( context->rand2 - tmp * q2 ) - tmp * r2;
      if ( context->rand2 < 0L ) context->rand2 += m2;

/* Use the previous random integer to generate an index into the
   shuffle table. */
      itab = (int) ( context->random_int /
                     ( 1L + ( m1 - 1L ) / (long int) ntab ) );

/* Extract the table entry and replace it with a new random value from
   the first generator "rand1". This is the Bays-Durham shuffle. */
      context->random_int = context->table[ itab ];
      context->table[ itab ] = context->rand1;

/* Combine the extracted value with the latest value from the second
   generator "rand2". */
      context->random_int -= context->rand2;
      if ( context->random_int < 1L ) context->random_int += m1 - 1L;

/* Update the scale factor to apply to the resulting random integer
   and accumulate its contribution to the result. */
      scale *= scale0;
      result += scale * (double) ( context->random_int - 1L );
   }

/* Return the result. */
   return result;
}

static void SetAttrib( AstObject *this_object, const char *setting ) {
/*
*  Name:
*     SetAttrib

*  Purpose:
*     Set an attribute value for a MathMap.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void SetAttrib( AstObject *this, const char *setting )

*  Class Membership:
*     MathMap member function (extends the astSetAttrib method inherited from
*     the Mapping class).

*  Description:
*     This function assigns an attribute value for a MathMap, the attribute
*     and its value being specified by means of a string of the form:
*
*        "attribute= value "
*
*     Here, "attribute" specifies the attribute name and should be in lower
*     case with no white space present. The value to the right of the "="
*     should be a suitable textual representation of the value to be assigned
*     and this will be interpreted according to the attribute's data type.
*     White space surrounding the value is only significant for string
*     attributes.

*  Parameters:
*     this
*        Pointer to the MathMap.
*     setting
*        Pointer to a null terminated string specifying the new attribute
*        value.

*  Returned Value:
*     void
*/

/* Local Vaiables: */
   AstMathMap *this;             /* Pointer to the MathMap structure */
   int ival;                     /* Integer attribute value */
   int len;                      /* Length of setting string */
   int nc;                       /* Number of characters read by sscanf */

/* Check the global error status. */
   if ( !astOK ) return;

/* Obtain a pointer to the MathMap structure. */
   this = (AstMathMap *) this_object;

/* Obtain the length of the setting string. */
   len = strlen( setting );

/* Test for each recognised attribute in turn, using "sscanf" to parse the
   setting string and extract the attribute value (or an offset to it in the
   case of string values). In each case, use the value set in "nc" to check
   that the entire string was matched. Once a value has been obtained, use the
   appropriate method to set it. */

/* Seed. */
/* ----- */
   if ( nc = 0,
        ( 1 == sscanf( setting, "seed= %d %n", &ival, &nc ) )
        && ( nc >= len ) ) {
      astSetSeed( this, ival );
      
/* SimpFI. */
/* ------- */
   } else if ( nc = 0,
               ( 1 == sscanf( setting, "simpfi= %d %n", &ival, &nc ) )
               && ( nc >= len ) ) {
      astSetSimpFI( this, ival );

/* SimpIF. */
/* ------- */
   } else if ( nc = 0,
               ( 1 == sscanf( setting, "simpif= %d %n", &ival, &nc ) )
               && ( nc >= len ) ) {
      astSetSimpIF( this, ival );

/* Pass any unrecognised setting to the parent method for further
   interpretation. */
   } else {
      (*parent_setattrib)( this_object, setting );
   }
}

static int TestAttrib( AstObject *this_object, const char *attrib ) {
/*
*  Name:
*     TestAttrib

*  Purpose:
*     Test if a specified attribute value is set for a MathMap.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     int TestAttrib( AstObject *this, const char *attrib )

*  Class Membership:
*     MathMap member function (over-rides the astTestAttrib protected
*     method inherited from the Mapping class).

*  Description:
*     This function returns a boolean result (0 or 1) to indicate whether
*     a value has been set for one of a MathMap's attributes.

*  Parameters:
*     this
*        Pointer to the MathMap.
*     attrib
*        Pointer to a null terminated string specifying the attribute
*        name.  This should be in lower case with no surrounding white
*        space.

*  Returned Value:
*     One if a value has been set, otherwise zero.

*  Notes:
*     - A value of zero will be returned if this function is invoked
*     with the global status set, or if it should fail for any reason.
*/

/* Local Variables: */
   AstMathMap *this;             /* Pointer to the MathMap structure */
   int result;                   /* Result value to return */

/* Initialise. */
   result = 0;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Obtain a pointer to the MathMap structure. */
   this = (AstMathMap *) this_object;

/* Check the attribute name and test the appropriate attribute. */

/* Seed. */
/* ----- */
   if ( !strcmp( attrib, "seed" ) ) {
      result = astTestSeed( this );

/* SimpFI. */
/* ------- */
   } else if ( !strcmp( attrib, "simpfi" ) ) {
      result = astTestSimpFI( this );

/* SimpIF. */
/* ------- */
   } else if ( !strcmp( attrib, "simpif" ) ) {
      result = astTestSimpIF( this );

/* If the attribute is not recognised, pass it on to the parent method
   for further interpretation. */
   } else {
      result = (*parent_testattrib)( this_object, attrib );
   }

/* Return the result, */
   return result;
}

static AstPointSet *Transform( AstMapping *map, AstPointSet *in,
                               int forward, AstPointSet *out ) {
/*
*  Name:
*     Transform

*  Purpose:
*     Apply a MathMap to transform a set of points.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     AstPointSet *Transform( AstMapping *map, AstPointSet *in,
*                             int forward, AstPointSet *out )

*  Class Membership:
*     MathMap member function (over-rides the astTransform method inherited
*     from the Mapping class).

*  Description:
*     This function takes a MathMap and a set of points encapsulated in a
*     PointSet and transforms the points so as to apply the required coordinate
*     transformation.

*  Parameters:
*     map
*        Pointer to the MathMap.
*     in
*        Pointer to the PointSet holding the input coordinate data.
*     forward
*        A non-zero value indicates that the forward coordinate transformation
*        should be applied, while a zero value requests the inverse
*        transformation.
*     out
*        Pointer to a PointSet which will hold the transformed (output)
*        coordinate values. A NULL value may also be given, in which case a
*        new PointSet will be created by this function.

*  Returned Value:
*     Pointer to the output (possibly new) PointSet.

*  Notes:
*     -  A null pointer will be returned if this function is invoked with the
*     global error status set, or if it should fail for any reason.
*     -  The number of coordinate values per point in the input PointSet must
*     match the number of coordinates for the MathMap being applied.
*     -  If an output PointSet is supplied, it must have space for sufficient
*     number of points and coordinate values per point to accommodate the
*     result. Any excess space will be ignored.
*/

/* Local Variables: */
   AstMathMap *this;             /* Pointer to MathMap to be applied */
   AstPointSet *result;          /* Pointer to output PointSet */
   double **ptr_in;              /* Pointer to input coordinate data */
   double **ptr_out;             /* Pointer to output coordinate data */
   int coord;                    /* Loop counter for coordinates */
   int ncoord_in;                /* Number of coordinates per input point */
   int ncoord_out;               /* Number of coordinates per output point */
   int npoint;                   /* Number of points */

/* Check the global error status. */
   if ( !astOK ) return NULL;

/* Obtain a pointer to the MathMap. */
   this = (AstMathMap *) map;

/* Apply the parent mapping using the stored pointer to the Transform member
   function inherited from the parent Mapping class. This function validates
   all arguments and generates an output PointSet if necessary, but does not
   actually transform any coordinate values. */
   result = (*parent_transform)( map, in, forward, out );

/* We will now extend the parent astTransform method by performing the
   permutation needed to generate the output coordinate values. */

/* Determine the numbers of points and coordinates per point from the input
   and output PointSets and obtain pointers for accessing the input and output
   coordinate values. */
   ncoord_in = astGetNcoord( in );
   ncoord_out = astGetNcoord( result );
   npoint = astGetNpoint( in );
   ptr_in = astGetPoints( in );      
   ptr_out = astGetPoints( result );

/* Determine whether to apply the forward or inverse transformation, according
   to the direction specified and whether the mapping has been inverted. */
   if ( astGetInvert( this ) ) forward = !forward;

/* Perform coordinate permutation. */
/* ------------------------------- */
   if ( astOK ) {

/* Loop to generate values for each output coordinate. */
      for ( coord = 0; coord < ncoord_out; coord++ ) {

/* Invoke the virtual machine that evaluates compiled
   expressions. Pass the appropriate code and constants arrays, depending
   on the direction of coordinate transformation, together with the
   required stack size. */
         VirtualMachine( &this->rcontext, npoint, ncoord_in, (const double **) ptr_in,
                         forward ? this->fwdcode[ coord ] :
                                   this->invcode[ coord ],
                         forward ? this->fwdcon[ coord ] :
                                   this->invcon[ coord ],
                         forward ? this->fwdstack : this->invstack,
                         ptr_out[ coord ] );
      }
   }

/* Return a pointer to the output PointSet. */
   return result;
}

static void ValidateSymbol( const char *method, const char *exprs,
                            int iend, int sym, int *lpar, int **argcount,
                            int **opensym, int *ncon, double **con ) {
/*
*  Name:
*     ValidateSymbol

*  Purpose:
*     Validate a symbol in an expression.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void ValidateSymbol( const char *method, const char *exprs,
*                          int iend, int sym, int *lpar, int **argcount,
*                          int **opensym, int *ncon, double **con )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function validates an identified standard symbol during
*     compilation of an expression. Its main task is to keep track of the
*     level of parenthesis in the expression and to count the number of
*     arguments supplied to functions at each level of parenthesis (for
*     nested function calls). On this basis it is able to interpret and
*     accept or reject symbols which represent function calls, parentheses
*     and delimiters. Other symbols are accepted automatically.

*  Parameters:
*     method
*        Pointer to a constant null-terminated character string
*        containing the name of the method that invoked this function.
*        This method name is used solely for constructing error messages.
*     exprs
*        Pointer to a null-terminated string containing the expression
*        being parsed. This is only used for constructing error messages.
*     iend
*        Index in "exprs" of the last character belonging to the most
*        recently identified symbol. This is only used for constructing error
*        messages.
*     sym
*        Index in the static "symbol" array of the most recently identified
*        symbol in the expression. This is the symbol to be verified.
*     lpar
*        Pointer to an int which holds the current level of parenthesis. On
*        the first invocation, this should be zero. The returned value should
*        be passed to subsequent invocations.
*     argcount
*        Address of a pointer to a dynamically allocated array of int in
*        which argument count information is maintained for each level of
*        parenthesis (e.g. for nested function calls). On the first invocation,
*        "*argcount" should be NULL. This function will allocate the required
*        space as needed and update this pointer. The returned pointer value
*        should be passed to subsequent invocations.
*
*        The allocated space must be freed by the caller (using astFree) when
*        no longer required.
*     opensym
*        Address of a pointer to a dynamically allocated array of int, in which
*        information is maintained about the functions associated with each
*        level of parenthesis (e.g. for nested function calls). On the first
*        invocation, "*opensym" should be NULL. This function will allocate the
*        required space as needed and update this pointer. The returned pointer
*        value should be passed to subsequent invocations.
*
*        The allocated space must be freed by the caller (using astFree) when
*        no longer required.
*     ncon
*        Pointer to an int which holds a count of the constants associated
*        with the expression (and determines the size of the "*con" array).
*        This function will update the count to reflect any new constants
*        appended to the "*con" array and the returned value should be passed
*        to subsequent invocations.
*     con
*        Address of a pointer to a dynamically allocated array of double, in
*        which the constants associated with the expression being parsed are
*        accumulated. On entry, "*con" should point at a dynamic array with
*        at least "*ncon" elements containing existing constants (or may be
*        NULL if no constants have yet been stored). This function will
*        allocate the required space as needed and update this pointer (and
*        "*ncon") appropriately. The returned pointer value should be passed
*        to subsequent invocations.
*
*        The allocated space must be freed by the caller (using astFree) when
*        no longer required.

*  Notes:
*     - The dynamically allocated arrays normally returned by this function
*     will be freed and NULL pointers will be returned if this function is
*     invoked with the global error status set, or if it should fail for any
*     reason.
*/

/* Check the global error status, but do not return at this point
   because dynamic arrays may require freeing. */
   if ( astOK ) {

/* Check if the symbol is a comma. */
      if ( ( symbol[ sym ].text[ 0 ] == ',' ) &&
           ( symbol[ sym ].text[ 1 ] == '\0' ) ) {

/* A comma is only used to delimit function arguments. If the current
   level of parenthesis is zero, or the symbol which opened the current
   level of parenthesis was not a function call (indicated by an argument
   count of zero at the current level of parenthesis), then report an
   error. */
         if ( ( *lpar <= 0 ) || ( ( *argcount )[ *lpar - 1 ] == 0 ) ) {
            astError( AST__COMIN,
                      "%s: Spurious comma encountered in the expression "
                      "\"%.*s\".",
                      method, iend + 1, exprs );

/* If a comma is valid, then increment the argument count at the
   current level of parenthesis. */
         } else {
            ( *argcount )[ *lpar - 1 ]++;
         }

/* If the symbol is not a comma, check if it increases the current
   level of parenthesis. */
      } else if ( symbol[ sym ].parincrement > 0 ) {

/* Increase the size of the arrays which hold parenthesis level
   information and check for errors. */
         *argcount = astGrow( *argcount, *lpar + 1, sizeof( int ) );
         *opensym = astGrow( *opensym, *lpar + 1, sizeof( int ) );
         if ( astOK ) {

/* Increment the level of parenthesis and initialise the argument
   count at the new level. This count is set to zero if the symbol which
   opens the parenthesis level is not a function call (indicated by a
   zero "nargs" entry in the symbol data), and it subsequently remains at
   zero. If the symbol is a function call, the argument count is
   initially set to 1 and increments whenever a comma is encountered at
   this parenthesis level. */
            ( *argcount )[ ++( *lpar ) - 1 ] = ( symbol[ sym ].nargs != 0 );

/* Remember the symbol which opened this parenthesis level. */
            ( *opensym )[ *lpar - 1 ] = sym;
         }

/* Check if the symbol decreases the current parenthesis level. */
      } else if ( symbol[ sym ].parincrement < 0 ) {

/* Ensure that the parenthesis level is not already at zero. If it is,
   then there is a missing left parenthesis in the expression being
   compiled, so report an error. */
         if ( *lpar == 0 ) {
            astError( AST__MLPAR,
                      "%s: Missing left parenthesis in the expression "
                      "\"%.*s\".",
                      method, iend + 1, exprs );

/* If the parenthesis level is valid and the symbol which opened this
   level of parenthesis was a function call with a fixed number of
   arguments (indicated by a positive "nargs" entry in the symbol data),
   then we must check the number of function arguments which have been
   encountered. */
         } else if ( symbol[ ( *opensym )[ *lpar - 1 ] ].nargs > 0 ) {

/* Report an error if the number of arguments is wrong. */
            if ( ( *argcount )[ *lpar - 1 ] !=
                 symbol[ ( *opensym )[ *lpar - 1 ] ].nargs ) {
               astError( AST__WRNFA,
                         "%s: Wrong number of function arguments in the "
                         "expression \"%.*s\".",
                         method, iend + 1, exprs );

/* If the number of arguments is valid, decrement the parenthesis
   level. */
            } else {
               ( *lpar )--;
            }

/* If the symbol which opened this level of parenthesis was a function
   call with a variable number of arguments (indicated by a negative
   "nargs" entry in the symbol data), then we must check and process the
   number of function arguments. */
         } else if ( symbol[ ( *opensym )[ *lpar - 1 ] ].nargs < 0 ) {

/* Check that the minimum required number of arguments have been
   supplied. Report an error if they have not. */
            if ( ( *argcount )[ *lpar - 1 ] <
                 ( -symbol[ ( *opensym )[ *lpar - 1 ] ].nargs ) ) {
               astError( AST__WRNFA,
                         "%s: Insufficient function arguments in the "
                         "expression \"%.*s\".",
                         method, iend + 1, exprs );

/* If the number of arguments is valid, increase the size of the
   constants array and check for errors. */
            } else {
               *con = astGrow( *con, *ncon + 1, sizeof( double ) );
               if ( astOK ) {

/* Append the argument count to the end of the array of constants and
   decrement the parenthesis level. */
                  ( *con )[ ( *ncon )++ ] =
                     (double) ( *argcount )[ --( *lpar ) ];
               }
            }

/* Finally, if the symbol which opened this level of parenthesis was
   not a function call ("nargs" entry in the symbol data is zero), then
   decrement the parenthesis level. In this case there is no need to
   check the argument count, because it will not have been
   incremented. */
         } else {
            ( *lpar )--;
         }
      }
   }

/* If an error occurred (or the global error status was set on entry),
   then reset the parenthesis level and free any memory which may have
   been allocated. */
   if ( !astOK ) {
      *lpar = 0;
      if ( *argcount ) *argcount = astFree( *argcount );
      if ( *opensym ) *opensym = astFree( *opensym );
      if ( *con ) *con = astFree( *con );
   }
}

static void VirtualMachine( Rcontext *rcontext, int npoint,
                            int ncoord_in, const double **ptr_in,
                            const int *code, const double *con, int stacksize,
                            double *out ) {
/*
*  Name:
*     VirtualMachine

*  Purpose:
*     Evaluate a function using a virtual machine.

*  Type:
*     Private function.

*  Synopsis:
*     #include "mathmap.h"
*     void VirtualMachine( int npoint, int ncoord_in, const double **ptr_in,
*                          const int *code, const double *con, int stacksize,
*                          double *out )

*  Class Membership:
*     MathMap member function.

*  Description:
*     This function implements a "virtual machine" which executes operations
*     on an arithmetic stack in order to evaluate transformation functions.
*     Each operation is specified by an input operation code (opcode) and
*     results in the execution of a vector operation on a stack. The final
*     result, after executing all the supplied opcodes, is returned as a
*     vector.
*
*     The virtual machine detects arithmetic errors (such as overflow and
*     division by zero) and propagates any "bad" coordinate values,
*     including those present in the input, to the output.

*  Parameters:
*     npoint
*        The number of points to be transformd (i.e. the size of the vector
*        of values on which operations are to be performed).
*     ncoord_in
*        The number of input coordinames per point.
*     ptr_in
*        Pointer to an array (with "ncoord_in" elements) of pointers to arrays
*        of double (with "npoint" elements). These arrays should contain the
*        input coordinate values, such that coordinate number "coord" for point
*        number "point" can be found in "ptr_in[coord][point]".
*     code
*        Pointer to an array of int containing the set of opcodes (cast to int)
*        for the operations to be performed. The first element of this array
*        should contain a count of the number of opcodes which follow.
*     con
*        Pointer to an array of double containing the set of constants required
*        to evaluate the function (this may be NULL if no constants are
*        required).
*     stacksize
*        The size of the stack required to evaluate the expression using the
*        opcodes and constants supplied. This value should be calculated during
*        expression compilation.
*     out
*        Pointer to an array of double (with "npoint" elements) in which to
*        return the vector of result values.
*/

/* Local Constants: */
   const int bits =              /* Number of bits in an unsigned long */
      sizeof( unsigned long ) * CHAR_BIT;
   const double eps =            /* Smallest number subtractable from 2.0 */
      2.0 * DBL_EPSILON;
   const double scale =          /* 2.0 raised to the power "bits" */
      ldexp( 1.0, bits );
   const double scale1 =         /* 2.0 raised to the power "bits-1" */
      scale * 0.5;
   const double rscale =         /* Reciprocal scale factor */
      1.0 / scale;
   const double rscale1 =        /* Reciprocal initial scale factor */
      1.0 / scale1;
   const int nblock =            /* Number of blocks of bits to process */
      ( sizeof( double ) + sizeof( unsigned long ) - 1 ) /
      sizeof( unsigned long );
   const unsigned long signbit = /* Mask for extracting sign bit */
      1UL << ( bits - 1 );

/* Local Variables: */
   double **stack;               /* Array of pointers to stack elements */
   double *work;                 /* Pointer to stack workspace */
   double *xv1;                  /* Pointer to first argument vector */
   double *xv2;                  /* Pointer to second argument vector */
   double *xv;                   /* Pointer to sole argument vector */
   double *y;                    /* Pointer to result */
   double *yv;                   /* Pointer to result vector */
   double abs1;                  /* Absolute value (temporary variable) */
   double abs2;                  /* Absolute value (temporary variable) */
   double frac1;                 /* First (maybe normalised) fraction */
   double frac2;                 /* Second (maybe normalised) fraction */
   double frac;                  /* Sole normalised fraction */
   double newexp;                /* New power of 2 exponent value */
   double pi;                    /* Value of PI */
   double ran;                   /* Random number */
   double result;                /* Function result value */
   double unscale;               /* Factor for removing scaling */
   double value;                 /* Value to be assigned to stack vector */
   double x1;                    /* First argument value */
   double x2;                    /* Second argument value */
   double x;                     /* Sole argument value */
   int expon1;                   /* First power of 2 exponent */
   int expon2;                   /* Second power of 2 exponent */
   int expon;                    /* Sole power of 2 exponent */
   int iarg;                     /* Loop counter for arguments */
   int iblock;                   /* Loop counter for blocks of bits */
   int icode;                    /* Opcode value */
   int icon;                     /* Counter for number of constants used */
   int istk;                     /* Loop counter for stack elements */
   int ivar;                     /* Input variable number */
   int narg;                     /* Number of function arguments */
   int ncode;                    /* Number of opcodes to process */
   int point;                    /* Loop counter for stack vector elements */
   int tos;                      /* Top of stack index */
   static double d2r;            /* Degrees to radians conversion factor */
   static double r2d;            /* Radians to degrees conversion factor */
   static int init = 0;          /* Initialisation performed? */
   unsigned long b1;             /* Block of bits from first argument */
   unsigned long b2;             /* Block of bits from second argument */
   unsigned long b;              /* Block of bits for result */
   unsigned long neg;            /* Result is negative? (sign bit) */

/* Check the global error status. */
   if ( !astOK ) return;

/* If this is the first invocation of this function, then initialise
   the trigonometrical conversion factors. */
   if ( !init ) {
      pi = acos( -1.0 );
      r2d = 180.0 / pi;
      d2r = pi / 180.0;

/* Note that initialisation has been performed. */
      init = 1;
   }

/* Allocate space for an array of pointers to elements of the
   workspace stack (each stack element being an array of double). */
   stack = astMalloc( sizeof( double * ) * (size_t) stacksize );

/* Allocate space for the stack itself. */
   work = astMalloc( sizeof( double ) *
                     (size_t) ( npoint * ( stacksize - 1 ) ) );

/* If OK, then initialise the stack pointer array to identify the
   start of each vector on the stack. The first element points at the
   output array (in which the result will be accumulated), while other
   elements point at successive vectors within the workspace allocated
   above. */
   if ( astOK ) {
      stack[ 0 ] = out;
      for ( istk = 1; istk < stacksize; istk++ ) {
         stack[ istk ] = work + ( istk - 1 ) * npoint;
      }

/* We now define a set of macros for performing vector operations on
   elements of the stack. Each is in the form of a "case" block for
   execution in response to the appropriate operation code (opcode). */

/* Zero-argument operation. */
/* ------------------------ */
/* This macro performs a zero-argument operation, which results in the
   insertion of a new vector on to the stack. */
#define ARG_0(oper,setup,function) \
\
/* Test for the required opcode value. */ \
   case oper: \
\
/* Perform any required initialisation. */ \
      {setup;} \
\
/* Increment the top of stack index and obtain a pointer to the new stack \
   element (vector). */ \
      yv = stack[ ++tos ]; \
\
/* Loop to access each vector element, obtaining a pointer to it. */ \
      for ( point = 0; point < npoint; point++ ) { \
         y = yv + point; \
\
/* Perform the processing, which results in assignment to this element. */ \
         {function;} \
      } \
\
/* Break out of the "case" block. */ \
      break;

/* One-argument operation. */
/* ----------------------- */
/* This macro performs a one-argument operation, which processes the
   top stack element without changing the stack size. */
#define ARG_1(oper,function) \
\
/* Test for the required opcode value. */ \
   case oper: \
\
/* Obtain a pointer to the top stack element (vector). */ \
      xv = stack[ tos ]; \
\
/* Loop to access each vector element, obtaining its value and \
   checking that it is not bad. */ \
      for ( point = 0; point < npoint; point++ ) { \
         if ( ( x = xv[ point ] ) != AST__BAD ) { \
\
/* Also obtain a pointer to the element. */ \
            y = xv + point; \
\
/* Perform the processing, which uses the element's value and then \
   assigns the result to this element. */ \
            {function;} \
         } \
      } \
\
/* Break out of the "case" block. */ \
      break;

/* One-argument boolean operation. */
/* ------------------------------- */
/* This macro is similar in function to ARG_1 above, except that no
   checks are made for bad argument values. It is intended for use with
   boolean functions where bad values are handled explicitly. */
#define ARG_1B(oper,function) \
\
/* Test for the required opcode value. */ \
   case oper: \
\
/* Obtain a pointer to the top stack element (vector). */ \
      xv = stack[ tos ]; \
\
/* Loop to access each vector element, obtaining the argument value \
   and a pointer to the element. */ \
      for ( point = 0; point < npoint; point++ ) { \
         x = xv[ point ]; \
         y = xv + point; \
\
/* Perform the processing, which uses the element's value and then \
   assigns the result to this element. */ \
         {function;} \
      } \
\
/* Break out of the "case" block. */ \
      break;

/* Two-argument operation. */
/* ----------------------- */
/* This macro performs a two-argument operation, which processes the
   top two stack elements and produces a single result, resulting in the
   stack size decreasing by one. In this case, we first define a macro
   without the "case" block statements present. */
#define DO_ARG_2(function) \
\
/* Obtain pointers to the top two stack elements (vectors), decreasing \
   the top of stack index by one. */ \
      xv2 = stack[ tos-- ]; \
      xv1 = stack[ tos ]; \
\
/* Loop to access each vector element, obtaining the value of the \
   first argument and checking that it is not bad. */ \
      for ( point = 0; point < npoint; point++ ) { \
         if ( ( x1 = xv1[ point ] ) != AST__BAD ) { \
\
/* Also obtain a pointer to the element which is to receive the \
   result. */ \
            y = xv1 + point; \
\
/* Obtain the value of the second argument, again checking that it is \
   not bad. */ \
            if ( ( x2 = xv2[ point ] ) != AST__BAD ) { \
\
/* Perform the processing, which uses the two argument values and then \
   assigns the result to the appropriate top of stack element. */ \
               {function;} \
\
/* If the second argument was bad, so is the result. */ \
            } else { \
               *y = AST__BAD; \
            } \
         } \
      }

/* This macro simply wraps the one above up in a "case" block. */
#define ARG_2(oper,function) \
   case oper: \
      DO_ARG_2(function) \
      break;

/* Two-argument boolean operation. */
/* ------------------------------- */
/* This macro is similar in function to ARG_2 above, except that no
   checks are made for bad argument values. It is intended for use with
   boolean functions where bad values are handled explicitly. */
#define ARG_2B(oper,function) \
\
/* Test for the required opcode value. */ \
   case oper: \
\
/* Obtain pointers to the top two stack elements (vectors), decreasing \
   the top of stack index by one. */ \
      xv2 = stack[ tos-- ]; \
      xv1 = stack[ tos ]; \
\
/* Loop to access each vector element, obtaining the value of both \
   arguments and a pointer to the element which is to receive the \
   result. */ \
      for ( point = 0; point < npoint; point++ ) { \
         x1 = xv1[ point ]; \
         x2 = xv2[ point ]; \
         y = xv1 + point; \
\
/* Perform the processing, which uses the two argument values and then \
   assigns the result to the appropriate top of stack element. */ \
         {function;} \
      } \
\
/* Break out of the "case" block. */ \
      break;

/* We now define some macros for performing mathematical operations in
   a "safe" way - i.e. trapping numerical problems such as overflow and
   invalid arguments and translating them into the AST__BAD value. */

/* Absolute value. */
/* --------------- */
/* This is just shorthand. */
#define ABS(x) ( ( (x) >= 0.0 ) ? (x) : -(x) )

/* Integer part. */
/* ------------- */
/* This implements rounding towards zero without involving conversion
   to an integer (which could overflow). */
#define INT(x) ( ( (x) >= 0.0 ) ? floor( (x) ) : ceil( (x) ) )

/* Trap maths overflow. */
/* -------------------- */
/* This macro calls a C maths library function and checks for overflow
   in the result. */
#define CATCH_MATHS_OVERFLOW(function) \
   ( \
\
/* Clear the "errno" value. */ \
      errno = 0, \
\
/* Evaluate the function. */ \
      result = (function), \
\
/* Check if "errno" and the returned result indicate overflow and \
   return the appropriate result. */ \
      ( ( errno == ERANGE ) && ( ABS( result ) == HUGE_VAL ) ) ? AST__BAD : \
                                                                 result \
   )

/* Trap maths errors. */
/* ------------------ */
/* This macro is similar to the one above, except that it also checks
   for domain errors (i.e. invalid argument values). */
#define CATCH_MATHS_ERROR(function) \
   ( \
\
/* Clear the "errno" value. */ \
      errno = 0, \
\
/* Evaluate the function. */ \
      result = (function), \
\
/* Check if "errno" and the returned result indicate a domain error or \
   overflow and return the appropriate result. */ \
      ( ( errno == EDOM ) || \
        ( ( errno == ERANGE ) && ( ABS( result ) == HUGE_VAL ) ) ) ? \
                                 AST__BAD : result \
   )

/* Tri-state boolean OR. */
/* --------------------- */
/* This evaluates a boolean OR using tri-state logic. For example,
   "a||b" may evaluate to 1 if "a" is bad but "b" is non-zero, so that
   the normal rules of bad value propagation do not apply. */
#define TRISTATE_OR(x1,x2) \
\
/* Test if the first argument is bad. */ \
   ( (x1) == AST__BAD ) ? ( \
\
/* If so, test the second argument. */ \
      ( ( (x2) == 0.0 ) || ( (x2) == AST__BAD ) ) ? AST__BAD : 1.0 \
   ) : ( \
\
/* Test if the second argument is bad. */ \
      ( (x2) == AST__BAD ) ? ( \
\
/* If so, test the first argument. */ \
         ( (x1) == 0.0 ) ? AST__BAD : 1.0 \
\
/* If neither argument is bad, use the normal OR operator. */ \
      ) : ( \
         ( (x1) != 0.0 ) || ( (x2) != 0.0 ) \
      ) \
   )

/* Tri-state boolean AND. */
/* ---------------------- */
/* This evaluates a boolean AND using tri-state logic. */
#define TRISTATE_AND(x1,x2) \
\
/* Test if the first argument is bad. */ \
   ( (x1) == AST__BAD ) ? ( \
\
/* If so, test the second argument. */ \
      ( (x2) != 0.0 ) ? AST__BAD : 0.0 \
   ) : ( \
\
/* Test if the second argument is bad. */ \
      ( (x2) == AST__BAD ) ? ( \
\
/* If so, test the first argument. */ \
         ( (x1) != 0.0 ) ? AST__BAD : 0.0 \
\
/* If neither argument is bad, use the normal AND operator. */ \
      ) : ( \
         ( (x1) != 0.0 ) && ( (x2) != 0.0 ) \
      ) \
   )

/* Safe addition. */
/* -------------- */
/* This macro performs addition while avoiding possible overflow. */
#define SAFE_ADD(x1,x2) ( \
\
/* Test if the first argument is non-negative. */ \
   ( (x1) >= 0.0 ) ? ( \
\
/* If so, then we can perform addition if the second argument is \
   non-positive. Otherwise, we must calculate the most positive safe \
   second argument value that can be added and test for this (the test \
   itself is safe against overflow). */ \
      ( ( (x2) <= 0.0 ) || ( ( (DBL_MAX) - (x1) ) >= (x2) ) ) ? ( \
\
/* Perform addition if it is safe, otherwise return AST__BAD. */ \
         (x1) + (x2) \
      ) : ( \
         AST__BAD \
      ) \
\
/* If the first argument is negative, then we can perform addition if \
   the second argument is non-negative. Otherwise, we must calculate the \
   most negative second argument value that can be added and test for \
   this (the test itself is safe against overflow). */ \
   ) : ( \
      ( ( (x2) >= 0.0 ) || ( ( (DBL_MAX) + (x1) ) >= -(x2) ) ) ? ( \
\
/* Perform addition if it is safe, otherwise return AST__BAD. */ \
         (x1) + (x2) \
      ) : ( \
         AST__BAD \
      ) \
   ) \
)

/* Safe subtraction. */
/* ----------------- */
/* This macro performs subtraction while avoiding possible overflow. */
#define SAFE_SUB(x1,x2) ( \
\
/* Test if the first argument is non-negative. */ \
   ( (x1) >= 0.0 ) ? ( \
\
/* If so, then we can perform subtraction if the second argument is \
   also non-negative. Otherwise, we must calculate the most negative safe \
   second argument value that can be subtracted and test for this (the \
   test itself is safe against overflow). */ \
      ( ( (x2) >= 0.0 ) || ( ( (DBL_MAX) - (x1) ) >= -(x2) ) ) ? ( \
\
/* Perform subtraction if it is safe, otherwise return AST__BAD. */ \
         (x1) - (x2) \
      ) : ( \
         AST__BAD \
      ) \
\
/* If the first argument is negative, then we can perform subtraction \
   if the second argument is non-positive. Otherwise, we must calculate \
   the most positive second argument value that can be subtracted and \
   test for this (the test itself is safe against overflow). */ \
   ) : ( \
      ( ( (x2) <= 0.0 ) || ( ( (DBL_MAX) + (x1) ) >= (x2) ) ) ? ( \
\
/* Perform subtraction if it is safe, otherwise return AST__BAD. */ \
         (x1) - (x2) \
      ) : ( \
         AST__BAD \
      ) \
   ) \
)

/* Safe multiplication. */
/* -------------------- */
/* This macro performs multiplication while avoiding possible overflow. */
#define SAFE_MUL(x1,x2) ( \
\
/* Multiplication is safe if the absolute value of either argument is \
   unity or less. Otherwise, we must use the first argument to calculate \
   the maximum absolute value that the second argument may have and test \
   for this (the test itself is safe against overflow). */ \
   ( ( ( abs1 = ABS( (x1) ) ) <= 1.0 ) || \
     ( ( abs2 = ABS( (x2) ) ) <= 1.0 ) || \
     ( ( (DBL_MAX) / abs1 ) >= abs2 ) ) ? ( \
\
/* Perform multiplication if it is safe, otherwise return AST__BAD. */ \
      (x1) * (x2) \
   ) : ( \
      AST__BAD \
   ) \
)

/* Safe division. */
/* -------------- */
/* This macro performs division while avoiding possible overflow. */
#define SAFE_DIV(x1,x2) ( \
\
/* Division is unsafe if the second argument is zero. Otherwise, it is \
   safe if the abolute value of the second argument is unity or \
   more. Otherwise, we must use the second argument to calculate the \
   maximum absolute value that the first argument may have and test for \
   this (the test itself is safe against overflow). */ \
   ( ( (x2) != 0.0 ) && \
     ( ( ( abs2 = ABS( (x2) ) ) >= 1.0 ) || \
       ( ( (DBL_MAX) * abs2 ) >= ABS( (x1) ) ) ) ) ? ( \
\
/* Perform division if it is safe, otherwise return AST__BAD. */ \
      (x1) / (x2) \
   ) : ( \
      AST__BAD \
   ) \
)

/* Bit shift operation. */
/* -------------------- */
#define SHIFT_BITS( x1, x2 ) ( \
   frac = frexp( (x1), &expon ), \
   newexp = INT( (x2) ) + (double) expon, \
   ( newexp < (double) -INT_MAX ) ? ( \
      0.0 \
   ) : ( ( newexp > (double) INT_MAX ) ? ( \
      AST__BAD \
   ) : ( \
      CATCH_MATHS_OVERFLOW( ldexp( frac, (int) newexp ) ) \
   ) ) \
)

/* Two-argument bit-wise boolean operation. */
/* ---------------------------------------- */
/* This macro expands to code which performs a bit-wise boolean
   operation on a pair of arguments and assigns the result to the
   variable "result". It operates on floating point (double) values,
   which are regarded as if they are fixed-point binary numbers with
   negative values expressed in twos-complement notation. This means that
   it delivers the same results for integer values as the normal
   (integer) C bit-wise operations. However, it will also operate on the
   fraction bits of floating point numbers. It also offers greater
   precision (the first 53 or so significant bits of the result being
   preserved for typical IEEE floating point implementations). */
#define BIT_OPER( oper, x1, x2 ) \
\
/* Convert each argument to a normalised fraction in the range \
   [0.5,1.0) and a power of two exponent, removing any sign \
   information. */ \
   frac1 = frexp( ABS( (x1) ), &expon1 ); \
   frac2 = frexp( ABS( (x2) ), &expon2 ); \
\
/* Set "expon" to be the larger of the two exponents. If the two \
   exponents are not equal, divide the fraction with the smaller exponent \
   by 2 to the power of the exponent difference. This gives both \
   fractions the same effective exponent (although one of them may no \
   longer be normalised). Note that overflow is avoided because all \
   numbers remain less than 1.0, but underflow may occur. */ \
   expon = expon1; \
   if ( expon2 > expon1 ) { \
      expon = expon2; \
      frac1 = ldexp( frac1, expon1 - expon ); \
   } else if ( expon1 > expon2 ) { \
      frac2 = ldexp( frac2, expon2 - expon ); \
   } \
\
/* If either of the original arguments is negative, we now subtract \
   the corresponding fraction from 2.0. If we think of the fraction as \
   represented in fixed-point binary notation, this corresponds to \
   converting negative numbers into the twos-complement form normally used \
   for integers (the sign bit being the bit with value 1) instead \
   of having a separate sign bit as for floating point numbers. \
\
   Note that one of the fractions may have underflowed during the \
   scaling above. In that case (if the original argument was negative), \
   we must subtract the value "eps" (= 2.0 * DBL_EPSILON) from 2.0 \
   instead, so that we produce the largest number less than 2.0. In \
   twos-complement notation this represents the smallest possible \
   negative number and corresponds to extending the sign bit of the \
   original number up into more significant bits. This causes all bits to \
   be set as we require (rather than all being clear if the underflow \
   is simply ignored). */ \
   if ( (x1) < 0.0 ) frac1 = 2.0 - ( ( frac1 > eps ) ? frac1 : eps ); \
   if ( (x2) < 0.0 ) frac2 = 2.0 - ( ( frac2 > eps ) ? frac2 : eps ); \
\
/* We now extract the bits from the fraction values into integer \
   variables so that we may perform bit-wise operations on them. However, \
   since a double may be longer than any available integer, we may \
   have to handle several successive blocks of bits individually. */ \
\
/* Extract the first block of bits by scaling by the required power of \
   2 to shift the required bits to the left of the binary point. Then \
   extract the integer part. Note that this initial shift is one bit less \
   than the number of bits in an unsigned long, because we have \
   introduced an extra sign bit. */ \
   frac1 *= scale1; \
   frac2 *= scale1; \
   b1 = (unsigned long) frac1; \
   b2 = (unsigned long) frac2; \
\
/* Perform the required bit-wise operation on the extracted blocks of \
   bits. */ \
   b = b1 oper b2; \
\
/* Extract the sign bit from this initial result. This determines \
   whether the final result bit pattern should represent a negative \
   floating point number. */ \
   neg = b & signbit; \
\
/* Initialise the floating point result by setting it to the integer \
   result multipled by the reciprocal of the scale factor used to shift \
   the bits above. This returns the result bits to their correct \
   significance. */ \
   unscale = rscale1; \
   result = (double) b * unscale; \
\
/* We now loop to extract and process further blocks of bits (if \
   present). The number of blocks is determined by the relative lengths \
   of a double and an unsigned long. In practice, some bits of the double \
   will be used by its exponent, so the last block may be incomplete and \
   will simply be padded with zeros. */ \
   for ( iblock = 1; iblock < nblock; iblock++ ) { \
\
/* Subtract the integer part (which has already been processed) from \
   each fraction, to leave the bits which remain to be processed. Then \
   multiply by a scale factor to shift the next set of bits to the left \
   of the binary point. This time, we use as many bits as will fit into \
   an unsigned long. */ \
      frac1 = ( frac1 - (double) b1 ) * scale; \
      frac2 = ( frac2 - (double) b2 ) * scale; \
\
/* Extract the integer part, which contains the required bits. */ \
      b1 = (unsigned long) frac1; \
      b2 = (unsigned long) frac2; \
\
/* Perform the required bit-wise operation on the extracted blocks of \
   bits. */ \
      b = b1 oper b2; \
\
/* Update the result floating point value by adding the new integer \
   result multiplied by a scale factor to return the bits to their \
   original significance. */ \
      unscale *= rscale; \
      result += (double) b * unscale; \
   } \
\
/* If the (normalised fraction) result represents a negative number, \
   then subtract 2.0 from it (equivalent to subtracting it from 2 and \
   negating the result). This converts back to using a separate sign bit \
   instead of twos-complement notation. */ \
   if ( neg ) result -= 2.0; \
\
/* Scale by the required power of 2 to remove the initial \
   normalisation applied and assign the result to the "result" \
   variable. */ \
   result = ldexp( result, expon )

/* Gaussian random number. */
/* ----------------------- */
/* This macro expands to code which assigns a pseudo-random value to
   the "result" variable. The value is drawn from a Gaussian distribution
   with mean "x1" and standard deviation "ABS(x2)". */
#define GAUSS( x1, x2 ) \
\
/* Loop until a satisfactory result is obtained. */ \
   do { \
\
/* Obtain a value drawn from a standard Gaussian distribution. */ \
      ran = Gauss( rcontext ); \
\
/* Multiply by "ABS(x2)", trapping possible overflow. */ \
      result = ABS( (x2) ); \
      result = SAFE_MUL( ran, result ); \
\
/* If OK, add "x1", again trapping possible overflow. */ \
      if ( result != AST__BAD ) result = SAFE_ADD( result, (x1) ); \
\
/* Continue generating values until one is found which does not cause \
   overflow. */ \
   } while ( result == AST__BAD );

/* All the required macros are now defined. */

/* Initialise the top of stack index and constant counter. */
      tos = -1;
      icon = 0;

/* Determine the number of opcodes to be processed and loop to process
   them, executing the appropriate "case" block for each one. */
      ncode = code[ 0 ];
      for ( icode = 1; icode <= ncode; icode++ ) {
         switch ( (Oper) code[ icode ] ) {

/* Ignore any null opcodes (which shouldn't occur). */
            case OP_NULL: break;

/* Otherwise, perform the required vector operation on the stack... */

/* User-supplied constants and variables. */
/* -------------------------------------- */
/* Loading a constant involves incrementing the constant count and
   assigning the next constant's value to the top of stack element. */
            ARG_0( OP_LDCON,    value = con[ icon++ ], *y = value )

/* Loading a variable involves obtaining the variable's index by
   consuming a constant (as above), and then copying the variable's
   values into the top of stack element. */
            ARG_0( OP_LDVAR,    ivar = (int) ( con[ icon++ ] + 0.5 ),
                                *y = ptr_in[ ivar ][ point ] )

/* System constants. */
/* ----------------- */
/* Loading a "bad" value simply means assigning AST__BAD to the top of
   stack element. */
            ARG_0( OP_LDBAD,    , *y = AST__BAD )

/* The following load constants associated with the (double) floating
   point representation into the top of stack element. */
            ARG_0( OP_LDDIG,    , *y = (double) DBL_DIG )
            ARG_0( OP_LDEPS,    , *y = DBL_EPSILON )
            ARG_0( OP_LDMAX,    , *y = DBL_MAX )
            ARG_0( OP_LDMAX10E, , *y = (double) DBL_MAX_10_EXP )
            ARG_0( OP_LDMAXE,   , *y = (double) DBL_MAX_EXP )
            ARG_0( OP_LDMDIG,   , *y = (double) DBL_MANT_DIG )
            ARG_0( OP_LDMIN,    , *y = DBL_MIN )
            ARG_0( OP_LDMIN10E, , *y = (double) DBL_MIN_10_EXP )
            ARG_0( OP_LDMINE,   , *y = (double) DBL_MIN_EXP )
            ARG_0( OP_LDRAD,    , *y = (double) FLT_RADIX )
            ARG_0( OP_LDRND,    , *y = (double) FLT_ROUNDS )

/* Mathematical constants. */
/* ----------------------- */
/* The following load mathematical constants into the top of stack
   element. */
            ARG_0( OP_LDE,      value = exp( 1.0 ), *y = value )
            ARG_0( OP_LDPI,     , *y = pi )

/* Functions with one argument. */
/* ---------------------------- */
/* The following simply evaluate a function of the top of stack
   element and assign the result to the same element. */
            ARG_1( OP_ABS,      *y = ABS( x ) )
            ARG_1( OP_ACOS,     *y = ( ABS( x ) <= 1.0 ) ?
                                     acos( x ) : AST__BAD )
            ARG_1( OP_ACOSD,    *y = ( ABS( x ) <= 1.0 ) ?
                                     acos( x ) * r2d : AST__BAD )
            ARG_1( OP_ASIN,     *y = ( ABS( x ) <= 1.0 ) ?
                                     asin( x ) : AST__BAD )
            ARG_1( OP_ASIND,    *y = ( ABS( x ) <= 1.0 ) ?
                                     asin( x ) * r2d : AST__BAD )
            ARG_1( OP_ATAN,     *y = atan( x ) )
            ARG_1( OP_ATAND,    *y = atan( x ) * r2d )
            ARG_1( OP_CEIL,     *y = ceil( x ) )
            ARG_1( OP_COS,      *y = cos( x ) )
            ARG_1( OP_COSD,     *y = cos( x * d2r ) )
            ARG_1( OP_COSH,     *y = CATCH_MATHS_OVERFLOW( cosh( x ) ) )
            ARG_1( OP_EXP,      *y = CATCH_MATHS_OVERFLOW( exp( x ) ) )
            ARG_1( OP_EXP2,     (void) frexp( x, &expon );
                                *y = (double) expon )
            ARG_1( OP_FLOOR,    *y = floor( x ) )
            ARG_1( OP_FR2,      *y = frexp( x, &expon ) )
            ARG_1( OP_INT,      *y = INT( x ) )
            ARG_1B( OP_ISBAD,   *y = ( x == AST__BAD ) )
            ARG_1( OP_LOG,      *y = ( x > 0.0 ) ? log( x ) : AST__BAD )
            ARG_1( OP_LOG10,    *y = ( x > 0.0 ) ? log10( x ) : AST__BAD )
            ARG_1( OP_NINT,     *y = ( x >= 0 ) ?
                                     floor( x + 0.5 ) : ceil( x - 0.5 ) )
            ARG_1( OP_POISS,    *y = Poisson( rcontext, x ) )
            ARG_1( OP_SIN,      *y = sin( x ) )
            ARG_1( OP_SIND,     *y = sin( x * d2r ) )
            ARG_1( OP_SINH,     *y = CATCH_MATHS_OVERFLOW( sinh( x ) ) )
            ARG_1( OP_SQRT,     *y = ( x >= 0.0 ) ? sqrt( x ) : AST__BAD )
            ARG_1( OP_TAN,      *y = CATCH_MATHS_OVERFLOW( tan( x ) ) )
            ARG_1( OP_TAND,     *y = tan( x * d2r ) )
            ARG_1( OP_TANH,     *y = tanh( x ) )

/* Functions with two arguments. */
/* ----------------------------- */
/* These evaluate a function of the top two entries on the stack. */
            ARG_2( OP_ATAN2,    *y = atan2( x1, x2 ) )
            ARG_2( OP_ATAN2D,   *y = atan2( x1, x2 ) * r2d )
            ARG_2( OP_DIM,      *y = ( x1 > x2 ) ? x1 - x2 : 0.0 )
            ARG_2( OP_GAUSS,    GAUSS( x1, x2 ); *y = result )
            ARG_2( OP_MOD,      *y = ( x2 != 0.0 ) ?
                                     fmod( x1, x2 ) : AST__BAD )
            ARG_2( OP_POW,      *y = CATCH_MATHS_ERROR( pow( x1, x2 ) ) )
            ARG_2( OP_RAND,     ran = Rand( rcontext );
                                *y = x1 * ran + x2 * ( 1.0 - ran ); )
            ARG_2( OP_SIGN,     *y = ( ( x1 >= 0.0 ) == ( x2 >= 0.0 ) ) ?
                                     x1 : -x1 )

/* Functions with variable numbers of arguments. */
/* --------------------------------------------- */
/* These operations take a variable number of arguments, the actual
   number being determined by consuming a constant. We then loop to
   perform a 2-argument operation on the stack (as above) the required
   number of times. */
            case OP_MAX:
               narg = (int) ( con[ icon++ ] + 0.5 );
               for ( iarg = 0; iarg < ( narg - 1 ); iarg++ ) {
                  DO_ARG_2( *y = ( x1 >= x2 ) ? x1 : x2 )
               }
               break;
            case OP_MIN:
               narg = (int) ( con[ icon++ ] + 0.5 );
               for ( iarg = 0; iarg < ( narg - 1 ); iarg++ ) {
                  DO_ARG_2( *y = ( x1 <= x2 ) ? x1 : x2 )
               }
               break;

/* Unary arithmetic operators. */
/* --------------------------- */
            ARG_1( OP_NEG,      *y = -x )

/* Unary boolean operators. */
/* ------------------------ */
            ARG_1( OP_NOT,      *y = ( x == 0.0 ) )

/* Binary arithmetic operators. */
/* ---------------------------- */
            ARG_2( OP_ADD,      *y = SAFE_ADD( x1, x2 ) )
            ARG_2( OP_SUB,      *y = SAFE_SUB( x1, x2 ) )
            ARG_2( OP_MUL,      *y = SAFE_MUL( x1, x2 ) )
            ARG_2( OP_DIV ,     *y = SAFE_DIV( x1, x2 ) )

/* Bit-shift operators. */
/* -------------------- */
            ARG_2( OP_SHFTL,    *y = SHIFT_BITS( x1, x2 ) )
            ARG_2( OP_SHFTR,    *y = SHIFT_BITS( x1, -x2 ) )

/* Relational operators. */
/* --------------------- */
            ARG_2( OP_EQ,       *y = ( x1 == x2 ) )
            ARG_2( OP_GE,       *y = ( x1 >= x2 ) )
            ARG_2( OP_GT,       *y = ( x1 > x2 ) )
            ARG_2( OP_LE,       *y = ( x1 <= x2 ) )
            ARG_2( OP_LT,       *y = ( x1 < x2 ) )
            ARG_2( OP_NE,       *y = ( x1 != x2 ) )

/* Bit-wise operators. */
/* ------------------- */
            ARG_2( OP_BITOR,    BIT_OPER( |, x1, x2 ); *y = result )
            ARG_2( OP_BITXOR,   BIT_OPER( ^, x1, x2 ); *y = result )
            ARG_2( OP_BITAND,   BIT_OPER( &, x1, x2 ); *y = result )

/* Binary boolean operators. */
/* ------------------------- */
            ARG_2B( OP_AND,     *y = TRISTATE_AND( x1, x2 ) )
            ARG_2( OP_EQV,      *y = ( ( x1 != 0.0 ) == ( x2 != 0.0 ) ) )
            ARG_2B( OP_OR,      *y = TRISTATE_OR( x1, x2 ) )
            ARG_2( OP_XOR,      *y = ( ( x1 != 0.0 ) != ( x2 != 0.0 ) ) )
         }
      }
   }

/* When all opcodes have been processed, the result of the function
   evaluation will reside in the lowest stack entry - i.e. the output
   array. */

/* Free the workspace arrays. */
   work = astFree( work );
   stack = astFree( stack );

/* Undefine macros local to this function. */
#undef ARG_0
#undef ARG_1
#undef ARG_1B
#undef DO_ARG_2
#undef ARG_2
#undef ARG_2B
#undef ABS
#undef INT
#undef CATCH_MATHS_OVERFLOW
#undef CATCH_MATHS_ERROR
#undef TRISTATE_OR
#undef TRISTATE_AND
#undef SAFE_ADD
#undef SAFE_SUB
#undef SAFE_MUL
#undef SAFE_DIV
#undef BIT_OPER
#undef SHIFT_BITS
#undef GAUSS
}

/* Functions which access class attributes. */
/* ---------------------------------------- */
/* Implement member functions to access the attributes associated with
   this class using the macros defined for this purpose in the
   "object.h" file. For a description of each attribute, see the class
   interface (in the associated .h file). */

/* Clear the Seed value by setting it to a new unpredictable value
   produced by DefaultSeed and clearing the "seed_set" flag in the
   MathMap's random number generator context. Also clear the "active"
   flag, so that the generator will be re-initialised to use this seed
   when it is next invoked. */
astMAKE_CLEAR(MathMap,Seed,rcontext.seed,( this->rcontext.seed_set = 0,
                                           this->rcontext.active = 0,
                                           DefaultSeed( &this->rcontext ) ))

/* Return the "seed" value from the random number generator
   context. */
astMAKE_GET(MathMap,Seed,int,0,this->rcontext.seed)

/* Store the new seed value in the MathMap's random number generator
   context and set the context's "seed_set" flag. Also clear the "active"
   flag, so that the generator will be re-initialised to use this seed
   when it is next invoked. */
astMAKE_SET(MathMap,Seed,int,rcontext.seed,( this->rcontext.seed_set = 1,
                                             this->rcontext.active = 0,
                                             value ))

/* Test the "seed_set" flag in the random number generator context. */
astMAKE_TEST(MathMap,Seed,( this->rcontext.seed_set ))


/* Clear the SimpFI value by setting it to -INT_MAX. */
astMAKE_CLEAR(MathMap,SimpFI,simp_fi,-INT_MAX)

/* Supply a default of 0 if no SimpFI value has been set. */
astMAKE_GET(MathMap,SimpFI,int,0,( ( this->simp_fi != -INT_MAX ) ?
                                   this->simp_fi : 0 ))

/* Set a SimpFI value of 1 if any non-zero value is supplied. */
astMAKE_SET(MathMap,SimpFI,int,simp_fi,( value != 0 ))

/* The SimpFI value is set if it is not -INT_MAX. */
astMAKE_TEST(MathMap,SimpFI,( this->simp_fi != -INT_MAX ))


/* Clear the SimpIF value by setting it to -INT_MAX. */
astMAKE_CLEAR(MathMap,SimpIF,simp_if,-INT_MAX)

/* Supply a default of 0 if no SimpIF value has been set. */
astMAKE_GET(MathMap,SimpIF,int,0,( ( this->simp_if != -INT_MAX ) ?
                                   this->simp_if : 0 ))

/* Set a SimpIF value of 1 if any non-zero value is supplied. */
astMAKE_SET(MathMap,SimpIF,int,simp_if,( value != 0 ))

/* The SimpIF value is set if it is not -INT_MAX. */
astMAKE_TEST(MathMap,SimpIF,( this->simp_if != -INT_MAX ))

/* Copy constructor. */
/* ----------------- */
static void Copy( const AstObject *objin, AstObject *objout ) {
/*
*  Name:
*     Copy

*  Purpose:
*     Copy constructor for MathMap objects.

*  Type:
*     Private function.

*  Synopsis:
*     void Copy( const AstObject *objin, AstObject *objout )

*  Description:
*     This function implements the copy constructor for MathMap objects.

*  Parameters:
*     objin
*        Pointer to the object to be copied.
*     objout
*        Pointer to the object being constructed.

*  Returned Value:
*     void

*  Notes:
*     -  This constructor makes a deep copy.
*/

/* Local Variables: */
   AstMathMap *in;               /* Pointer to input MathMap */
   AstMathMap *out;              /* Pointer to output MathMap */
   int ifun;                     /* Loop counter for functions */

/* Check the global error status. */
   if ( !astOK ) return;

/* Obtain pointers to the input and output MathMaps. */
   in = (AstMathMap *) objin;
   out = (AstMathMap *) objout;

/* For safety, first clear any references to the input memory from
   the output MathMap. */
   out->fwdfun = NULL;
   out->invfun = NULL;
   out->fwdcode = NULL;
   out->invcode = NULL;
   out->fwdcon = NULL;
   out->invcon = NULL;

/* Now allocate and initialise each of the output pointer arrays
   required. */
   if ( in->fwdfun ) {
      MALLOC_POINTER_ARRAY( out->fwdfun, char *, out->nfwd )
   }
   if ( in->invfun ) {
      MALLOC_POINTER_ARRAY( out->invfun, char *, out->ninv )
   }
   if ( in->fwdcode ) {
      MALLOC_POINTER_ARRAY( out->fwdcode, int *, out->nfwd )
   }
   if ( in->invcode ) {
      MALLOC_POINTER_ARRAY( out->invcode, int *, out->ninv )
   }
   if ( in->fwdcon ) {
      MALLOC_POINTER_ARRAY( out->fwdcon, double *, out->nfwd )
   }
   if ( in->invcon ) {
      MALLOC_POINTER_ARRAY( out->invcon, double *, out->ninv )
   }

/* If OK, loop to make copies of the data (where available) associated
   with each forward transformation function, storing pointers to the
   copy in the output pointer arrays allocated above. */
   if ( astOK ) {
      for ( ifun = 0; ifun < out->nfwd; ifun++ ) {
         if ( in->fwdfun && in->fwdfun[ ifun ] ) {
            out->fwdfun[ ifun ] = astStore( NULL, in->fwdfun[ ifun ],
                                            astSizeOf( in->fwdfun[ ifun ] ) );
         }
         if ( in->fwdcode && in->fwdcode[ ifun ] ) {
            out->fwdcode[ ifun ] = astStore( NULL, in->fwdcode[ ifun ],
                                            astSizeOf( in->fwdcode[ ifun ] ) );
         }
         if ( in->fwdcon && in->fwdcon[ ifun ] ) {
            out->fwdcon[ ifun ] = astStore( NULL, in->fwdcon[ ifun ],
                                            astSizeOf( in->fwdcon[ ifun ] ) );
         }
         if ( !astOK ) break;
      }
   }

/* Repeat this process for the inverse transformation functions. */
   if ( astOK ) {
      for ( ifun = 0; ifun < out->ninv; ifun++ ) {
         if ( in->invfun && in->invfun[ ifun ] ) {
            out->invfun[ ifun ] = astStore( NULL, in->invfun[ ifun ],
                                            astSizeOf( in->invfun[ ifun ] ) );
         }
         if ( in->invcode && in->invcode[ ifun ] ) {
            out->invcode[ ifun ] = astStore( NULL, in->invcode[ ifun ],
                                            astSizeOf( in->invcode[ ifun ] ) );
         }
         if ( in->invcon && in->invcon[ ifun ] ) {
            out->invcon[ ifun ] = astStore( NULL, in->invcon[ ifun ],
                                            astSizeOf( in->invcon[ ifun ] ) );
         }
         if ( !astOK ) break;
      }
   }

/* If an error occurred, clean up by freeing all output memory
   allocated above. */
   if ( !astOK ) {
      FREE_POINTER_ARRAY( out->fwdfun, out->nfwd )
      FREE_POINTER_ARRAY( out->invfun, out->ninv )
      FREE_POINTER_ARRAY( out->fwdcode, out->nfwd )
      FREE_POINTER_ARRAY( out->invcode, out->ninv )
      FREE_POINTER_ARRAY( out->fwdcon, out->nfwd )
      FREE_POINTER_ARRAY( out->invcon, out->ninv )
   }
}

/* Destructor. */
/* ----------- */
static void Delete( AstObject *obj ) {
/*
*  Name:
*     Delete

*  Purpose:
*     Destructor for MathMap objects.

*  Type:
*     Private function.

*  Synopsis:
*     void Delete( AstObject *obj )

*  Description:
*     This function implements the destructor for MathMap objects.

*  Parameters:
*     obj
*        Pointer to the object to be deleted.

*  Returned Value:
*     void

*  Notes:
*     This function attempts to execute even if the global error status is
*     set.
*/

/* Local Variables: */
   AstMathMap *this;             /* Pointer to MathMap */

/* Obtain a pointer to the MathMap structure. */
   this = (AstMathMap *) obj;

/* Free all memory allocated by the MathMap. (Note that the number of
   forward and inverse functions are stored as instance variables of this
   class so that they can be obtained reliably under error conditions.) */
   FREE_POINTER_ARRAY( this->fwdfun, this->nfwd )
   FREE_POINTER_ARRAY( this->invfun, this->ninv )
   FREE_POINTER_ARRAY( this->fwdcode, this->nfwd )
   FREE_POINTER_ARRAY( this->invcode, this->ninv )
   FREE_POINTER_ARRAY( this->fwdcon, this->nfwd )
   FREE_POINTER_ARRAY( this->invcon, this->ninv )
}

/* Dump function. */
/* -------------- */
static void Dump( AstObject *this_object, AstChannel *channel ) {
/*
*  Name:
*     Dump

*  Purpose:
*     Dump function for MathMap objects.

*  Type:
*     Private function.

*  Synopsis:
*     void Dump( AstObject *this, AstChannel *channel )

*  Description:
*     This function implements the Dump function which writes out data
*     for the MathMap class to an output Channel.

*  Parameters:
*     this
*        Pointer to the MathMap whose data are being written.
*     channel
*        Pointer to the Channel to which the data are being written.
*/

/* Local Constants: */
#define COMMENT_LEN 150          /* Maximum length of a comment string */
#define KEY_LEN 50               /* Maximum length of a keyword */

/* Local Variables: */
   AstMathMap *this;             /* Pointer to the MathMap structure */
   char comment[ COMMENT_LEN + 1 ]; /* Buffer for comment strings */
   char key[ KEY_LEN + 1 ];      /* Buffer for keyword strings */
   int ifun;                     /* Loop counter for functions */
   int ival;                     /* Integer attribute value */
   int set;                      /* Attribute value set? */

/* Check the global error status. */
   if ( !astOK ) return;

/* Obtain a pointer to the MathMap structure. */
   this = (AstMathMap *) this_object;

/* Write out values representing the instance variables for the
   MathMap class.  Accompany these with appropriate comment strings,
   possibly depending on the values being written.*/

/* In the case of attributes, we first use the appropriate (private)
   Test...  member function to see if they are set. If so, we then use
   the (private) Get... function to obtain the value to be written
   out.

   For attributes which are not set, we use the astGet... method to
   obtain the value instead. This will supply a default value
   (possibly provided by a derived class which over-rides this method)
   which is more useful to a human reader as it corresponds to the
   actual default attribute value.  Since "set" will be zero, these
   values are for information only and will not be read back. */

/* Forward transformation functions. */
/* --------------------------------- */
/* Loop to write out each forward transformation function, generating
   a suitable keyword and comment for each one. */
   for ( ifun = 0; ifun < this->nfwd; ifun++ ) {
      (void) sprintf( key, "F%d", ifun + 1 );
      (void) sprintf( comment,
                      !ifun ? "Forward function %d" :
                              "   \"        \"    %d", ifun + 1 );
      astWriteString( channel, key, 1, 1, this->fwdfun[ ifun ], comment );
   }

/* Inverse transformation functions. */
/* --------------------------------- */
/* Similarly, loop to write out each inverse transformation
   function. */
   for ( ifun = 0; ifun < this->ninv; ifun++ ) {
      (void) sprintf( key, "I%d", ifun + 1 );
      (void) sprintf( comment,
                      !ifun ? "Inverse function %d" :
                              "   \"        \"    %d", ifun + 1 );
      astWriteString( channel, key, 1, 1, this->invfun[ ifun ], comment );
   }

/* SimpFI. */
/* ------- */
/* Write out the forward-inverse simplification flag. */
   set = TestSimpFI( this );
   ival = set ? GetSimpFI( this ) : astGetSimpFI( this );
   astWriteInt( channel, "SimpFI", set, 0, ival,
                ival ? "Forward-inverse pairs may simplify" :
                       "Forward-inverse pairs do not simplify" );

/* SimpIF. */
/* ------- */
/* Write out the inverse-forward simplification flag. */
   set = TestSimpIF( this );
   ival = set ? GetSimpIF( this ) : astGetSimpIF( this );
   astWriteInt( channel, "SimpIF", set, 0, ival,
                ival ? "Inverse-forward pairs may simplify" :
                       "Inverse-forward pairs do not simplify" );

/* Seed. */
/* ----- */
/* Write out any random number seed value which is set. Prefix this with
   a separate flag which indicates if the seed has been set. */
   set = TestSeed( this );
   ival = set ? GetSeed( this ) : astGetSeed( this );
   astWriteInt( channel, "Seeded", set, 0, set,
                set? "Explicit random number seed set" :
                     "No random number seed set" );
   astWriteInt( channel, "Seed", set, 0, ival,
                set ? "Random number seed value" :
                      "Default random number seed used" );

/* Undefine macros local to this function. */
#undef COMMENT_LEN
#undef KEY_LEN
}

/* Standard class functions. */
/* ========================= */
/* Implement the astIsAMathMap and astCheckMathMap functions using the macros
   defined for this purpose in the "object.h" header file. */
astMAKE_ISA(MathMap,Mapping,check,&class_init)
astMAKE_CHECK(MathMap)

AstMathMap *astMathMap_( int nin, int nout,
                         const char *fwd[], const char *inv[],
                         const char *options, ... ) {
/*
*+
*  Name:
*     astMathMap

*  Purpose:
*     Create a MathMap.

*  Type:
*     Protected function.

*  Synopsis:
*     #include "mathmap.h"
*     AstMathMap *astMathMap( int nin, int nout,
*                             const char *fwd[], const char *inv[],
*                             const char *options, ... )

*  Class Membership:
*     MathMap constructor.

*  Description:
*     This function creates a new MathMap and optionally initialises its
*     attributes.

*  Parameters:
*     nin
*        Number of input variables for the MathMap.
*     nout
*        Number of output variables for the MathMap.
*     fwd
*        Pointer to an array, with "nout" elements, of pointers to null
*        terminated strings which contain each of the forward transformation
*        functions.
*     inv
*        Pointer to an array, with "nin" elements, of pointers to null
*        terminated strings which contain each of the inverse transformation
*        functions.
*     options
*        Pointer to a null terminated string containing an optional
*        comma-separated list of attribute assignments to be used for
*        initialising the new MathMap. The syntax used is the same as
*        for the astSet method and may include "printf" format
*        specifiers identified by "%" symbols in the normal way.
*     ...
*        If the "options" string contains "%" format specifiers, then
*        an optional list of arguments may follow it in order to
*        supply values to be substituted for these specifiers. The
*        rules for supplying these are identical to those for the
*        astSet method (and for the C "printf" function).

*  Returned Value:
*     A pointer to the new MathMap.

*  Notes:
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-

*  Implementation Notes:
*     - This function implements the basic MathMap constructor which is
*     available via the protected interface to the MathMap class.  A
*     public interface is provided by the astMathMapId_ function.
*/

/* Local Variables: */
   AstMathMap *new;              /* Pointer to new MathMap */
   va_list args;                 /* Variable argument list */

/* Check the global status. */
   if ( !astOK ) return NULL;

/* Initialise the MathMap, allocating memory and initialising the
   virtual function table as well if necessary. */
   new = astInitMathMap( NULL, sizeof( AstMathMap ), !class_init, &class_vtab,
                         "MathMap", nin, nout, fwd, inv );

/* If successful, note that the virtual function table has been
   initialised. */
   if ( astOK ) {
      class_init = 1;

/* Obtain the variable argument list and pass it along with the options string
   to the astVSet method to initialise the new MathMap's attributes. */
      va_start( args, options );
      astVSet( new, options, args );
      va_end( args );

/* If an error occurred, clean up by deleting the new object. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return a pointer to the new MathMap. */
   return new;
}

AstMathMap *astMathMapId_( int nin, int nout,
                           const char *fwd[], const char *inv[],
                           const char *options, ... ) {
/*
*++
*  Name:
c     astMathMap
f     AST_MATHMAP

*  Purpose:
*     Create a MathMap.

*  Type:
*     Public function.

*  Synopsis:
c     #include "mathmap.h"
c     AstMathMap *astMathMap( int nin, int nout, const char *fwd[],
c                             const char *inv[], const char *options, ... )
f     RESULT = AST_MATHMAP( NIN, NOUT, FWD, INV, OPTIONS, STATUS )

*  Class Membership:
*     MathMap constructor.

*  Description:
*     This function creates a new MathMap and optionally initialises its
*     attributes.

*  Parameters:
c     nin
f     NIN = INTEGER
*        Number of input variables for the MathMap.
c     nout
f     NOUT = INTEGER
*        Number of output variables for the MathMap.
c     fwd
f     FWD = CHARACTER * ( * )( NOUT )
c        Pointer to an array, with "nout" elements, of pointers to null
c        terminated strings which contain each of the forward transformation
c        functions.
f        An array containing each of the forward transformation
f        functions.
c     inv
f     INV = CHARACTER * ( * )( NIN )
c        Pointer to an array, with "nin" elements, of pointers to null
c        terminated strings which contain each of the inverse transformation
c        functions.
f        An array containing each of the inverse transformation
f        functions.
c     options
f     OPTIONS = CHARACTER * ( * ) (Given)
c        Pointer to a null-terminated string containing an optional
c        comma-separated list of attribute assignments to be used for
c        initialising the new MathMap. The syntax used is identical to
c        that for the astSet function and may include "printf" format
c        specifiers identified by "%" symbols in the normal way.
c        If no initialisation is required, a zero-length string may be
c        supplied.
f        A character string containing an optional comma-separated
f        list of attribute assignments to be used for initialising the
f        new MathMap. The syntax used is identical to that for the
f        AST_SET routine. If no initialisation is required, a blank
f        value may be supplied.
c     ...
c        If the "options" string contains "%" format specifiers, then
c        an optional list of additional arguments may follow it in
c        order to supply values to be substituted for these
c        specifiers. The rules for supplying these are identical to
c        those for the astSet function (and for the C "printf"
c        function).
f     STATUS = INTEGER (Given and Returned)
f        The global status.

*  Returned Value:
c     astMathMap()
f     AST_MATHMAP = INTEGER
*        A pointer to the new MathMap.

*  Notes:
*     - A null Object pointer (AST__NULL) will be returned if this
c     function is invoked with the AST error status set, or if it
f     function is invoked with STATUS set to an error value, or if it
*     should fail for any reason.
*--

*  Implementation Notes:
*     - This function implements the external (public) interface to
*     the astMathMap constructor function. It returns an ID value
*     (instead of a true C pointer) to external users, and must be
*     provided because astMathMap_ has a variable argument list which
*     cannot be encapsulated in a macro (where this conversion would
*     otherwise occur).
*     - The variable argument list also prevents this function from
*     invoking astMathMap_ directly, so it must be a re-implementation
*     of it in all respects, except for the final conversion of the
*     result to an ID value.
*/

/* Local Variables: */
   AstMathMap *new;                /* Pointer to new MathMap */
   va_list args;                 /* Variable argument list */

/* Check the global error status. */
   if ( !astOK ) return NULL;

/* Initialise the MathMap, allocating memory and initialising the virtual
   function table as well if necessary. */
   new = astInitMathMap( NULL, sizeof( AstMathMap ), !class_init, &class_vtab,
                         "MathMap", nin, nout, fwd, inv );

/* If successful, note that the virtual function table has been initialised. */
   if ( astOK ) {
      class_init = 1;

/* Obtain the variable argument list and pass it along with the options string
   to the astVSet method to initialise the new MathMap's attributes. */
      va_start( args, options );
      astVSet( new, options, args );
      va_end( args );

/* If an error occurred, clean up by deleting the new object. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return an ID value for the new MathMap. */
   return astMakeId( new );
}

AstMathMap *astInitMathMap_( void *mem, size_t size, int init,
                             AstMathMapVtab *vtab, const char *name,
                             int nin, int nout,
                             const char *fwd[], const char *inv[] ) {
/*
*+
*  Name:
*     astInitMathMap

*  Purpose:
*     Initialise a MathMap.

*  Type:
*     Protected function.

*  Synopsis:
*     #include "mathmap.h"
*     AstMathMap *astInitMathMap_( void *mem, size_t size, int init,
*                                  AstMathMapVtab *vtab, const char *name,
*                                  int nin, int nout,
*                                  const char *fwd[], const char *inv[] )

*  Class Membership:
*     MathMap initialiser.

*  Description:
*     This function is provided for use by class implementations to initialise
*     a new MathMap object. It allocates memory (if necessary) to accommodate
*     the MathMap plus any additional data associated with the derived class.
*     It then initialises a MathMap structure at the start of this memory. If
*     the "init" flag is set, it also initialises the contents of a virtual
*     function table for a MathMap at the start of the memory passed via the
*     "vtab" parameter.

*  Parameters:
*     mem
*        A pointer to the memory in which the MathMap is to be initialised.
*        This must be of sufficient size to accommodate the MathMap data
*        (sizeof(MathMap)) plus any data used by the derived class. If a value
*        of NULL is given, this function will allocate the memory itself using
*        the "size" parameter to determine its size.
*     size
*        The amount of memory used by the MathMap (plus derived class data).
*        This will be used to allocate memory if a value of NULL is given for
*        the "mem" parameter. This value is also stored in the MathMap
*        structure, so a valid value must be supplied even if not required for
*        allocating memory.
*     init
*        A logical flag indicating if the MathMap's virtual function table is
*        to be initialised. If this value is non-zero, the virtual function
*        table will be initialised by this function.
*     vtab
*        Pointer to the start of the virtual function table to be associated
*        with the new MathMap.
*     name
*        Pointer to a constant null-terminated character string which contains
*        the name of the class to which the new object belongs (it is this
*        pointer value that will subsequently be returned by the Object
*        astClass function).
*     nin
*        Number of input variables for the MathMap.
*     nout
*        Number of output variables for the MathMap.
*     fwd
*        Pointer to an array, with "nout" elements, of pointers to null
*        terminated strings which contain each of the forward transformation
*        functions.
*     inv
*        Pointer to an array, with "nin" elements, of pointers to null
*        terminated strings which contain each of the inverse transformation
*        functions.

*  Returned Value:
*     A pointer to the new MathMap.

*  Notes:
*     -  This function does not attempt to ensure that the forward and inverse
*     transformations performed by the resulting MathMap are consistent in any
*     way.
*     -  This function makes a copy of the contents of the strings supplied.
*     -  A null pointer will be returned if this function is invoked with the
*     global error status set, or if it should fail for any reason.
*-
*/

/* Local Variables: */
   AstMathMap *new;              /* Pointer to new MathMap */
   char **fwdfun;                /* Array of cleaned forward functions */
   char **invfun;                /* Array of cleaned inverse functions */
   double **fwdcon;              /* Constants for forward functions */
   double **invcon;              /* Constants for inverse functions */
   int **fwdcode;                /* Code for forward functions */
   int **invcode;                /* Code for inverse functions */
   int fwdstack;                 /* Stack size for forward functions */
   int invstack;                 /* Stack size for inverse functions */

/* Initialise. */
   new = NULL;

/* Check the global status. */
   if ( !astOK ) return new;

/* Clean the forward and inverse functions provided. This makes a
   lower-case copy with white space removed. */
   CleanFunctions( nout, fwd, &fwdfun );
   CleanFunctions( nin, inv, &invfun );

/* Compile the cleaned functions. From the returned pointers (if
   successful), we can now tell which transformations (forward and/or
   inverse) are defined. */
   CompileMapping( "astInitMathMap", nin, nout,
                   (const char **) fwdfun, (const char **) invfun,
                   &fwdcode, &invcode, &fwdcon, &invcon,
                   &fwdstack, &invstack );

/* Initialise a Mapping structure (the parent class) as the first
   component within the MathMap structure, allocating memory if
   necessary. Specify that the Mapping should be defined in the required
   directions. */
   new = (AstMathMap *) astInitMapping( mem, size, init,
                                        (AstMappingVtab *) vtab, name,
                                        nin, nout,
                                        ( fwdcode != NULL ),
                                        ( invcode != NULL ) );

/* If necessary, initialise the virtual function table. */
/* ---------------------------------------------------- */
   if ( init ) InitVtab( vtab );

/* If an error has occurred, free all the memory which may have been
   allocated by the cleaning and compilation steps above. */
   if ( !astOK ) {
      FREE_POINTER_ARRAY( fwdfun, nout )
      FREE_POINTER_ARRAY( invfun, nin )
      FREE_POINTER_ARRAY( fwdcode, nout )
      FREE_POINTER_ARRAY( invcode, nin )
      FREE_POINTER_ARRAY( fwdcon, nout )
      FREE_POINTER_ARRAY( invcon, nin )
   }

/* Initialise the MathMap data. */
/* ---------------------------- */
   if ( new ) {
      new->fwdfun = fwdfun;
      new->invfun = invfun;
      new->fwdcode = fwdcode;
      new->invcode = invcode;
      new->fwdcon = fwdcon;
      new->invcon = invcon;
      new->fwdstack = fwdstack;
      new->invstack = invstack;
      new->nfwd = nout;
      new->ninv = nin;
      new->simp_fi = -INT_MAX;
      new->simp_if = -INT_MAX;

/* Initialise the random number generator context associated with the
   MathMap. */
      new->rcontext.random_int = 0;
      new->rcontext.active = 0;
      new->rcontext.seed_set = 0;
      new->rcontext.seed = DefaultSeed( &new->rcontext );

/* If an error occurred, clean up by deleting the new object. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return a pointer to the new object. */
   return new;
}

AstMathMap *astLoadMathMap_( void *mem, size_t size, int init,
                             AstMathMapVtab *vtab, const char *name,
                             AstChannel *channel ) {
/*
*+
*  Name:
*     astLoadMathMap

*  Purpose:
*     Load a MathMap.

*  Type:
*     Protected function.

*  Synopsis:
*     #include "mathmap.h"
*     AstMathMap *astLoadMathMap( void *mem, size_t size, int init,
*                                 AstMathMapVtab *vtab, const char *name,
*                                 AstChannel *channel )

*  Class Membership:
*     MathMap loader.

*  Description:
*     This function is provided to load a new MathMap using data read
*     from a Channel. It first loads the data used by the parent class
*     (which allocates memory if necessary) and then initialises a
*     MathMap structure in this memory, using data read from the input
*     Channel.
*
*     If the "init" flag is set, it also initialises the contents of a
*     virtual function table for a MathMap at the start of the memory
*     passed via the "vtab" parameter.

*  Parameters:
*     mem
*        A pointer to the memory into which the MathMap is to be
*        loaded.  This must be of sufficient size to accommodate the
*        MathMap data (sizeof(MathMap)) plus any data used by derived
*        classes. If a value of NULL is given, this function will
*        allocate the memory itself using the "size" parameter to
*        determine its size.
*     size
*        The amount of memory used by the MathMap (plus derived class
*        data).  This will be used to allocate memory if a value of
*        NULL is given for the "mem" parameter. This value is also
*        stored in the MathMap structure, so a valid value must be
*        supplied even if not required for allocating memory.
*
*        If the "vtab" parameter is NULL, the "size" value is ignored
*        and sizeof(AstMathMap) is used instead.
*     init
*        A boolean flag indicating if the MathMap's virtual function
*        table is to be initialised. If this value is non-zero, the
*        virtual function table will be initialised by this function.
*
*        If the "vtab" parameter is NULL, the "init" value is ignored
*        and the (static) virtual function table initialisation flag
*        for the MathMap class is used instead.
*     vtab
*        Pointer to the start of the virtual function table to be
*        associated with the new MathMap. If this is NULL, a pointer
*        to the (static) virtual function table for the MathMap class
*        is used instead.
*     name
*        Pointer to a constant null-terminated character string which
*        contains the name of the class to which the new object
*        belongs (it is this pointer value that will subsequently be
*        returned by the astGetClass method).
*
*        If the "vtab" parameter is NULL, the "name" value is ignored
*        and a pointer to the string "MathMap" is used instead.

*  Returned Value:
*     A pointer to the new MathMap.

*  Notes:
*     - A null pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-
*/

/* Local Constants: */
#define KEY_LEN 50               /* Maximum length of a keyword */

/* Local Variables: */
   AstMathMap *new;              /* Pointer to the new MathMap */
   char key[ KEY_LEN + 1 ];      /* Buffer for keyword strings */
   int ifun;                     /* Loop counter for functions */
   int invert;                   /* Invert attribute value */

/* Initialise. */
   new = NULL;

/* Check the global error status. */
   if ( !astOK ) return new;

/* If a NULL virtual function table has been supplied, then this is
   the first loader to be invoked for this MathMap. In this case the
   MathMap belongs to this class, so supply appropriate values to be
   passed to the parent class loader (and its parent, etc.). */
   if ( !vtab ) {
      size = sizeof( AstMathMap );
      init = !class_init;
      vtab = &class_vtab;
      name = "MathMap";
   }

/* Invoke the parent class loader to load data for all the ancestral
   classes of the current one, returning a pointer to the resulting
   partly-built MathMap. */
   new = astLoadMapping( mem, size, init, (AstMappingVtab *) vtab, name,
                         channel );

/* If required, initialise the part of the virtual function table used
   by this class. */
   if ( init ) InitVtab( vtab );

/* Note if we have successfully initialised the (static) virtual
   function table owned by this class (so that this is done only
   once). */
   if ( astOK ) {
      if ( ( vtab == &class_vtab ) && init ) class_init = 1;

/* Read input data. */
/* ================ */
/* Request the input Channel to read all the input data appropriate to
   this class into the internal "values list". */
      astReadClassData( channel, "MathMap" );

/* Now read each individual data item from this list and use it to
   initialise the appropriate instance variable(s) for this class. */

/* In the case of attributes, we first read the "raw" input value,
   supplying the "unset" value as the default. If a "set" value is
   obtained, we then use the appropriate (private) Set... member
   function to validate and set the value properly. */

/* Determine if the MathMap is inverted and obtain the "true" number
   of forward and inverse transformation functions by un-doing the
   effects of any inversion. */
      invert = astGetInvert( new );
      new->nfwd = invert ? astGetNin( new ) : astGetNout( new );
      new->ninv = invert ? astGetNout( new ) : astGetNin( new );
      if ( astOK ) {

/* Allocate memory for the MathMap's transformation function arrays. */
         MALLOC_POINTER_ARRAY( new->fwdfun, char *, new->nfwd )
         MALLOC_POINTER_ARRAY( new->invfun, char *, new->ninv )
         if ( astOK ) {

/* Forward transformation functions. */
/* --------------------------------- */
/* Create a keyword for each forward transformation function and read
   the function's value as a string. */
            for ( ifun = 0; ifun < new->nfwd; ifun++ ) {
               (void) sprintf( key, "f%d", ifun + 1 );
               new->fwdfun[ ifun ] = astReadString( channel, key, "" );
            }

/* Inverse transformation functions. */
/* --------------------------------- */
/* Repeat this process for the inverse transformation functions. */
            for ( ifun = 0; ifun < new->ninv; ifun++ ) {
               (void) sprintf( key, "i%d", ifun + 1 );
               new->invfun[ ifun ] = astReadString( channel, key, "" );
            }

/* Forward-inverse simplification flag. */
/* ------------------------------------ */
            new->simp_fi = astReadInt( channel, "simpfi", -INT_MAX );
            if ( TestSimpFI( new ) ) SetSimpFI( new, new->simp_fi );

/* Inverse-forward simplification flag. */
/* ------------------------------------ */
            new->simp_if = astReadInt( channel, "simpif", -INT_MAX );
            if ( TestSimpIF( new ) ) SetSimpIF( new, new->simp_if );

/* Random number context. */
/* ---------------------- */
/* Initialise the random number generator context. */
            new->rcontext.random_int = 0;
            new->rcontext.active = 0;

/* Read the flag that determines if the Seed value is set, and the
   Seed value itself. */
            new->rcontext.seed_set = astReadInt( channel, "seeded", 0 );
            if ( TestSeed( new ) ) {
               new->rcontext.seed = astReadInt( channel, "seed", 0 );
               SetSeed( new, new->rcontext.seed );

/* Supply a default Seed value if necessary. */
            } else {
               new->rcontext.seed = DefaultSeed( &new->rcontext );
            }

/* Compile the MathMap's transformation functions. */
            CompileMapping( "astLoadMathMap",
                            new->ninv, new->nfwd,
                            (const char **) new->fwdfun,
                            (const char **) new->invfun,
                            &new->fwdcode, &new->invcode,
                            &new->fwdcon, &new->invcon,
                            &new->fwdstack, &new->invstack );
         }

/* If an error occurred, clean up by deleting the new MathMap. */
         if ( !astOK ) new = astDelete( new );
      }
   }

/* Return the new MathMap pointer. */
   return new;

/* Undefine macros local to this function. */
#undef KEY_LEN
}
