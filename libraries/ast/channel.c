/*
*class++
*  Name:
*     Channel

*  Purpose:
*     Basic (textual) I/O channel.

*  Constructor Function:
c     astChannel
f     AST_CHANNEL

*  Description:
*     The Channel class implements low-level input/output for the AST
*     library.  Writing an Object to a Channel will generate a textual
*     representation of that Object, and reading from a Channel will
*     create a new Object from its textual representation.
*
*     Normally, when you use a Channel, you should provide "source"
c     and "sink" functions which connect it to an external data store
f     and "sink" routines which connect it to an external data store
*     by reading and writing the resulting text. By default, however,
*     a Channel will read from standard input and write to standard
*     output.

*  Inheritance:
*     The Channel class inherits from the Object class.

*  Attributes:
*     In addition to those attributes common to all Objects, every
*     Channel also has the following attributes:
*
*     - Comment: Include textual comments in output?
*     - Full: Set level of output detail
*     - Skip: Skip irrelevant data?

*  Functions:
c     In addition to those functions applicable to all Objects, the
c     following functions may also be applied to all Channels:
f     In addition to those routines applicable to all Objects, the
f     following routines may also be applied to all Channels:
*
c     - astRead: Read an Object from a Channel
c     - astWrite: Write an Object to a Channel
f     - AST_READ: Read an Object from a Channel
f     - AST_WRITE: Write an Object to a Channel

*  Copyright:
*     <COPYRIGHT_STATEMENT>

*  Authors:
*     RFWS: R.F. Warren-Smith (Starlink)

*  History:
*     12-AUG-1996 (RFWS):
*        Original version.
*     6-SEP-1996:
*        Finished initial implementation.
*     11-DEC-1996 (RFWS):
*        Added support for foreign language source and sink functions.
*     28-APR-1997 (RFWS):
*        Prevent "-0" being written (use "0" instead).
*class--
*/

/* Module Macros. */
/* ============== */
/* Set the name of the class we are implementing. This indicates to
   the header files that define class interfaces that they should make
   "protected" symbols available. */
#define astCLASS Channel

/* Define a string containing the maximum length of keywords used to
   identify values in the external representation of data. This is
   deliberately kept small so as to simplify integration with
   standards such as FITS. */
#define MAX_NAME "8"

/* Define the increment used for indenting output text. */
#define INDENT_INC 3

/* Include files. */
/* ============== */
/* Interface definitions. */
/* ---------------------- */
#include "error.h"               /* Error reporting facilities */
#include "memory.h"              /* Memory allocation facilities */
#include "object.h"              /* Base Object class */
#include "channel.h"             /* Interface definition for this class */
#include "loader.h"              /* Interface to the global loader */

/* Error code definitions. */
/* ----------------------- */
#include "ast_err.h"             /* AST error codes */

/* C header files. */
/* --------------- */
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Type definitions. */
/* ================= */
/* Define a private structure type used to store linked lists of
   name-value associations. */
typedef struct Value {
   struct Value *flink;          /* Link to next element */
   struct Value *blink;          /* Link to previous element */
   char *name;                   /* Pointer to name string */
   union {                       /* Holds pointer to value */
      char *string;              /* Pointer to string value */
      AstObject *object;         /* Pointer to Object value */
   } ptr;
   int is_object;                /* Whether value is an Object (else string) */
} Value;

/* Module Variables. */
/* ================= */
/* Define the class virtual function table and its initialisation flag
   as static variables. */
static AstChannelVtab class_vtab; /* Virtual function table */
static int class_init = 0;       /* Virtual function table initialised? */

/* Pointers to parent class methods which are extended by this class. */
static const char *(* parent_getattrib)( AstObject *, const char * );
static int (* parent_testattrib)( AstObject *, const char * );
static void (* parent_clearattrib)( AstObject *, const char * );
static void (* parent_setattrib)( AstObject *, const char * );

/* Count of the number of output items written since the last "Begin"
   or "IsA" item. */
static items_written = 0;

/* Amount of indentation to be applied to the next output item. */
static int current_indent = 0;

/* Nesting level, used to keep track of data associated with building
   Objects when they contain other Objects. */
static int nest = -1;

/***
   The following items are all pointers to dynamically allocated
   arrays (stacks) that grow as necessary to accommodate one element
   for each level of nesting (one more than the value of "nest").
***/

/* Stack of pointers to null-terminated character strings giving the
   names of the classes of the Objects being built at each nesting
   level. */
static char **object_class = NULL;

/* Stack of pointers to the elements designated as the "heads" of
   circular, doubly linked lists of name-value associations. */
static Value **values_list = NULL;

/* Stack of pointers to null-terminated character strings giving the
   names of the classes for which the values held in the values lists
   are intended. */
static char **values_class = NULL;

/* Stack of flags indicating whether the values held in the values
   lists are intended for the class loaders currently executing to
   build Objects at each nesting level. */
static int *values_ok = NULL;

/* Stack of flags indicating whether "End" items have been read for
   the Objects being built at each nesting level. */
static int *end_of_object = NULL;

/* External Interface Function Prototypes. */
/* ======================================= */
/* The following functions have public prototypes only (i.e. no
   protected prototypes), so we must provide local prototypes for use
   within this module. */
AstChannel *astChannelForId_( const char *(*)( void ), char *(*)( const char *(*)( void ) ), void (*)( const char * ), void (*)( void (*)( const char * ), const char * ), const char *, ... );
AstChannel *astChannelId_( const char *(*)( void ), void (*)( const char * ), const char *, ... );

/* Prototypes for Private Member Functions. */
/* ======================================== */
static AstChannel *Init( void *, size_t, int, AstChannelVtab *, const char *, const char *(*)( void ), char *(*)( const char *(*)( void ) ), void (*)( const char * ), void (*)( void (*)( const char * ), const char * ) );
static AstObject *Read( AstChannel * );
static AstObject *ReadObject( AstChannel *, const char *, AstObject * );
static Value *FreeValue( Value * );
static Value *LookupValue( const char * );
static char *AppendString( char *, int *, const char * );
static char *InputTextItem( AstChannel * );
static char *InputTextItem( AstChannel * );
static char *ReadString( AstChannel *, const char *, const char * );
static char *SourceWrap( const char *(*)( void ) );
static const char *GetAttrib( AstObject *, const char * );
static double ReadDouble( AstChannel *, const char *, double );
static int GetComment( AstChannel * );
static int GetFull( AstChannel * );
static int GetSkip( AstChannel * );
static int ReadInt( AstChannel *, const char *, int );
static int TestAttrib( AstObject *, const char * );
static int TestComment( AstChannel * );
static int TestFull( AstChannel * );
static int TestSkip( AstChannel * );
static int Use( AstChannel *, int, int );
static int Write( AstChannel *, AstObject * );
static void AppendValue( Value *, Value ** );
static void ClearAttrib( AstObject *, const char * );
static void ClearComment( AstChannel * );
static void ClearFull( AstChannel * );
static void ClearSkip( AstChannel * );
static void ClearValues( AstChannel * );
static void Dump( AstObject *, AstChannel * );
static void GetNextData( AstChannel *, int, char **, char ** );
static void InitVtab( AstChannelVtab * );
static void OutputTextItem( AstChannel *, const char * );
static void PutNextText( AstChannel *, const char * );
static void ReadClassData( AstChannel *, const char * );
static void RemoveValue( Value *, Value ** );
static void SetAttrib( AstObject *, const char * );
static void SetComment( AstChannel *, int );
static void SetFull( AstChannel *, int );
static void SetSkip( AstChannel *, int );
static void SinkWrap( void (*)( const char * ), const char * );
static void Unquote( AstChannel *, char * );
static void WriteBegin( AstChannel *, const char *, const char * );
static void WriteDouble( AstChannel *, const char *, int, int, double, const char * );
static void WriteEnd( AstChannel *, const char * );
static void WriteInt( AstChannel *, const char *, int, int, int, const char * );
static void WriteIsA( AstChannel *, const char *, const char * );
static void WriteObject( AstChannel *, const char *, int, int, AstObject *, const char * );
static void WriteString( AstChannel *, const char *, int, int, const char *, const char * );

/* Member functions. */
/* ================= */
static char *AppendString( char *str1, int *nc, const char *str2 ) {
/*
*  Name:
*     AppendString

*  Purpose:
*     Append a string to another string which grows dynamically.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     char *AppendString( char *str1, int *nc, const char *str2 )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function appends one string to another dynamically
*     allocated string, extending the dynamic string as necessary to
*     accommodate the new characters (plus the final null).

*  Parameters:
*     str1
*        Pointer to the null-terminated dynamic string, whose memory
*        has been allocated using the AST memory allocation functions
*        defined in "memory.h". If no space has yet been allocated for
*        this string, a NULL pointer may be given and fresh space will
*        be allocated by this function.
*     nc
*        Pointer to an integer containing the number of characters in
*        the dynamic string (excluding the final null). This is used
*        to save repeated searching of this string to determine its
*        length and it defines the point where the new string will be
*        appended. Its value is updated by this function to include
*        the extra characters appended.
*
*        If "str1" is NULL, the initial value supplied for "*nc" will
*        be ignored and zero will be used.
*     str2
*        Pointer to a constant null-terminated string, a copy of which
*        is to be appended to "str1".

*  Returned Value:
*     A possibly new pointer to the dynamic string with the new string
*     appended (its location in memory may have to change if it has to
*     be extended, in which case the original memory is automatically
*     freed by this function). When the string is no longer required,
*     its memory should be freed using astFree.

*  Notes:
*     - If this function is invoked with the global error status set
*     or if it should fail for any reason, then the returned pointer
*     will be equal to "str1" and the dynamic string contents will be
*     unchanged.
*/

/* Local Variables: */
   char *result;                 /* Pointer value to return */
   int len;                      /* Length of new string */

/* Initialise. */
   result = str1;

/* If the first string pointer is NULL, also initialise the character
   count to zero. */
   if ( !str1 ) *nc = 0;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Calculate the total string length once the two strings have been
   concatenated. */
   len = *nc + (int) strlen( str2 );

/* Extend the first (dynamic) string to the required length, including
   a final null. Save the resulting pointer, which will be
   returned. */
   result = astGrow( str1, len + 1, sizeof( char ) );

/* If OK, append the second string and update the total character
   count. */
   if ( astOK ) {
      (void) strcpy( result + *nc, str2 );
      *nc = len;
   }

/* Return the result pointer. */
   return result;
}

static void AppendValue( Value *value, Value **head ) {
/*
*  Name:
*     AppendValue

*  Purpose:
*     Append a Value structure to a list.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void AppendValue( Value *value, Value **head )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function appends a Value structure to a doubly linked
*     circular list of such structures. The new list element is
*     inserted just in front of the element occupying the "head of
*     list" position (i.e. it becomes the new last element in the
*     list).

*  Parameters:
*     value
*        Pointer to the new element. This must not already be in the
*        list.
*     head
*        Address of a pointer to the element at the head of the list
*        (this pointer should be NULL if the list is initially
*        empty). This pointer will only be updated if a new element is
*        being added to an empty list.

*  Notes:
*     - This function does not perform error chacking and does not
*     generate errors.
*/

/* If the list is initially empty, the sole new element points at
   itself. */
   if ( !*head ) {
      value->flink = value;
      value->blink = value;

/* Update the list head to identify the new element. */
      *head = value;

/* Otherwise, insert the new element in front of the element at the
   head of the list. */
   } else {
      value->flink = *head;
      value->blink = ( *head )->blink;
      ( *head )->blink = value;
      value->blink->flink = value;
   }
}

static void ClearAttrib( AstObject *this_object, const char *attrib ) {
/*
*  Name:
*     ClearAttrib

*  Purpose:
*     Clear an attribute value for a Channel.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void ClearAttrib( AstObject *this, const char *attrib )

*  Class Membership:
*     Channel member function (over-rides the astClearAttrib protected
*     method inherited from the Object class).

*  Description:
*     This function clears the value of a specified attribute for a
*     Channel, so that the default value will subsequently be used.

*  Parameters:
*     this
*        Pointer to the Channel.
*     attrib
*        Pointer to a null terminated string specifying the attribute
*        name.  This should be in lower case with no surrounding white
*        space.
*/

/* Local Variables: */
   AstChannel *this;              /* Pointer to the Channel structure */

/* Check the global error status. */
   if ( !astOK ) return;

/* Obtain a pointer to the Channel structure. */
   this = (AstChannel *) this_object;

/* Check the attribute name and clear the appropriate attribute. */

/* Comment. */
/* -------- */
   if ( !strcmp( attrib, "comment" ) ) {
      astClearComment( this );

/* Full. */
/* ----- */
   } else if ( !strcmp( attrib, "full" ) ) {
      astClearFull( this );

/* Skip. */
/* ----- */
   } else if ( !strcmp( attrib, "skip" ) ) {
      astClearSkip( this );

/* If the attribute is still not recognised, pass it on to the parent
   method for further interpretation. */
   } else {
      (*parent_clearattrib)( this_object, attrib );
   }
}

static void ClearValues( AstChannel *this ) {
/*
*  Name:
*     ClearValues

*  Purpose:
*     Clear the current values list.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void ClearValues( AstChannel *this )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function clears any (un-read) Value structures remaining in
*     the current values list (i.e. at the current nesting level). It
*     should be invoked after all required values have been read.
*
*     If the values list has not been read, or if any remaining values
*     are found (i.e. the list is not empty) then this indicates an
*     unrecognised input class or an input value that has not been
*     read by a class loader. This implies an error in the loader, or
*     bad input data, so an error is reported.
*
*     All resources used by any remaining Value structures are freed
*     and the values list is left in an empty state.

*  Parameters:
*     this
*        Pointer to the Channel being read. This is only used for
*        constructing error messages. It must not be NULL.

*  Notes:
*     - This function attempts to execute even if the global error
*     status is set on entry, although no further error report will be
*     made if it subsequently fails under these circumstances.
*/

/* Local Variables: */
   Value **head;                 /* Address of pointer to values list */
   Value *value;                 /* Pointer to value list element */

/* If "values_class" is non-NULL, then the values list has previously
   been filled with Values for a class. */
   if ( values_class[ nest ] ) {

/* If "values_ok" is zero, however, then these Values have not yet
   been read by a class loader. This must be due to a bad class name
   associated with them or because the class data are not available in
   the correct order. Report an error (unless the error status is
   already set). */
      if ( !values_ok[ nest ] && astOK ) {
         astError( AST__BADIN,
                   "astRead(%s): Invalid class structure in input data.",
                   astGetClass( this ) );
         astError( AST__BADIN,
                   "Class \"%s\" is invalid or out of order within a %s.",
                   values_class[ nest ], object_class[ nest ] );
      }

/* Free the memory holding the class string. */
      values_class[ nest ] = astFree( values_class[ nest ] );
   }

/* Reset the "values_ok" flag. */
   values_ok[ nest ] = 0;

/* Now clear any Values remaining in the values list. Obtain the
   address of the pointer to the head of this list (at the current
   nesting level) and loop to remove Values from the list while it is
   not empty. */
   head = values_list + nest;
   while ( *head ) {

/* Obtain a pointer to the first element. */
      value = *head;

/* If no error has yet occurred, then report an appropriate error
   message, depending on whether the Value is associated with an
   Object or a string. */
      if ( astOK ) {
         if ( value->is_object ) {
            astError( AST__BADIN,
                      "astRead(%s): The Object \"%s = <%s>\" was "
                      "not recognised as valid input.",
                      astGetClass( this ), value->name,
                      astGetClass( value->ptr.object ) );
         } else {
            astError( AST__BADIN,
                      "astRead(%s): The value \"%s = %s\" was not "
                      "recognised as valid input.",
                      astGetClass( this ), value->name, value->ptr.string );
         }
      }

/* Remove the Value structure from the list (which updates the head of
   list pointer) and free its resources. */
      RemoveValue( value, head );
      value = FreeValue( value );
   }
}

static Value *FreeValue( Value *value ) {
/*
*  Name:
*     FreeValue

*  Purpose:
*     Free a dynamically allocated Value structure.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     Value *FreeValue( Value *value )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function frees a dynamically allocated Value structure,
*     releasing all resources used by it. The structure contents must
*     have been correctly initialised.

*  Parameters:
*     value
*        Pointer to the Value structure to be freed.

*  Returned Value:
*     A NULL pointer is always returned.

*  Notes:
*     - This function attempts to execute even if the global error
*     status is set on entry, although no further error report will be
*     made if it subsequently fails under these circumstances.
*/

/* Check that a non-NULL pointer has been supplied. */
   if ( value ) {

/* If the "name" component has been allocated, then free it. */
      if ( value->name ) value->name = astFree( value->name );

/* If the "ptr" component identifies an Object, then annul the Object
   pointer. */
      if ( value->is_object ) {
         if ( value->ptr.object ) {
            value->ptr.object = astAnnul( value->ptr.object );
         }

/* Otherwise, if it identifies a string, then free the string. */
      } else {
         if ( value->ptr.string ) {
            value->ptr.string = astFree( value->ptr.string );
         }
      }

/* Free the Value structure itself. */
      value = astFree( value );
   }

/* Return a NULL pointer. */
   return NULL;
}

static const char *GetAttrib( AstObject *this_object, const char *attrib ) {
/*
*  Name:
*     GetAttrib

*  Purpose:
*     Get the value of a specified attribute for a Channel.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     const char *GetAttrib( AstObject *this, const char *attrib )

*  Class Membership:
*     Channel member function (over-rides the protected astGetAttrib
*     method inherited from the Object class).

*  Description:
*     This function returns a pointer to the value of a specified
*     attribute for a Channel, formatted as a character string.

*  Parameters:
*     this
*        Pointer to the Channel.
*     attrib
*        Pointer to a null terminated string containing the name of
*        the attribute whose value is required. This name should be in
*        lower case, with all white space removed.

*  Returned Value:
*     - Pointer to a null terminated string containing the attribute
*     value.

*  Notes:
*     - The returned string pointer may point at memory allocated
*     within the Channel, or at static memory. The contents of the
*     string may be over-written or the pointer may become invalid
*     following a further invocation of the same function or any
*     modification of the Channel. A copy of the string should
*     therefore be made if necessary.
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*/

/* Local Constants: */
#define BUFF_LEN 50              /* Max. characters in result buffer */

/* Local Variables: */
   AstChannel *this;             /* Pointer to the Channel structure */
   const char *result;           /* Pointer value to return */
   int comment;                  /* Comment attribute value */
   int full;                     /* Full attribute value */
   int skip;                     /* Skip attribute value */
   static char buff[ BUFF_LEN + 1 ]; /* Buffer for string result */

/* Initialise. */
   result = NULL;

/* Check the global error status. */   
   if ( !astOK ) return result;

/* Obtain a pointer to the Channel structure. */
   this = (AstChannel *) this_object;

/* Compare "attrib" with each recognised attribute name in turn,
   obtaining the value of the required attribute. If necessary, write
   the value into "buff" as a null terminated string in an appropriate
   format.  Set "result" to point at the result string. */

/* Comment. */
/* -------- */
   if ( !strcmp( attrib, "comment" ) ) {
      comment = astGetComment( this );
      if ( astOK ) {
         (void) sprintf( buff, "%d", comment );
         result = buff;
      }

/* Full. */
/* ----- */
   } else if ( !strcmp( attrib, "full" ) ) {
      full = astGetFull( this );
      if ( astOK ) {
         (void) sprintf( buff, "%d", full );
         result = buff;
      }

/* Skip. */
/* ----- */
   } else if ( !strcmp( attrib, "skip" ) ) {
      skip = astGetSkip( this );
      if ( astOK ) {
         (void) sprintf( buff, "%d", skip );
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

static void GetNextData( AstChannel *this, int skip, char **name,
                         char **val ) {
/*
*+
*  Name:
*     astGetNextData

*  Purpose:
*     Read the next item of data from a data source.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astGetNextData( AstChannel *this, int skip, char **name,
*                          char **val )

*  Class Membership:
*     Channel method.

*  Description:
*     This function reads the next item of input data from a data
*     source associated with a Channel and returns the result.
*
*     It is layered conceptually on the astGetNextText method, but
*     instead of returning the raw input text, it decodes it and
*     returns name/value pairs ready for use. Note that in some
*     derived classes, where the data are not stored as text, this
*     function may not actually use astGetNextText, but will access
*     the data directly.

*  Parameters:
*     this
*        Pointer to the Channel.
*     skip
*        A non-zero value indicates that a new Object is to be read,
*        and that all input data up to the next "Begin" item are to be
*        skipped in order to locate it. This is useful if the data
*        source contains AST objects interspersed with other data (but
*        note that these other data cannot appear inside AST Objects,
*        only between them).
*
*        A zero value indicates that all input data are significant
*        and the next item will therefore be read and an attempt made
*        to interpret it whatever it contains. Any other data
*        inter-mixed with AST Objects will then result in an error.
*     name
*        An address at which to store a pointer to a null-terminated
*        dynamically allocated string containing the name of the next
*        item in the input data stream. This name will be in lower
*        case with no surrounding white space.  It is the callers
*        responsibilty to free the memory holding this string (using
*        astFree) when it is no longer required.
*
*        A NULL pointer value will be returned (without error) to
*        indicate when there are no further input data items to be
*        read.
*     val
*        An address at which to store a pointer to a null-terminated
*        dynamically allocated string containing the value associated
*        with the next item in the input data stream. No case
*        conversion is performed on this string and all white space is
*        potentially significant.  It is the callers responsibilty to
*        free the memory holding this string (using astFree) when it
*        is no longer required.
*
*        The returned pointer will be NULL if an Object data item is
*        read (see the "Data Representation" section).

*  Data Representation:
*     The returned data items fall into the following categories:
*
*     - Begin: Identified by the name string "begin", this indicates
*     the start of an Object definition. The associated value string
*     gives the class name of the Object being defined.
*
*     - IsA: Identified by the name string "isa", this indicates the
*     end of the data associated with a particular class structure
*     within the definiton of a larger Object. The associated value
*     string gives the name of the class whose data have just been
*     read.
*
*     - End: Identified by the name string "end", this indicates the
*     end of the data associated with a complete Object
*     definition. The associated value string gives the class name of
*     the Object whose definition is being ended.
*
*     - Non-Object: Identified by any other name string plus a
*     non-NULL "val" pointer, this gives the value of a non-Object
*     structure component (instance variable). The name identifies
*     which instance variable it is (within the context of the class
*     whose data are being read) and the value is encoded as a string.
*
*     - Object: Identified by any other name string plus a NULL "val"
*     pointer, this identifies the value of an Object structure
*     component (instance variable).  The name identifies which
*     instance variable it is (within the context of the class whose
*     data are being read) and the value is given by subsequent data
*     items (so the next item should be a "Begin" item).

*  Notes:
*     - NULL pointer values will be returned if this function is
*     invoked with the global error status set, or if it should fail
*     for any reason.
*     - This method is provided primarily so that derived classes may
*     over-ride it in order to read from alternative data sources. It
*     provides a higher-level interface than astGetNextText, so is
*     suitable for classes that either need to read textual data in a
*     different format, or to read from non-textual data sources.
*-
*/

/* Local Variables: */
   char *line;                   /* Pointer to input text line */
   int done;                     /* Data item read? */
   int i;                        /* Loop counter for string characters */
   int len;                      /* Length of input text line */
   int nc1;                      /* Offset to start of first field */
   int nc2;                      /* Offset to end of first field */
   int nc3;                      /* Offset to start of second field */
   int nc;                       /* Number of charaters read by "sscanf" */

/* Initialise the returned values. */
   *name = NULL;
   *val = NULL;

/* Check the global error status. */
   if ( !astOK ) return;

/* Read the next input line as text (the loop is needed to allow
   initial lines to be skipped if the "skip" flag is set). */
   done = 0;
   while ( !done && ( line = InputTextItem( this ) ) && astOK ) {

/* If OK, determine the line length. */
      len = strlen( line );

/* Non-Object value. */
/* ----------------- */
/* Test for lines of the form " name = value" (or similar), where the
   name is no more than MAX_NAME characters long (the presence of a
   value on the right hand side indicates that this is a non-Object
   value, encoded as a string). Ignore these lines if the "skip" flag
   is set. */
      if ( nc = 0,
           ( !skip
             && ( 0 == sscanf( line,
                               " %n%*" MAX_NAME "[^ \t=]%n = %n%*[^\n]%n",
                               &nc1, &nc2, &nc3, &nc ) )
             && ( nc >= len ) ) ) {

/* Note we have found a data item. */
         done = 1;

/* Extract the name and value fields. */
         *name = astString( line + nc1, nc2 - nc1 );
         *val = astString( line + nc3, len - nc3 );

/* If OK, truncate the value to remove any trailing white space. */
         if ( astOK ) {
            i = len - nc3 - 1;
            while ( ( i >= 0 ) && isspace( ( *val )[ i ] ) ) i--;
            ( *val )[ i + 1 ] = '\0';

/* Also remove any quotes from the string. */
            Unquote( this, *val );
         }

/* Object value. */
/* ------------- */
/* Test for lines of the form " name = " (or similar), where the name
   is no more than MAX_NAME characters long (the absence of a value on
   the right hand side indicates that this is an Object, whose
   definition follows on subsequent input lines). Ignore these lines
   if the "skip" flag is set. */
      } else if ( nc = 0,
                  ( !skip
                    && ( 0 == sscanf( line,
                                      " %n%*" MAX_NAME "[^ \t=]%n = %n",
                                      &nc1, &nc2, &nc ) )
                    && ( nc >= len ) ) ) {

/* Note we have found a data item. */
         done = 1;

/* Extract the name field but leave the value pointer as NULL. */
         *name = astString( line + nc1, nc2 - nc1 );

/* Begin. */
/* ------ */
/* Test for lines of the form " Begin Class " (or similar). */
      } else if ( nc = 0,
             ( ( 0 == sscanf( line,
                             " %*1[Bb]%*1[Ee]%*1[Gg]%*1[Ii]%*1[Nn] %n%*s%n %n",
                              &nc1, &nc2, &nc ) )
               && ( nc >= len ) ) ) {

/* Note we have found a data item. */
         done = 1;

/* Set the returned name to "begin" and extract the associated class
   name for the value. Store both of these in dynamically allocated
   strings. */
         *name = astString( "begin", 5 );
         *val = astString( line + nc1, nc2 - nc1 );

/* IsA. */
/* ---- */
/* Test for lines of the form " IsA Class " (or similar). Ignore these
   lies if the "skip" flag is set. */
      } else if ( nc = 0,
                  ( !skip
                    && ( 0 == sscanf( line,
                                      " %*1[Ii]%*1[Ss]%*1[Aa] %n%*s%n %n",
                                      &nc1, &nc2, &nc ) )
                    && ( nc >= len ) ) ) {

/* Note we have found a data item. */
         done = 1;

/* Set the returned name to "isa" and extract the associated class
   name for the value. */
         *name = astString( "isa", 3 );
         *val = astString( line + nc1, nc2 - nc1 );

/* End. */
/* ---- */
/* Test for lines of the form " End Class " (or similar). Ignore these
   lines if the "skip" flag is set. */
      } else if ( nc = 0,
                  ( !skip
                    && ( 0 == sscanf( line,
                                      " %*1[Ee]%*1[Nn]%*1[Dd] %n%*s%n %n",
                                      &nc1, &nc2, &nc ) )
                    && ( nc >= len ) ) ) {

/* Note we have found a data item. */
         done = 1;

/* If found, set the returned name to "end" and extract the associated
   class name for the value. */
         *name = astString( "end", 3 );
         *val = astString( line + nc1, nc2 - nc1 );

/* If the input line didn't match any of the above and the "skip" flag
   is not set, then report an error. */
      } else if ( !skip ) {
         astError( AST__BADIN,
                   "astRead(%s): Cannot interpret the input data: \"%s\".",
                   astGetClass( this ), line );
      }

/* Free the memory holding the input data as text. */
      line = astFree( line );
   }

/* If successful, convert the name to lower case. */
   if ( astOK && *name ) {
      for ( i = 0; ( *name )[ i ]; i++ ) {
         ( *name )[ i ] = tolower( ( *name )[ i ] );
      }
   }

/* If an error occurred, ensure that any memory allocated is freed and
   that NULL pointer values are returned. */
   if ( !astOK ) {
      *name = astFree( *name );
      *val = astFree( *val );
   }
}

static char *GetNextText( AstChannel *this ) {
/*
*+
*  Name:
*     GetNextText

*  Purpose:
*     Read the next line of input text from a data source.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     char *astGetNextText( AstChannel *this )

*  Class Membership:
*     Channel method.

*  Description:
*     This function reads the next "raw" input line of text from the
*     data source associated with a Channel.
*
*     Each line is returned as a pointer to a null-terminated string
*     held in dynamic memory, and it is the caller's responsibility to
*     free this memory (using astFree) when it is no longer
*     required. A NULL pointer is returned if there are no more input
*     lines to be read.

*  Parameters:
*     this
*        Pointer to the Channel.

*  Returned Value:
*     Pointer to a null-terminated string containing the input line
*     (held in dynamically allocated memory, which must be freed by
*     the caller when no longer required). A NULL pointer is returned
*     if there are no more input lines to be read.

*  Notes:
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*     - This method is provided primarily so that derived classes may
*     over-ride it in order to read from alternative (textual) data
*     sources.
*-
*/

/* Local Constants: */
#define MIN_CHARS 81             /* Initial size for allocating memory */

/* Local Variables: */
   char *line;                   /* Pointer to line data to be returned */
   int c;                        /* Input character */
   int i;                        /* Loop counter for line characters */
   int len;                      /* Length of input line */
   int readstat;                 /* "errno" value set by "getchar" */
   int size;                     /* Size of allocated memory */

/* Initialise. */
   line = NULL;

/* Check the global error status. */
   if ( !astOK ) return line;

/* Source function defined. */
/* ------------------------ */
/* If a source function (and its wrapper function) is defined for the
   Channel, use the wrapper function to invoke the source function to
   read a line of input text. This is returned in a dynamically
   allocated string. */
   if ( this->source && this->source_wrap ) {
      line = ( *this->source_wrap )( this->source );

/* No source function. */
/* ------------------- */
/* Read the line from standard input. */
   } else {
      c = '\0';
      len = 0;
      size = 0;

/* Loop to read input characters, saving any "errno" value that may be
   set by "getchar" if an error occurs. Quit if an end of file (or
   error) occurs or if a newline character is read. */
      while ( errno = 0, c = getchar(), readstat = errno,
              ( c != EOF ) && ( c != '\n' ) ) {

/* If no memory has yet been allocated to hold the line, allocate some
   now, using MIN_CHARS as the initial line length. */
         if ( !line ) {
            line = astMalloc( sizeof( char ) * (size_t) MIN_CHARS );
            size = MIN_CHARS;

/* If memory has previously been allocated, extend it when necessary
   to hold the new input character (plus a terminating null) and note
   the new size. */
         } else if ( ( len + 2 ) > size ) {
            line = astGrow( line, len + 2, sizeof( char ) );
            if ( !astOK ) break;
            size = (int) astSizeOf( line );
         }

/* Store the character just read. */
         line[ len++ ] = c;
      }

/* If the above loop completed without setting the global error
   status, check the last character read and use "ferror" to see if a
   read error occurred. If so, report the error, using the saved
   "errno" value (but only if one was set). */
      if ( astOK && ( c == EOF ) && ferror( stdin ) ) {
         if ( readstat ) {
            astError( AST__RDERR,
                      "astRead(%s): Read error on standard input - %s.",
                      astGetClass( this ), strerror( readstat ) );
         } else {
            astError( AST__RDERR,
                      "astRead(%s): Read error on standard input.",
                      astGetClass( this ) );
         }
      }

/* If an empty line has been read, allocate memory to hold an empty
   string. */
      if ( !line && ( c == '\n' ) ) {
         line = astMalloc( sizeof( char ) );
      }

/* If memory has been allocated and there has been no error,
   null-terminate the string of input characters. */
      if ( line ) {
         if ( astOK ) {
            line[ len ] = '\0';

/* If there has been an error, free the allocated memory. */
         } else {
            line = astFree( line );
         }
      }
   }

/* Return the result pointer. */
   return line;

/* Undefine macros local to this function. */
#undef MIN_CHARS
}

static AstChannel *Init( void *mem, size_t size, int init,
                         AstChannelVtab *vtab, const char *name,
                         const char *(* source)( void ),
                         char *(* source_wrap)( const char *(*)( void ) ),
                         void (* sink)( const char * ),
                         void (* sink_wrap)( void (*)( const char * ),
                                             const char * ) ) {
/*
*  Name:
*     Init

*  Purpose:
*     Initialise a Channel.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     AstChannel *Init( void *mem, size_t size, int init,
*                       AstChannelVtab *vtab, const char *name,
*                       const char *(* source)( void ),
*                       char *(* source_wrap)( const char *(*)( void ) ),
*                       void (* sink)( const char * ),
*                       void (* sink_wrap)( void (*)( const char * ),
*                                           const char * ) )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function initialises a new Channel object. It allocates
*     memory (if necessary) to accommodate the Channel plus any
*     additional data associated with the derived class.  It then
*     initialises a Channel structure at the start of this memory. If
*     the "init" flag is set, it also initialises the contents of a
*     virtual function table for a Channel at the start of the memory
*     passed via the "vtab" parameter.

*  Parameters:
*     mem
*        A pointer to the memory in which the Channel is to be
*        initialised.  This must be of sufficient size to accommodate
*        the Channel data (sizeof(Channel)) plus any data used by the
*        derived class. If a value of NULL is given, this function
*        will allocate the memory itself using the "size" parameter to
*        determine its size.
*     size
*        The amount of memory used by the Channel (plus derived class
*        data).  This will be used to allocate memory if a value of
*        NULL is given for the "mem" parameter. This value is also
*        stored in the Channel structure, so a valid value must be
*        supplied even if not required for allocating memory.
*     init
*        A boolean flag indicating if the Channel's virtual function
*        table is to be initialised. If this value is non-zero, the
*        virtual function table will be initialised by this function.
*     vtab
*        Pointer to the start of the virtual function table to be
*        associated with the new Channel.
*     name
*        Pointer to a constant null-terminated character string which
*        contains the name of the class to which the new object
*        belongs (it is this pointer value that will subsequently be
*        returned by the astGetClass method).
*     source
*        Pointer to a "source" function which will be used to obtain
*        lines of input text. Generally, this will be obtained by
*        casting a pointer to a source function which is compatible
*        with the "source_wrap" wrapper function (below). The pointer
*        should later be cast back to its original type by the
*        "source_wrap" function before the function is invoked.
*
*        If "source" is NULL, the Channel will read from standard
*        input instead.
*     source_wrap
*        Pointer to a function which can be used to invoke the
*        "source" function supplied (above). This wrapper function is
*        necessary in order to hide variations in the nature of the
*        source function, such as may arise when it is supplied by a
*        foreign (non-C) language interface.
*
*        The single parameter of the "source_wrap" function is a
*        pointer to the "source" function, and it should cast this
*        function pointer (as necessary) and invoke the function with
*        appropriate arguments to obtain the next line of input
*        text. The "source_wrap" function should then return a pointer
*        to a dynamically allocated, null terminated string containing
*        the text that was read. The string will be freed (using
*        astFree) when no longer required and the "source_wrap"
*        function need not concern itself with this. A NULL pointer
*        should be returned if there is no more input to read.
*
*        If "source_wrap" is NULL, the Channel will read from standard
*        input instead.
*     sink
*        Pointer to a "sink" function which will be used to deliver
*        lines of output text. Generally, this will be obtained by
*        casting a pointer to a sink function which is compatible with
*        the "sink_wrap" wrapper function (below). The pointer should
*        later be cast back to its original type by the "sink_wrap"
*        function before the function is invoked.
*
*        If "sink" is NULL, the Channel will write to standard output
*        instead.
*     sink_wrap
*        Pointer to a function which can be used to invoke the "sink"
*        function supplied (above). This wrapper function is necessary
*        in order to hide variations in the nature of the sink
*        function, such as may arise when it is supplied by a foreign
*        (non-C) language interface.
*
*        The first parameter of the "sink_wrap" function is a pointer
*        to the "sink" function, and the second parameter is a pointer
*        to a const, null-terminated character string containing the
*        text to be written.  The "sink_wrap" function should cast the
*        "sink" function pointer (as necessary) and invoke the
*        function with appropriate arguments to deliver the line of
*        output text. The "sink_wrap" function then returns void.
*
*        If "sink_wrap" is NULL, the Channel will write to standard
*        output instead.

*  Returned Value:
*     A pointer to the new Channel.

*  Notes:
*     - A null pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*/

/* Local Variables: */
   AstChannel *new;              /* Pointer to new Channel */

/* Check the global status. */
   if ( !astOK ) return NULL;

/* Initialise an Object structure (the parent class) as the first
   component within the Channel structure, allocating memory if
   necessary. */
   new = (AstChannel *) astInitObject( mem, size, init,
                                       (AstObjectVtab *) vtab, name );

/* If necessary, initialise the virtual function table. */
   if ( init ) InitVtab( vtab );
   if ( astOK ) {

/* Initialise the Channel data. */
/* ---------------------------- */
/* Save the pointers to the source and sink functions and the wrapper
   functions that invoke them. */
      new->source = source;
      new->source_wrap = source_wrap;
      new->sink = sink;
      new->sink_wrap = sink_wrap;

/* Set all attributes to their undefined values. */
      new->comment = -INT_MAX;
      new->full = -INT_MAX;
      new->skip = -INT_MAX;

/* If an error occurred, clean up by deleting the new object. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return a pointer to the new object. */
   return new;
}

static void InitVtab( AstChannelVtab *vtab ) {
/*
*  Name:
*     InitVtab

*  Purpose:
*     Initialise a virtual function table for a Channel.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void InitVtab( AstChannelVtab *vtab )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function initialises the component of a virtual function
*     table which is used by the Channel class.

*  Parameters:
*     vtab
*        Pointer to the virtual function table. The components used by
*        all ancestral classes should already have been initialised.
*/

/* Local Variables: */
   AstObjectVtab *object;        /* Pointer to Object component of Vtab */

/* Check the local error status. */
   if ( !astOK ) return;

/* Store a unique "magic" value in the virtual function table. This
   will be used (by astIsAChannel) to determine if an object belongs
   to this class.  We can conveniently use the address of the (static)
   class_init variable to generate this unique value. */
   vtab->check = &class_init;

/* Initialise member function pointers. */
/* ------------------------------------ */
/* Store pointers to the member functions (implemented here) that
   provide virtual methods for this class. */
   vtab->ClearComment = ClearComment;
   vtab->ClearFull = ClearFull;
   vtab->ClearSkip = ClearSkip;
   vtab->GetComment = GetComment;
   vtab->GetFull = GetFull;
   vtab->GetNextData = GetNextData;
   vtab->GetNextText = GetNextText;
   vtab->GetSkip = GetSkip;
   vtab->PutNextText = PutNextText;
   vtab->Read = Read;
   vtab->ReadClassData = ReadClassData;
   vtab->ReadDouble = ReadDouble;
   vtab->ReadInt = ReadInt;
   vtab->ReadObject = ReadObject;
   vtab->ReadString = ReadString;
   vtab->SetComment = SetComment;
   vtab->SetFull = SetFull;
   vtab->SetSkip = SetSkip;
   vtab->TestComment = TestComment;
   vtab->TestFull = TestFull;
   vtab->TestSkip = TestSkip;
   vtab->Write = Write;
   vtab->WriteBegin = WriteBegin;
   vtab->WriteDouble = WriteDouble;
   vtab->WriteEnd = WriteEnd;
   vtab->WriteInt = WriteInt;
   vtab->WriteIsA = WriteIsA;
   vtab->WriteObject = WriteObject;
   vtab->WriteString = WriteString;

/* Save the inherited pointers to methods that will be extended, and
   replace them with pointers to the new member functions. */
   object = (AstObjectVtab *) vtab;

   parent_clearattrib = object->ClearAttrib;
   object->ClearAttrib = ClearAttrib;
   parent_getattrib = object->GetAttrib;
   object->GetAttrib = GetAttrib;
   parent_setattrib = object->SetAttrib;
   object->SetAttrib = SetAttrib;
   parent_testattrib = object->TestAttrib;
   object->TestAttrib = TestAttrib;

/* Declare the Dump function for this class. There is no destructor or
   copy constructor. */
   astSetDump( vtab, Dump, "Channel", "Basic I/O Channel" );
}

static char *InputTextItem( AstChannel *this ) {
/*
*  Name:
*     InputTextItem

*  Purpose:
*     Read the next item from a data source as text.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     char *InputTextItem( AstChannel *this )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function reads the next input data item as text from the
*     data source associated with a Channel. It is similar to the
*     astGetNextText method (which it invokes), except that it strips
*     off any comments along with leading and trailing white
*     space. Input lines which are empty or do not contain significant
*     characters (e.g. all comment) are skipped, so that only
*     significant lines are returned.
*
*     Each line is returned as a pointer to a null-terminated string
*     held in dynamic memory, and it is the caller's responsibility to
*     free this memory (using astFree) when it is no longer
*     required. A NULL pointer is returned if there are no more input
*     lines to be read.

*  Parameters:
*     this
*        Pointer to the Channel.

*  Returned Value:
*     Pointer to a null-terminated string containing the input line
*     (held in dynamically allocated memory, which must be freed by
*     the caller when no longer required). A NULL pointer is returned
*     if there are no more input lines to be read.

*  Notes:
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*/

/* Local Variables: */
   char *line;                   /* Pointer to line data to be returned */
   int i;                        /* Loop counter for line characters */
   int j;                        /* Counter for characters */
   int len;                      /* Length of result line */
   int nonspace;                 /* Non-space character encountered? */
   int quoted;                   /* Character is inside quotes? */

/* Initialise. */
   line = NULL;

/* Check the global error status. */
   if ( !astOK ) return line;

/* Loop to read input lines until one is found which contains useful
   characters or end of file is reached (or a read error occurs). */
   while ( !line && ( line = astGetNextText( this ) ) && astOK ) {

/* Loop to remove comments and leading and trailing white space. */
      len = 0;
      nonspace = 0;
      quoted = 0;
      for ( i = j = 0; line[ i ]; i++ ) {

/* Note quote characters and ignore all text after the first unquoted
   comment character. */
         if ( line[ i ] == '"' ) quoted = !quoted;
         if ( ( line[ i ] == '#' ) && !quoted ) break;

/* Note the first non-space character and ignore everything before
   it. */
         if ( nonspace = nonspace || !isspace( line[ i ] ) ) {

/* Move each character to its new position in the string. */
            line[ j++ ] = line[ i ];

/* Note the final length of the string (ignoring trailing spaces). */
            if ( !isspace( line[ i ] ) ) len = j;
         }
      }

/* If the string is not empty, terminate it. */
     if ( len ) {
        line[ len ] = '\0';

/* Otherwise, free the memory used for the string so that another
   input line will be read. */
      } else {
         line = astFree( line );
      }
   }

/* Return the result pointer. */
   return line;

/* Undefine macros local to this function. */
#undef MIN_CHARS
}

static Value *LookupValue( const char *name ) {
/*
*  Name:
*     LookupValue

*  Purpose:
*     Look up a Value structure by name.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     Value *LookupValue( const char *name )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function searches the current values list (i.e. at the
*     current nesting level) to identify a Value structure with a
*     specified name. If one is found, it is removed from the list and
*     a pointer to it is returned. If no suitable Value can be found,
*     a NULL pointer is returned instead.

*  Parameters:
*     name
*        Pointer to a constant null-terminated character string
*        containing the name of the required Value. This must be in
*        lower case with no surrounding white space. Note that names
*        longer than NAME_MAX characters will not match any Value.

*  Returned value:
*     Pointer to the required Value structure, or NULL if no suitable
*     Value exists.

*  Notes:
*     - The returned pointer refers to a dynamically allocated
*     structure and it is the callers responsibility to free this when
*     no longer required. The FreeValue function must be used for this
*     purpose.
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*/

/* Local Variables: */
   Value **head;                 /* Address of head of list pointer */
   Value *result;                /* Pointer value to return */
   Value *value;                 /* Pointer to list element */

/* Initialise. */
   result = NULL;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Check that the "values_ok" flag is set. If not, the Values in the
   values list belong to a different class to that of the current
   class loader, so we cannot return any Value. */
   if ( values_ok[ nest ] ) {

/* Obtain the address of the current "head of list" pointer for the
   values list (at the current nesting level). */
      head = values_list + nest;

/* Obtain the head of list pointer itself and check the list is not
   empty. */
      if ( value = *head ) {

/* Loop to inspect each list element. */
         while ( 1 ) {

/* If a name match is found, remove the element from the list, return
   a pointer to it and quit searching. */
            if ( !strcmp( name, value->name ) ) {
               RemoveValue( value, head );
               result = value;
               break;
            }

/* Follow the list until we return to the head. */
            value = value->flink;
            if ( value == *head ) break;
         }
      }
   }

/* Return the result. */
   return result;
}

static void OutputTextItem( AstChannel *this, const char *line ) {
/*
*  Name:
*     OutputTextItem

*  Purpose:
*     Output a data item formatted as text.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void OutputTextItem( AstChannel *this, const char *line )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function outputs a data item formatted as a text string to
*     a data sink associated with a Channel. It keeps track of the
*     number of items written.

*  Parameters:
*     this
*        Pointer to the Channel.
*     line
*        Pointer to a constant null-terminated string containing the
*        data item to be output (no newline character should be
*        appended).
*/

/* Check the global error status. */
   if ( !astOK ) return;

/* Write out the line of text using the astPutNextText method (which
   may be over-ridden). */
   astPutNextText( this, line );

/* If successful, increment the count of items written. */
   if ( astOK ) items_written++;
}

static void PutNextText( AstChannel *this, const char *line ) {
/*
*+
*  Name:
*     astPutNextText

*  Purpose:
*     Write a line of output text to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astPutNextText( AstChannel *this, const char *line )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes an output line of text to a data sink
*     associated with a Channel.

*  Parameters:
*     this
*        Pointer to the Channel.
*     line
*        Pointer to a constant null-terminated string containing the
*        line of output text to be written (no newline character
*        should be appended).

*  Notes:
*     - This method is provided primarily so that derived classes may
*     over-ride it in order to write to alternative (textual) data
*     sinks.
*-
*/

/* Check the global error status. */
   if ( !astOK ) return;

/* If a sink function (and its wrapper function) is defined for the
   Channel, use the wrapper function to invoke the sink finction to
   output the text line. */
   if ( this->sink && this->sink_wrap ) {
      ( *this->sink_wrap )( *this->sink, line );

/* Otherwise, simply write the text to standard output with a newline
   appended. */
   } else {
      (void) printf( "%s\n", line );
   }
}

static AstObject *Read( AstChannel *this ) {
/*
*++
*  Name:
c     astRead
f     AST_READ

*  Purpose:
*     Read an Object from a Channel.

*  Type:
*     Public function.

*  Synopsis:
c     #include "channel.h"
c     AstObject *astRead( AstChannel *this )
f     RESULT = AST_READ( THIS, STATUS )

*  Class Membership:
*     Channel method.

*  Description:
*     This function reads the next Object from a Channel and returns a
*     pointer to the new Object.

*  Parameters:
c     this
f     THIS = INTEGER (Given)
*        Pointer to the Channel.
f     STATUS = INTEGER (Given and Returned)
f        The global status.

*  Returned Value:
c     astRead()
f     AST_READ = INTEGER
*        A pointer to the new Object. The class to which this will
*        belong is determined by the input data, so is not known in
*        advance.

*  Notes:
*     - A null Object pointer (AST__NULL) will be returned, without
*     error, if the Channel contains no further Objects to be read.
*     - A null Object pointer will also be returned if this function
c     is invoked with the AST error status set, or if it should fail
f     is invoked with STATUS set to an error value, or if it should fail
*     for any reason.
*--
*/

/* Local Variables: */
   AstLoaderType *loader;        /* Pointer to loader for Object */
   AstObject *new;               /* Pointer to new Object */
   char *class;                  /* Pointer to Object class name string */
   char *name;                   /* Pointer to data item name */
   int skip;                     /* Skip non-AST data? */
   int top;                      /* Reading top-level Object definition? */

/* Initialise. */
   new = NULL;

/* Check the global error status. */
   if ( !astOK ) return new;

/* Determine if we are reading a top-level (i.e. user-level) Object
   definition, as opposed to the definition of an Object contained
   within another Object. This is indicated by the current nesting
   level. */
   top = ( nest == -1 );

/* If reading a top-level object, determine if data lying in between
   Object definitions in the input data stream are to be skipped. */
   skip = ( top && astGetSkip( this ) );

/* Read the next input data item. If we are reading a top-level Object
   definition, skip any unrelated data beforehand. Otherwise read the
   data strictly as it comes (there should be no unrelated data
   embedded within Object definitions themselves). */
   astGetNextData( this, skip, &name, &class );

/* If no suitable data item was found (and no error occurred), we have
   reached the end of data. For a top-level Object a NULL Object
   pointer is simply returned, but for a nested Object this indicates
   that part of the Object definition is missing, so report an
   error. */
   if ( astOK ) {
      if ( !name ) {
         if ( !top ) {
            astError( AST__EOCHN,
                      "astRead(%s): End of input encountered while trying to "
                      "read an AST Object.", astGetClass( this ) );
         }

/* If a data item was found, check it is a "Begin" item. If not, there
   is a data item missing, so report an error and free all memory. */
      } else if ( strcmp( name, "begin" ) ) {
         astError( AST__BADIN,
                   "astRead(%s): Missing \"Begin\" when expecting an Object.",
                   astGetClass( this ) );
         name = astFree( name );
         if ( class ) class = astFree( class );

/* If the required "Begin" item was found, free the memory used for the
   name string. */
      } else {
         name = astFree( name );

/* Use the associated class name to locate the loader for that
   class. This function will then be used to build the Object. */
         loader = astGetLoader( class );

/* Extend all necessary stack arrays to accommodate entries for the
   next nesting level (this allocates space if none has yet been
   allocated). */
         end_of_object = astGrow( end_of_object, nest + 2, sizeof( int ) );
         object_class = astGrow( object_class, nest + 2, sizeof( char * ) );
         values_class = astGrow( values_class, nest + 2, sizeof( char * ) );
         values_list = astGrow( values_list, nest + 2, sizeof( Value * ) );
         values_ok = astGrow( values_ok, nest + 2, sizeof( int ) );

/* If an error occurred, free the memory used by the class string,
   which will not now be used. */
         if ( !astOK ) {
            class = astFree( class );

/* Otherwise, increment the nesting level and initialise the new stack
   elements for this new level. This includes clearing the
   "end_of_object" flag so that ReadClassData can read more data, and
   storing the class name of the object we are about to read. */
         } else {
            nest++;
            end_of_object[ nest ] = 0;
            object_class[ nest ] = class;
            values_class[ nest ] = NULL;
            values_list[ nest ] = NULL;
            values_ok[ nest ] = 0;

/* Invoke the loader, which reads the Object definition from the input
   data stream and builds the Object. Supply NULL/zero values to the
   loader so that it will substitute values appropriate to its own
   class. */
            new = (*loader)( NULL, (size_t) 0, 0, NULL, NULL, this );

/* Clear the values list for the current nesting level. If the list
   has not been read or any Values remain in it, an error will
   result. */
            ClearValues( this );

/* If no error has yet occurred, check that the "end_of_object" flag
   has been set. If not, the input data were not correctly terminated,
   so report an error. */
            if ( astOK && !end_of_object[ nest ] ) {
               astError( AST__BADIN,
                         "astRead(%s): Unexpected end of input (missing end "
                         "of %s).",
                         astGetClass( this ), object_class[ nest ] );
            }

/* If an error occurred, report contextual information. Only do this
   for top-level Objects to avoid multple messages. */
            if ( !astOK && top ) {
               astError( astStatus, "Error while reading a %s from a %s.",
                         class, astGetClass( this ) );
            }

/* Clear the Object's class string, freeing the associated memory
   (note this is the memory allocated for the "class" string
   earlier). */
            object_class[ nest ] = astFree( object_class[ nest ] );

/* Restore the previous nesting level. */
            nest--;
         }

/* Once the top-level Object has been built, free the memory used by
   the stack arrays. */
         if ( top ) {
            end_of_object = astFree( end_of_object );
            object_class = astFree( object_class );
            values_class = astFree( values_class );
            values_list = astFree( values_list );
            values_ok = astFree( values_ok );
         }
      }
   }

/* If an error occurred, clean up by deleting the new Object and
   return a NULL pointer. */
   if ( !astOK ) new = astDelete( new );

/* Return the pointer to the new Object. */
   return new;
}

static void ReadClassData( AstChannel *this, const char *class ) {
/*
*+
*  Name:
*     astReadClassData

*  Purpose:
*     Read values from a data source for a class loader.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astReadClassData( AstChannel *this, const char *class )

*  Class Membership:
*     Channel method.

*  Description:
*     This function reads the data for a class from the data source
*     associated with a Channel, so as to provide values for
*     initialising the instance variables of that class as part of
*     building a complete Object. This function should be invoked by
*     the loader for each class immediately before it attempts to read
*     these values.
*
*     The values read are placed into the current values list by this
*     function. They may then be read from this list by the class
*     loader making calls to astReadDouble, astReadInt, astReadObject
*     and astReadString. The order in which values are read by the
*     loader is unimportant (although using the same order for reading
*     as for writing will usually be more efficient) and values are
*     removed from the list as they are read.

*  Parameters:
*     this
*        Pointer to the Channel.
*     class
*        A pointer to a constant null-terminated string containing the
*        name of the class whose loader is requesting the data (note
*        this is not usually the same as the class name of the Object
*        being built). This value allows the class structure of the
*        input data to be validated.
*-
*/

/* Local Variables: */
   AstObject *object;            /* Pointer to new Object */
   Value *value;                 /* Pointer to Value structure */
   char *name;                   /* Pointer to data item name string */
   char *val;                    /* Pointer to data item value string */
   int done;                     /* All class data read? */
   static int msg;               /* Contextual error message reported? */

/* Check the global error status. */
   if ( !astOK ) return;

/* If the "values_ok" flag is set, this indicates that the values list
   (at the current nesting level) has been filled by a previous
   invocation of this function and has then been read by the
   appropriate class loader. In this case, clear any entries which may
   remain in the current values list. If any such entries are found,
   they represent input data that were not read, so an error will
   result. */
   if ( values_ok[ nest ] ) ClearValues( this );

/* If "values_class" is non-NULL, this indicates that the values list
   (at the current nesting level) has been filled by a previous
   invocation of this function, but that the values belong to a class
   whose loader has not yet tried to read them. In this case, we must
   continue to keep the values until they are needed, so we do not
   read any more input data this time. */
   if ( values_class[ nest ] ) {

/* If the class to which the previously saved values belong matches
   the class we now want values for, set the "values_ok" flag. This
   then allows the values to be accessed (by LookupValue). */
      values_ok[ nest ] = !strcmp( values_class[ nest ], class );

/* If the current values list is empty, we must read in values for the
   next class that appears in the input data. However, first check
   that the "end_of_object" flag has not been set. If it has, we have
   already reached the end of this Object's data, so there is some
   kind of problem with the order in which class loaders have been
   invoked. This will probably never happen, but report an error if
   necessary. */
   } else if ( end_of_object[ nest ] ) {
      astError( AST__LDERR,
                "astRead(%s): Invalid attempt to read further %s data "
                "following an end of %s.",
                astGetClass( this ), class, object_class[ nest ] );
      astError( AST__LDERR,
                "Perhaps the wrong class loader has been invoked?" );

/* If we need new values, loop to read input data items until the end
   of the data for a class is reached. */
   } else {
      done = 0;   
      while ( astOK && !done ) {

/* Read the next input data item. */
         astGetNextData( this, 0, &name, &val );
         if ( astOK ) {

/* Unexpected end of input. */
/* ------------------------ */
/* If no "name" value is returned, we have reached the end of the
   input data stream without finding the required end of class
   terminator, so report an error. */
            if ( !name ) {
               astError( AST__EOCHN,
                         "astRead(%s): Unexpected end of input (missing end "
                         "of %s).",
                         astGetClass( this ), object_class[ nest ] );

/* "IsA" item. */
/* ----------- */
/* Otherwise, if an "IsA" item was read, it indicates the end of the
   data for a class. Store the pointer to the name of this class in
   "values_class" and note whether this is the class whose data we
   wanted in "values_ok". If the data we have read do not belong to
   the class we wanted, they will simply be kept until the right class
   comes looking for them. */
            } else if ( !strcmp( name, "isa" ) ) {
               values_class[ nest ] = val;
               values_ok[ nest ] = !strcmp( val, class );

/* Free the memory holding the name string. */
               name = astFree( name );

/* Note we have finished reading class data. */
               done = 1;

/* "End" item. */
/* ----------- */
/* If an "End" item was read, it indicates the end of the data both
   for a class and for the current Object definition as a whole. Set
   the "end_of_object" flag (for the current nesting level) which
   prevents any further data being read for this Object. This flag is
   also used (by Read) to check that an "End" item was actually
   read. */
            } else if ( !strcmp( name, "end" ) ) {
               end_of_object[ nest ] = 1;

/* Check that the class name in the "End" item matches that of the
   Object being built. If so, store the pointer to the name of this
   class in "values_class" and note whether this is the class whose
   data we wanted in "values_ok". If the data we have read do not
   belong to the class we wanted, they will simply be kept until the
   right class comes looking for them. */
               if ( !strcmp( val, object_class[ nest ] ) ) {
                  values_class[ nest ] = val;
                  values_ok[ nest ] = !strcmp( class, val );

/* If the "End" item contains the wrong class name (i.e. not matching
   the corresponding "Begin" item), then report an error. */
               } else {
                  astError( AST__BADIN,
                            "astRead(%s): Bad class structure in input data.",
                            astGetClass( this ) );
                  astError( AST__BADIN,
                            "End of %s read when expecting end of %s.",
                            val, object_class[ nest ] );

/* Free the memory used by the class string, which will not now be
   used. */
                  val = astFree( val );
               }

/* Free the memory holding the name string. */
               name = astFree( name );

/* Note we have finished reading class data. */
               done = 1;

/* String value. */
/* ------------- */
/* If any other name is obtained and "val" is not NULL, we have read a
   non-Object value, encoded as a string. Allocate memory for a Value
   structure to describe it. */
            } else if ( val ) {
               value = astMalloc( sizeof( Value ) );
               if ( astOK ) {

/* Store pointers to the name and value string in the Value structure
   and note this is not an Object value. */
                  value->name = name;
                  value->ptr.string = val;
                  value->is_object = 0;

/* Append the Value structure to the values list for the current
   nesting level. */
                  AppendValue( value, values_list + nest );

/* If an error occurred, free the memory holding the "name" and "val"
   strings. */
               } else {
                  name = astFree( name );
                  val = astFree( val );
               }

/* Object value. */
/* ------------- */
/* If "val" is NULL, we have read an Object item, and the Object
   definition should follow. Allocate memory for a Value structure to
   describe it. */
            } else {
               value = astMalloc( sizeof( Value ) );

/* Invoke astRead to read the Object definition from subsequent data
   items and to build the Object, returning a pointer to it. This will
   result in recursive calls to the current function, but as these
   will use higher nesting levels they will not interfere with the
   current invocation. */
               msg = 0;
               object = astRead( this );
               if ( astOK ) {

/* Store pointers to the name and Object in the Value structure and
   note this is an Object value. */
                  value->name = name;
                  value->ptr.object = object;
                  value->is_object = 1;

/* Append the Value structure to the values list for the current
   nesting level. */
                  AppendValue( value, values_list + nest );

/* If an error occurred, report a contextual error maessage and set
   the "msg" flag (this prevents multiple messages if this function is
   invoked recursively to deal with nested Objects). */
               } else {
                  if ( !msg ) {
                     astError( astStatus,
                               "Failed to read the \"%s\" Object value.",
                               name );
                     msg = 1;
                  }

/* Free the memory holding the "name" string and any Value structure
   that was allocated. */
                  name = astFree( name );
                  value = astFree( value );
               }
            }
         }
      }
   }
}

static double ReadDouble( AstChannel *this, const char *name, double def ) {
/*
*+
*  Name:
*     astReadDouble

*  Purpose:
*     Read a double value as part of loading a class.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     double astReadDouble( AstChannel *this, const char *name, double def )

*  Class Membership:
*     Channel method.

*  Description:
*     This function searches the current values list of a Channel to
*     identify a double value with a specified name. If such a value
*     is found, it is returned, otherwise a default value is returned
*     instead.
*
*     This function should only be invoked from within the loader
*     function associated with a class, in order to return a double
*     value to be assigned to an instance variable. It must be
*     preceded by a call to the astReadClassData function, which loads
*     the values associated with the class into the current values
*     list from the input data source.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated character string
*        containing the name of the required value. This must be in
*        lower case with no surrounding white space. Note that names
*        longer than 6 characters will not match any value.
*     def
*        If no suitable value can be found (e.g. it is absent from the
*        data stream being read), then this value will be returned
*        instead.

*  Returned Value:
*     The required value, or the default if the value was not found.

*  Notes:
*     - A value of 0.0 will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-
*/

/* Local Variables: */
   Value *value;                 /* Pointer to required Value structure */
   double result;                /* Value to be returned */
   int nc;                       /* Number of characters read by sscanf */
   
/* Initialise. */
   result = 0.0;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Search for a Value structure with the required name in the current
   values list.*/
   value = LookupValue( name );
   if ( astOK ) {

/* If a Value was found, check that it describes a string (as opposed
   to an Object). */
      if ( value ) {
         if ( !value->is_object ) {

/* If so, then attempt to decode the string to give a double value,
   checking that the entire string is read. If this fails, then the
   wrong name has probably been given, or the input data are corrupt,
   so report an error. */
            nc = 0;
            if ( !( ( 1 == sscanf( value->ptr.string, " %lf %n",
                                                      &result, &nc ) )
                    && ( nc >= (int) strlen( value->ptr.string ) ) ) ) {
               astError( AST__BADIN,
                         "astRead(%s): The value \"%s = %s\" cannot "
                         "be read as a double precision floating point "
                         "number.", astGetClass( this ),
                         value->name, value->ptr.string );
            }

/* Report a similar error if the Value does not describe a string. */
         } else {
            astError( AST__BADIN,
                      "astRead(%s): The Object \"%s = <%s>\" cannot "
                      "be read as a double precision floating point number.",
                      astGetClass( this ),
                      value->name, astGetClass( value->ptr.object ) );
         }

/* Free the Value structure and the resources it points at. */
         value = FreeValue( value );

/* If no suitable Value structure was found, then use the default
   value instead. */
      } else {
         result = def;
      }
   }

/* Return the result. */
   return result;
}

static int ReadInt( AstChannel *this, const char *name, int def ) {
/*
*+
*  Name:
*     astReadInt

*  Purpose:
*     Read an int value as part of loading a class.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     int astReadInt( AstChannel *this, const char *name, int def )

*  Class Membership:
*     Channel method.

*  Description:
*     This function searches the current values list of a Channel to
*     identify an int value with a specified name. If such a value is
*     found, it is returned, otherwise a default value is returned
*     instead.
*
*     This function should only be invoked from within the loader
*     function associated with a class, in order to return an int
*     value to be assigned to an instance variable. It must be
*     preceded by a call to the astReadClassData function, which loads
*     the values associated with the class into the current values
*     list from the input data source.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated character string
*        containing the name of the required value. This must be in
*        lower case with no surrounding white space. Note that names
*        longer than 6 characters will not match any value.
*     def
*        If no suitable value can be found (e.g. it is absent from the
*        data stream being read), then this value will be returned
*        instead.

*  Returned Value:
*     The required value, or the default if the value was not found.

*  Notes:
*     - A value of zero will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-
*/

/* Local Variables: */
   Value *value;                 /* Pointer to required Value structure */
   int nc;                       /* Number of characters read by sscanf */
   int result;                   /* Value to be returned */
   
/* Initialise. */
   result = 0;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Search for a Value structure with the required name in the current
   values list.*/
   value = LookupValue( name );
   if ( astOK ) {

/* If a Value was found, check that it describes a string (as opposed
   to an Object). */
      if ( value ) {
         if ( !value->is_object ) {

/* If so, then attempt to decode the string to give an int value,
   checking that the entire string is read. If this fails, then the
   wrong name has probably been given, or the input data are corrupt,
   so report an error. */
            nc = 0;
            if ( !( ( 1 == sscanf( value->ptr.string, " %d %n",
                                                      &result, &nc ) )
                    && ( nc >= (int) strlen( value->ptr.string ) ) ) ) {
               astError( AST__BADIN,
                         "astRead(%s): The value \"%s = %s\" cannot "
                         "be read as an integer.", astGetClass( this ),
                         value->name, value->ptr.string );
            }

/* Report a similar error if the Value does not describe a string. */
         } else {
            astError( AST__BADIN,
                      "astRead(%s): The Object \"%s = <%s>\" cannot "
                      "be read as an integer.", astGetClass( this ),
                      value->name, astGetClass( value->ptr.object ) );
         }

/* Free the Value structure and the resources it points at. */
         value = FreeValue( value );

/* If no suitable Value structure was found, then use the default
   value instead. */
      } else {
         result = def;
      }
   }

/* Return the result. */
   return result;
}

static AstObject *ReadObject( AstChannel *this, const char *name,
                              AstObject *def ) {
/*
*+
*  Name:
*     astReadObject

*  Purpose:
*     Read a (sub)Object as part of loading a class.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     AstObject *astReadObject( AstChannel *this, const char *name,
*                               AstObject *def )

*  Class Membership:
*     Channel method.

*  Description:
*     This function searches the current values list of a Channel to
*     identify an Object with a specified name. If such an Object is
*     found, a pointer to it is returned, otherwise a default pointer
*     is returned instead.
*
*     This function should only be invoked from within the loader
*     function associated with a class, in order to return an Object
*     pointer value to be assigned to an instance variable. It must be
*     preceded by a call to the astReadClassData function, which loads
*     the values associated with the class into the current values
*     list from the input data source.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated character string
*        containing the name of the required Object. This must be in
*        lower case with no surrounding white space. Note that names
*        longer than 6 characters will not match any Object.
*     def
*        If no suitable Object can be found (e.g. the Object is absent
*        from the data stream being read), then a clone of this
*        default Object pointer will be returned instead (or NULL if
*        this default pointer is NULL).

*  Returned Value:
*     A pointer to the Object, or a clone of the default pointer if
*     the Object was not found.

*  Notes:
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-
*/

/* Local Variables: */
   AstObject *result;            /* Pointer value to return */
   Value *value;                 /* Pointer to required Value structure */

/* Initialise. */
   result = NULL;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Search for a Value structure with the required name in the current
   values list.*/
   value = LookupValue( name );
   if ( astOK ) {

/* If a Value was found, check that it describes an Object (as opposed to
   a string). */
      if ( value ) {
         if ( value->is_object ) {

/* If so, then extract the Object pointer, replacing it with NULL. */
            result = value->ptr.object;
            value->ptr.object = NULL;

/* If the Value does not describe an Object, then the wrong name has
   probably been given, or the input data are corrupt, so report an
   error. */
         } else {
            astError( AST__BADIN,
                      "astRead(%s): The value \"%s = %s\" cannot be "
                      "read as an Object.", astGetClass( this ),
                      value->name, value->ptr.string );
         }

/* Free the Value structure and the resources it points at. */
         value = FreeValue( value );

/* If no suitable Value structure was found, clone the default
   pointer, if given. */
      } else if ( def ) {
         result = astClone( def );
      }
   }

/* Return the result. */
   return result;
}

static char *ReadString( AstChannel *this, const char *name,
                         const char *def ) {
/*
*+
*  Name:
*     astReadString

*  Purpose:
*     Read a string value as part of loading a class.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     char *astReadString( AstChannel *this, const char *name,
*                          const char *def )

*  Class Membership:
*     Channel method.

*  Description:
*     This function searches the current values list of a Channel to
*     identify a string value with a specified name. If such a value
*     is found, a pointer to the string is returned, otherwise a
*     pointer to a copy of a default string is returned instead.
*
*     This function should only be invoked from within the loader
*     function associated with a class, in order to return a string
*     pointer value to be assigned to an instance variable. It must be
*     preceded by a call to the astReadClassData function, which loads
*     the values associated with the class into the current values
*     list from the input data source.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated character string
*        containing the name of the required value. This must be in
*        lower case with no surrounding white space. Note that names
*        longer than 6 characters will not match any value.
*     def
*        If no suitable string can be found (e.g. the value is absent
*        from the data stream being read), then a dynamically
*        allocated copy of the null-terminated string pointed at by
*        "def" will be made, and a pointer to this copy will be
*        returned instead (or NULL if this default pointer is NULL).

*  Returned Value:
*     A pointer to a dynamically allocated null-terminated string
*     containing the value required, or to a copy of the default
*     string if the value was not found (or NULL if the "def" pointer
*     was NULL).

*  Notes:
*     - It is the caller's responsibility to arrange for the memory
*     holding the returned string to be freed (using astFree) when it
*     is no longer required.
*     - A NULL pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-
*/

/* Local Variables: */
   Value *value;                 /* Pointer to required Value structure */
   char *result;                 /* Pointer value to return */

/* Initialise. */
   result = NULL;   

/* Check the global error status. */
   if ( !astOK ) return result;

/* Search for a Value structure with the required name in the current
   values list.*/
   value = LookupValue( name );
   if ( astOK ) {

/* If a Value was found, check that it describes a string (as opposed
   to an Object). */
      if ( value ) {
         if ( !value->is_object ) {

/* If so, then extract the string pointer, replacing it with NULL. */
            result = value->ptr.string;
            value->ptr.string = NULL;

/* If the Value does not describe a string, then the wrong name has
   probably been given, or the input data are corrupt, so report an
   error. */
         } else {
            astError( AST__BADIN,
                      "astRead(%s): The Object \"%s = <%s>\" cannot "
                      "be read as a string.", astGetClass( this ),
                      value->name, astGetClass( value->ptr.object ) );
         }

/* Free the Value structure and the resources it points at. */
         value = FreeValue( value );

/* If no suitable Value structure was found, then make a dynamic copy
   of the default string (if given) and return a pointer to this. */
      } else if ( def ) {
         result = astStore( NULL, def, strlen( def ) + (size_t) 1 );
      }
   }

/* Return the result. */
   return result;
}

static void RemoveValue( Value *value, Value **head ) {
/*
*  Name:
*     RemoveValue

*  Purpose:
*     Remove a Value structure from a circular linked list.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void RemoveValue( Value *value, Value **head );

*  Class Membership:
*     Channel member function.

*  Description:
*     This function removes a Value structure from a doubly linked
*     circular list of such structures. The "head of list" pointer is
*     updated to point at the element following the one removed.

*  Parameters:
*     value
*        Pointer to the structure to be removed (note that this must
*        actually be in the list, although this function does not
*        check).
*     head
*        Address of a pointer to the element at the head of the
*        list. This pointer will be updated to point at the list
*        element that follows the one removed. If the list becomes
*        empty, the pointer will be set to NULL.

*  Notes:
*     - This function does not perform error chacking and does not
*     generate errors.
*/

/* Remove the Value structure from the list by re-establishing links
   between the elements on either side of it. */
   value->blink->flink = value->flink;
   value->flink->blink = value->blink;

/* Update the head of list pointer to identify the following
   element. */
   *head = value->flink;

/* If the head of list identifies the removed element, then note that
   the list is now empty. */
   if ( *head == value ) *head = NULL;

/* Make the removed element point at itself. */
   value->flink = value;
   value->blink = value;
}

static void SetAttrib( AstObject *this_object, const char *setting ) {
/*
*  Name:
*     astSetAttrib

*  Purpose:
*     Set an attribute value for a Channel.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void SetAttrib( AstObject *this, const char *setting )

*  Class Membership:
*     Channel member function (over-rides the astSetAttrib protected
*     method inherited from the Object class).

*  Description:
*     This function assigns an attribute value for a Channel, the
*     attribute and its value being specified by means of a string of
*     the form:
*
*        "attribute= value "
*
*     Here, "attribute" specifies the attribute name and should be in
*     lower case with no white space present. The value to the right
*     of the "=" should be a suitable textual representation of the
*     value to be assigned and this will be interpreted according to
*     the attribute's data type.  White space surrounding the value is
*     only significant for string attributes.

*  Parameters:
*     this
*        Pointer to the Channel.
*     setting
*        Pointer to a null terminated string specifying the new attribute
*        value.
*/

/* Local Variables: */
   AstChannel *this;             /* Pointer to the Channel structure */
   int comment;                  /* Comment attribute value */
   int full;                     /* Full attribute value */
   int len;                      /* Length of setting string */
   int nc;                       /* Number of characters read by "sscanf" */
   int skip;                     /* Skip attribute value */

/* Check the global error status. */
   if ( !astOK ) return;

/* Obtain a pointer to the Channel structure. */
   this = (AstChannel *) this_object;

/* Obtain the length of the setting string. */
   len = (int) strlen( setting );

/* Test for each recognised attribute in turn, using "sscanf" to parse
   the setting string and extract the attribute value (or an offset to
   it in the case of string values). In each case, use the value set
   in "nc" to check that the entire string was matched. Once a value
   has been obtained, use the appropriate method to set it. */

/* Comment. */
/* ---------*/
   if ( nc = 0,
        ( 1 == sscanf( setting, "comment= %d %n", &comment, &nc ) )
        && ( nc >= len ) ) {
      astSetComment( this, comment );

/* Full. */
/* ----- */
   } else if ( nc = 0,
               ( 1 == sscanf( setting, "full= %d %n", &full, &nc ) )
               && ( nc >= len ) ) {
      astSetFull( this, full );

/* Skip. */
/* ----- */
   } else if ( nc = 0,
               ( 1 == sscanf( setting, "skip= %d %n", &skip, &nc ) )
               && ( nc >= len ) ) {
      astSetSkip( this, skip );

/* If the attribute is still not recognised, pass it on to the parent
   method for further interpretation. */
   } else {
      (*parent_setattrib)( this_object, setting );
   }
}

static void SinkWrap( void (* sink)( const char * ), const char *line ) {
/*
*  Name:
*     SinkWrap

*  Purpose:
*     Wrapper function to invoke a C Channel sink function.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     static void SinkWrap( void (* sink)( const char * ), const char *line )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function invokes the sink function whose pointer is
*     supplied in order to write an output line to an external data
*     store.

*  Parameters:
*     sink
*        Pointer to a sink function, whose single parameter is a
*        pointer to a const, null-terminated string containing the
*        text to be written, and which returns void. This is the form
*        of Channel sink function employed by the C language interface
*        to the AST library.
*/

/* Check the global error status. */
   if ( !astOK ) return;

/* Invoke the sink function. */
   ( *sink )( line );
}

static char *SourceWrap( const char *(* source)( void ) ) {
/*
*  Name:
*     SourceWrap

*  Purpose:
*     Wrapper function to invoke a C Channel source function.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     static char *SourceWrap( const char *(* source)( void ) )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function invokes the source function whose pointer is
*     supplied in order to read the next input line from an external
*     data store. It then returns a pointer to a dynamic string
*     containing a copy of the text that was read.

*  Parameters:
*     source
*        Pointer to a source function, with no parameters, that
*        returns a pointer to a const, null-terminated string
*        containing the text that it read. This is the form of Channel
*        source function employed by the C language interface to the
*        AST library.

*  Returned Value:
*     A pointer to a dynamically allocated, null terminated string
*     containing a copy of the text that was read. This string must be
*     freed by the caller (using astFree) when no longer required.
*
*     A NULL pointer will be returned if there is no more input text
*     to read.

*  Notes:
*     - A NULL pointer value will be returned if this function is
*     invoked with the global error status set or if it should fail
*     for any reason.
*/

/* Local Variables: */
   char *result;                 /* Pointer value to return */
   const char *line;             /* Pointer to input line */

/* Initialise. */
   result = NULL;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Invoke the source function to read the next input line and return a
   pointer to the resulting string. */
   line = ( *source )();

/* If a string was obtained, make a dynamic copy of it and save the
   resulting pointer. */
   if ( line ) result = astString( line, (int) strlen( line ) );

/* Return the result. */
   return result;
}

static int TestAttrib( AstObject *this_object, const char *attrib ) {
/*
*  Name:
*     TestAttrib

*  Purpose:
*     Test if a specified attribute value is set for a Channel.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     int TestAttrib( AstObject *this, const char *attrib )

*  Class Membership:
*     Channel member function (over-rides the astTestAttrib protected
*     method inherited from the Object class).

*  Description:
*     This function returns a boolean result (0 or 1) to indicate whether
*     a value has been set for one of a Channel's attributes.

*  Parameters:
*     this
*        Pointer to the Channel.
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
   AstChannel *this;             /* Pointer to the Channel structure */
   int result;                   /* Result value to return */

/* Initialise. */
   result = 0;

/* Check the global error status. */
   if ( !astOK ) return result;

/* Obtain a pointer to the Channel structure. */
   this = (AstChannel *) this_object;

/* Check the attribute name and test the appropriate attribute. */

/* Comment. */
/* -------- */
   if ( !strcmp( attrib, "comment" ) ) {
      result = astTestComment( this );

/* Full. */
/* ----- */
   } else if ( !strcmp( attrib, "full" ) ) {
      result = astTestFull( this );

/* Skip. */
/* ----- */
   } else if ( !strcmp( attrib, "skip" ) ) {
      result = astTestSkip( this );

/* If the attribute is still not recognised, pass it on to the parent
   method for further interpretation. */
   } else {
      result = (*parent_testattrib)( this_object, attrib );
   }

/* Return the result, */
   return result;
}

static void Unquote( AstChannel *this, char *str ) {
/*
*  Name:
*     Unquote

*  Purpose:
*     Remove quotes from a (possibly) quoted string.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     void Unquote( AstChannel *this, char *str )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function removes one layer of quote characters (") from a
*     string which is possibly quoted. Any quotes within quotes (which
*     should have been doubled when the string was originally quoted)
*     are also converted back to single quotes again.
*
*     The quotes need not start or end at the ends of the string, and
*     there may be any number of quoted sections within the string. No
*     error results if the string does not contain any quotes at all
*     (it is simply returned unchanged), but an error results if any
*     unmatched quotes are found.

*  Parameters:
*     this
*        Pointer to a Channel. This is only used for constructing error
*        messages and has no influence on the string processing.
*     str
*        Pointer to the null-terminated string to be processed. This
*        is modified in place. The new string starts at the same
*        location as the original but has a new null character
*        appended if necessary (it will usually be shorter than the
*        original).
*/

/* Local Variables: */
   int i;                        /* Loop counter for "input" characters */
   int j;                        /* Counter for "output" characters */
   int quoted;                   /* Inside a quoted string? */

/* Check the global error status. */
   if ( !astOK ) return;

/* Loop to inspect each character in the string. */
   quoted = 0;
   for ( i = j = 0; str[ i ]; i++ ) {

/* Non-quote characters are simply copied to their new location in the
   string. */
      if ( str[ i ] != '"' ) {
         str[ j++ ] = str[ i ];

/* If a quote character '"' is encountered and we are not already in a
   quoted string, then note the start of a quoted string (and discard
   the quote). */
      } else if ( !quoted ) {
         quoted = 1;

/* If a quote character is encountered inside a quoted string, then
   check if the next character is also a quote. If so, convert this
   double quote to a single one. */
      } else if ( str[ i + 1 ] == '"' ) {
         str[ j++ ] = '"';
         i++;

/* If a single quote character is encountered inside a quoted string,
   then note the end of the quoted string (and discard the quote). */
      } else {
         quoted = 0;
      }
   }

/* Append a null to terminate the processed string. */
   str[ j ] = '\0';

/* If the "quoted" flag is still set, then there were unmatched
   quotes, so report an error. */
   if ( quoted ) {
      astError( AST__UNMQT,
                "astRead(%s): Unmatched quotes in input data: %s.",
                astGetClass( this ), str );
   }
}

static int Use( AstChannel *this, int set, int helpful ) {
/*
*  Name:
*     Use

*  Purpose:
*     Decide whether to write a value to a data sink.

*  Type:
*     Private function.

*  Synopsis:
*     #include "channel.h"
*     int Use( AstChannel *this, int set, int helpful )

*  Class Membership:
*     Channel member function.

*  Description:
*     This function decides whether a value supplied by a class "Dump"
*     function, via a call to one of the astWrite... protected
*     methods, should actually be written to the data sink associated
*     with a Channel.
*
*     This decision is based on the settings of the "set" and
*     "helpful" flags supplied to the astWrite... method, plus the
*     attribute settings of the Channel.

*  Parameters:
*     this
*        A pointer to the Channel.
*     set
*        The "set" flag supplied.
*     helpful
*        The "helpful" value supplied.

*  Returned Value:
*     One if the value should be written out, otherwise zero.

*  Notes:
*     - A value of zero will be returned if this function is invoked
*     with the global error status set or if it should fail for any
*     reason.
*/

/* Local Variables: */
   int full;                     /* Full attribute value */
   int result;                   /* Result value to be returned */

/* Check the global error status. */
   if ( !astOK ) return 0;

/* If "set" is non-zero, then so is the result ("set" values must
   always be written out). */
   result = ( set != 0 );

/* Otherwise, obtain the value of the Channel's Full attribute. */
   if ( !set ) {
      full = astGetFull( this );

/* If Full is positive, display all values, if zero, display only
   "helpful" values, if negative, display no (un-"set") values. */
      if ( astOK ) result = ( ( helpful && ( full > -1 ) ) || ( full > 0 ) );
   }

/* Return the result. */
   return result;
}

static int Write( AstChannel *this, AstObject *object ) {
/*
*++
*  Name:
c     astWrite
f     AST_WRITE

*  Purpose:
*     Write an Object to a Channel.

*  Type:
*     Public function.

*  Synopsis:
c     #include "channel.h"
c     int astWrite( AstChannel *this, AstObject *object )
f     RESULT = AST_WRITE( THIS, OBJECT, STATUS )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes an Object to a Channel, appending it to any
*     previous Objects written to that Channel.

*  Parameters:
c     this
f     THIS = INTEGER (Given)
*        Pointer to the Channel.
c     object
f     OBJECT = INTEGER (Given)
*        Pointer to the Object which is to be written.
f     STATUS = INTEGER (Given and Returned)
f        The global status.

*  Returned Value:
c     astWrite()
f     AST_WRITE = INTEGER
*        The number of Objects written to the Channel by this
c        invocation of astWrite (normally, this will be one).
f        invocation of AST_WRITE (normally, this will be one).

*  Notes:
*     - A value of zero will be returned if this function is invoked
c     with the AST error status set, or if it should fail for any
f     with STATUS set to an error value, or if it should fail for any
*     reason.
*--
*/

/* Check the global error status. */
   if ( !astOK ) return 0;

/* The work of this function is actually performed by the protected
   astDump method of the Object. The fact that this is further
   encapsulated within the astWrite method (which belongs to the
   Channel) is simply a trick to allow it to be over-ridden either by
   a derived Channel, or a derived Object (or both), and hence to
   adapt to the nature of either argument. */
   astDump( object, this );

/* Return the number of Objects written. */
   return astOK ? 1 : 0;
}

static void WriteBegin( AstChannel *this, const char *class,
                        const char *comment ) {
/*
*+
*  Name:
*     astWriteBegin

*  Purpose:
*     Write a "Begin" data item to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astWriteBegin( AstChannel *this, const char *class,
*                         const char *comment )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes a "Begin" data item to the data sink
*     associated with a Channel, so as to begin the output of a new
*     Object definition.

*  Parameters:
*     this
*        Pointer to the Channel.
*     class
*        Pointer to a constant null-terminated string containing the
*        name of the class to which the Object belongs.
*     comment
*        Pointer to a constant null-terminated string containing a
*        textual comment to be associated with the "Begin"
*        item. Normally, this will describe the purpose of the Object.

*  Notes:
*     - The comment supplied may not actually be used, depending on
*     the nature of the Channel supplied.
*-
*/

/* Local Variables: */
   char *line;                   /* Pointer to dynamic output string */
   int i;                        /* Loop counter for indentation characters */
   int nc;                       /* Number of output characters */

/* Check the global error status. */
   if ( !astOK ) return;

/* Start building a dynamic string with an initial space. Then add
   further spaces to suit the current indentation level. */
   line = AppendString( NULL, &nc, " " );
   for ( i = 0; i < current_indent; i++ ) {
      line = AppendString( line, &nc, " " );
   }

/* Append the "Begin" keyword followed by the class name. */
   line = AppendString( line, &nc, "Begin " );
   line = AppendString( line, &nc, class );

/* If required, also append the comment. */
   if ( astGetComment( this ) && *comment ) {
      line = AppendString( line, &nc, " \t# " );
      line = AppendString( line, &nc, comment );
   }

/* Write out the resulting line of text. */
   OutputTextItem( this, line );

/* Free the dynamic string. */
   line = astFree( line );

/* Increment the indentation level and clear the count of items written
   for this Object. */
   current_indent += INDENT_INC;
   items_written = 0;
}

static void WriteDouble( AstChannel *this, const char *name,
                         int set, int helpful,
                         double value, const char *comment ) {
/*
*+
*  Name:
*     astWriteDouble

*  Purpose:
*     Write a double value to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astWriteDouble( AstChannel *this, const char *name,
*                          int set, int helpful,
*                          double value, const char *comment )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes a named double value, representing the
*     value of a class instance variable, to the data sink associated
*     with a Channel. It is intended for use by class "Dump" functions
*     when writing out class information which will subsequently be
*     re-read.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated string containing the
*        name to be used to identify the value in the external
*        representation. This will form the key for identifying it
*        again when it is re-read. The name supplied should be unique
*        within its class.
*
*        Mixed case may be used and will be preserved in the external
*        representation (where possible) for cosmetic effect. However,
*        case is not significant when re-reading values.
*
*        It is recommended that a maximum of 6 alphanumeric characters
*        (starting with an alphabetic character) be used. This permits
*        maximum flexibility in adapting to standard external data
*        representations (e.g. FITS).
*     set
*        If this is zero, it indicates that the value being written is
*        a default value (or can be re-generated from other values) so
*        need not necessarily be written out. Such values will
*        typically be included in the external representation with
*        (e.g.) a comment character so that they are available to
*        human readers but will be ignored when re-read. They may also
*        be completely omitted in some circumstances.
*
*        If "set" is non-zero, the value will always be explicitly
*        included in the external representation so that it can be
*        re-read.
*     helpful
*        This flag provides a hint about whether a value whose "set"
*        flag is zero (above) should actually appear at all in the
*        external representaton.
*
*        If the external representation allows values to be "commented
*        out" then, by default, values will be included in this form
*        only if their "helpful" flag is non-zero. Otherwise, they
*        will be omitted entirely. When possible, omitting the more
*        obscure values associated with a class is recommended in
*        order to improve readability.
*
*        This default behaviour may be further modified if the
*        Channel's Full attribute is set - either to permit all values
*        to be shown, or to suppress non-essential information
*        entirely.
*     value
*        The value to be written.
*     comment
*        Pointer to a constant null-terminated string containing a
*        textual comment to be associated with the value.
*
*        Note that this comment may not actually be used, depending on
*        the nature of the Channel supplied and the setting of its
*        Comment attribute.
*-
*/

/* Local Constants: */
#define BUFF_LEN 100             /* Size of local formatting buffer */

/* Local Variables: */
   char *line;                   /* Pointer to dynamic output string */
   char buff[ BUFF_LEN + 1 ];    /* Local formatting buffer */
   int i;                        /* Loop counter for indentation characters */
   int nc;                       /* Number of output characters */

/* Check the global error status. */
   if ( !astOK ) return;

/* Use the "set" and "helpful" flags, along with the Channel's
   attributes to decide whether this value should actually be
   written. */
   if ( Use( this, set, helpful ) ) {

/* Start building a dynamic string with an initial space, or a comment
   character if "set" is zero. Then add further spaces to suit the
   current indentation level. */
      line = AppendString( NULL, &nc, set ? " " : "#" );
      for ( i = 0; i < current_indent; i++ ) {
         line = AppendString( line, &nc, " " );
      }

/* Append the name string followed by " = ". */
      line = AppendString( line, &nc, name );
      line = AppendString( line, &nc, " = " );

/* Format the value as a string and append this. Make sure "-0" isn't
   produced. */
      (void) sprintf( buff, "%.*g", DBL_DIG, value );
      if ( !strcmp( buff, "-0" ) ) {
         buff[ 0 ] = '0';
         buff[ 1 ] = '\0';
      }
      line = AppendString( line, &nc, buff );

/* If required, also append the comment. */
      if ( astGetComment( this ) && *comment ) {
         line = AppendString( line, &nc, " \t# " );
         line = AppendString( line, &nc, comment );
      }

/* Write out the resulting line of text. */
      OutputTextItem( this, line );

/* Free the dynamic string. */
      line = astFree( line );
   }

/* Undefine macros local to this function. */
#undef BUFF_LEN
}

static void WriteEnd( AstChannel *this, const char *class ) {
/*
*+
*  Name:
*     astWriteEnd

*  Purpose:
*     Write an "End" data item to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astWriteEnd( AstChannel *this, const char *class )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes an "End" data item to the data sink
*     associated with a Channel. This item delimits the end of an
*     Object definition.

*  Parameters:
*     this
*        Pointer to the Channel.
*     class
*        Pointer to a constant null-terminated string containing the
*        class name of the Object whose definition is being terminated
*        by the "End" item.
*-
*/

/* Local Variables: */
   char *line;                   /* Pointer to dynamic output string */
   int i;                        /* Loop counter for indentation characters */
   int nc;                       /* Number of output characters */

/* Check the global error status. */
   if ( !astOK ) return;

/* Decrement the indentation level so that the "End" item matches the
   corresponding "Begin" item. */
   current_indent -= INDENT_INC;

/* Start building a dynamic string with an initial space. Then add
   further spaces to suit the current indentation level. */
   line = AppendString( NULL, &nc, " " );
   for ( i = 0; i < current_indent; i++ ) {
      line = AppendString( line, &nc, " " );
   }

/* Append the "End" keyword followed by the class name. */
   line = AppendString( line, &nc, "End " );
   line = AppendString( line, &nc, class );

/* Write out the resulting line of text. */
   OutputTextItem( this, line );

/* Free the dynamic string. */
   line = astFree( line );
}

static void WriteInt( AstChannel *this, const char *name, int set, int helpful,
                      int value, const char *comment ) {
/*
*+
*  Name:
*     astWriteInt

*  Purpose:
*     Write an integer value to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astWriteInt( AstChannel *this, const char *name,
*                       int set, int helpful,
*                       int value, const char *comment )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes a named integer value, representing the
*     value of a class instance variable, to the data sink associated
*     with a Channel. It is intended for use by class "Dump" functions
*     when writing out class information which will subsequently be
*     re-read.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated string containing the
*        name to be used to identify the value in the external
*        representation. This will form the key for identifying it
*        again when it is re-read. The name supplied should be unique
*        within its class.
*
*        Mixed case may be used and will be preserved in the external
*        representation (where possible) for cosmetic effect. However,
*        case is not significant when re-reading values.
*
*        It is recommended that a maximum of 6 alphanumeric characters
*        (starting with an alphabetic character) be used. This permits
*        maximum flexibility in adapting to standard external data
*        representations (e.g. FITS).
*     set
*        If this is zero, it indicates that the value being written is
*        a default value (or can be re-generated from other values) so
*        need not necessarily be written out. Such values will
*        typically be included in the external representation with
*        (e.g.) a comment character so that they are available to
*        human readers but will be ignored when re-read. They may also
*        be completely omitted in some circumstances.
*
*        If "set" is non-zero, the value will always be explicitly
*        included in the external representation so that it can be
*        re-read.
*     helpful
*        This flag provides a hint about whether a value whose "set"
*        flag is zero (above) should actually appear at all in the
*        external representaton.
*
*        If the external representation allows values to be "commented
*        out" then, by default, values will be included in this form
*        only if their "helpful" flag is non-zero. Otherwise, they
*        will be omitted entirely. When possible, omitting the more
*        obscure values associated with a class is recommended in
*        order to improve readability.
*
*        This default behaviour may be further modified if the
*        Channel's Full attribute is set - either to permit all values
*        to be shown, or to suppress non-essential information
*        entirely.
*     value
*        The value to be written.
*     comment
*        Pointer to a constant null-terminated string containing a
*        textual comment to be associated with the value.
*
*        Note that this comment may not actually be used, depending on
*        the nature of the Channel supplied and the setting of its
*        Comment attribute.
*-
*/

/* Local Constants: */
#define BUFF_LEN 50              /* Size of local formatting buffer */

/* Local Variables: */
   char *line;                   /* Pointer to dynamic output string */
   char buff[ BUFF_LEN + 1 ];    /* Local formatting buffer */
   int i;                        /* Loop counter for indentation characters */
   int nc;                       /* Number of output characters */

/* Check the global error status. */
   if ( !astOK ) return;

/* Use the "set" and "helpful" flags, along with the Channel's
   attributes to decide whether this value should actually be
   written. */
   if ( Use( this, set, helpful ) ) {

/* Start building a dynamic string with an initial space, or a comment
   character if "set" is zero. Then add further spaces to suit the
   current indentation level. */
      line = AppendString( NULL, &nc, set ? " " : "#" );
      for ( i = 0; i < current_indent; i++ ) {
         line = AppendString( line, &nc, " " );
      }

/* Append the name string followed by " = ". */
      line = AppendString( line, &nc, name );
      line = AppendString( line, &nc, " = " );

/* Format the value as a decimal string and append this. */
      (void) sprintf( buff, "%d", value );
      line = AppendString( line, &nc, buff );

/* If required, also append the comment. */
      if ( astGetComment( this ) && *comment ) {
         line = AppendString( line, &nc, " \t# " );
         line = AppendString( line, &nc, comment );
      }

/* Write out the resulting line of text. */
      OutputTextItem( this, line );

/* Free the dynamic string. */
      line = astFree( line );
   }

/* Undefine macros local to this function. */
#undef BUFF_LEN
}

static void WriteIsA( AstChannel *this, const char *class,
                      const char *comment ) {
/*
*+
*  Name:
*     astWriteIsA

*  Purpose:
*     Write an "IsA" data item to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astWriteIsA( AstChannel *this, const char *class,
*                       const char *comment )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes an "IsA" data item to the data sink
*     associated with a Channel. This item delimits the end of the
*     data associated with the instance variables of a class, as part
*     of an overall Object definition.

*  Parameters:
*     this
*        Pointer to the Channel.
*     class
*        Pointer to a constant null-terminated string containing the
*        name of the class whose data are terminated by the "IsA"
*        item.
*     comment
*        Pointer to a constant null-terminated string containing a
*        textual comment to be associated with the "IsA"
*        item. Normally, this will describe the purpose of the class
*        whose data are being terminated.

*  Notes:
*     - The comment supplied may not actually be used, depending on
*     the nature of the Channel supplied.
*-
*/

/* Local Variables: */
   char *line;                   /* Pointer to dynamic output string */
   int i;                        /* Loop counter for indentation characters */
   int nc;                       /* Number of output characters */

/* Check the global error status. */
   if ( !astOK ) return;

/* Output an "IsA" item only if there has been at least one item
   written since the last "Begin" or "IsA" item, or if the Full
   attribute for the Channel is greater than zero (requesting maximum
   information). */
   if ( items_written || astGetFull( this ) > 0 ) {

/* Start building a dynamic string with an initial space. Then add
   further spaces to suit the current indentation level, but reduced
   by one to allow the "IsA" item to match the "Begin" and "End" items
   which enclose it. */
      line = AppendString( NULL, &nc, " " );
      for ( i = 0; i < ( current_indent - INDENT_INC ); i++ ) {
         line = AppendString( line, &nc, " " );
      }

/* Append the "IsA" keyword followed by the class name. */
      line = AppendString( line, &nc, "IsA " );
      line = AppendString( line, &nc, class );

/* If required, also append the comment. */
      if ( astGetComment( this ) && *comment ) {
         line = AppendString( line, &nc, " \t# " );
         line = AppendString( line, &nc, comment );
      }

/* Write out the resulting line of text. */
      OutputTextItem( this, line );

/* Free the dynamic string. */
      line = astFree( line );

/* Clear the count of items written for this class. */
      items_written = 0;
   }
}

static void WriteObject( AstChannel *this, const char *name,
                         int set, int helpful,
                         AstObject *value, const char *comment ) {
/*
*+
*  Name:
*     astWriteObject

*  Purpose:
*     Write an Object as a value to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astWriteObject( AstChannel *this, const char *name,
*                          int set, int helpful,
*                          AstObject *value, const char *comment )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes an Object as a named value, representing
*     the value of a class instance variable, to the data sink
*     associated with a Channel. It is intended for use by class
*     "Dump" functions when writing out class information which will
*     subsequently be re-read.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated string containing the
*        name to be used to identify the value in the external
*        representation. This will form the key for identifying it
*        again when it is re-read. The name supplied should be unique
*        within its class.
*
*        Mixed case may be used and will be preserved in the external
*        representation (where possible) for cosmetic effect. However,
*        case is not significant when re-reading values.
*
*        It is recommended that a maximum of 6 alphanumeric characters
*        (starting with an alphabetic character) be used. This permits
*        maximum flexibility in adapting to standard external data
*        representations (e.g. FITS).
*     set
*        If this is zero, it indicates that the value being written is
*        a default value (or can be re-generated from other values) so
*        need not necessarily be written out. Such values will
*        typically be included in the external representation with
*        (e.g.) a comment character so that they are available to
*        human readers but will be ignored when re-read. They may also
*        be completely omitted in some circumstances.
*
*        If "set" is non-zero, the value will always be explicitly
*        included in the external representation so that it can be
*        re-read.
*     helpful
*        This flag provides a hint about whether a value whose "set"
*        flag is zero (above) should actually appear at all in the
*        external representaton.
*
*        If the external representation allows values to be "commented
*        out" then, by default, values will be included in this form
*        only if their "helpful" flag is non-zero. Otherwise, they
*        will be omitted entirely. When possible, omitting the more
*        obscure values associated with a class is recommended in
*        order to improve readability.
*
*        This default behaviour may be further modified if the
*        Channel's Full attribute is set - either to permit all values
*        to be shown, or to suppress non-essential information
*        entirely.
*     value
*        A Pointer to the Object to be written.
*     comment
*        Pointer to a constant null-terminated string containing a
*        textual comment to be associated with the value.
*
*        Note that this comment may not actually be used, depending on
*        the nature of the Channel supplied and the setting of its
*        Comment attribute.
*-
*/

/* Local Variables: */
   char *line;                   /* Pointer to dynamic output string */
   int i;                        /* Loop counter for indentation characters */
   int nc;                       /* Number of output characters */

/* Check the global error status. */
   if ( !astOK ) return;

/* Use the "set" and "helpful" flags, along with the Channel's
   attributes to decide whether this value should actually be
   written. */
   if ( Use( this, set, helpful ) ) {

/* Start building a dynamic string with an initial space, or a comment
   character if "set" is zero. Then add further spaces to suit the
   current indentation level. */
      line = AppendString( NULL, &nc, set ? " " : "#" );
      for ( i = 0; i < current_indent; i++ ) {
         line = AppendString( line, &nc, " " );
      }

/* Append the name string followed by " =". The absence of a value on
   the right hand side indicates an Object value, whose definition
   follows. */
      line = AppendString( line, &nc, name );
      line = AppendString( line, &nc, " =" );

/* If required, also append the comment. */
      if ( astGetComment( this ) && *comment ) {
         line = AppendString( line, &nc, " \t# " );
         line = AppendString( line, &nc, comment );
      }

/* Write out the resulting line of text. */
      OutputTextItem( this, line );

/* Free the dynamic string. */
      line = astFree( line );

/* If the value is not a default, write the Object to the Channel as
   well, suitably indented (this is omitted if the value is commented
   out). */
      if ( set ) {
         current_indent += INDENT_INC;
         (void) astWrite( this, value );
         current_indent -= INDENT_INC;
      }
   }
}

static void WriteString( AstChannel *this, const char *name,
                         int set, int helpful,
                         const char *value, const char *comment ) {
/*
*+
*  Name:
*     astWriteString

*  Purpose:
*     Write a string value to a data sink.

*  Type:
*     Protected virtual function.

*  Synopsis:
*     #include "channel.h"
*     void astWriteString( AstChannel *this, const char *name,
*                          int set, int helpful,
*                          const char *value, const char *comment )

*  Class Membership:
*     Channel method.

*  Description:
*     This function writes a named string value, representing the
*     value of a class instance variable, to the data sink associated
*     with a Channel. It is intended for use by class "Dump" functions
*     when writing out class information which will subsequently be
*     re-read.

*  Parameters:
*     this
*        Pointer to the Channel.
*     name
*        Pointer to a constant null-terminated string containing the
*        name to be used to identify the value in the external
*        representation. This will form the key for identifying it
*        again when it is re-read. The name supplied should be unique
*        within its class.
*
*        Mixed case may be used and will be preserved in the external
*        representation (where possible) for cosmetic effect. However,
*        case is not significant when re-reading values.
*
*        It is recommended that a maximum of 6 alphanumeric characters
*        (starting with an alphabetic character) be used. This permits
*        maximum flexibility in adapting to standard external data
*        representations (e.g. FITS).
*     set
*        If this is zero, it indicates that the value being written is
*        a default value (or can be re-generated from other values) so
*        need not necessarily be written out. Such values will
*        typically be included in the external representation with
*        (e.g.) a comment character so that they are available to
*        human readers but will be ignored when re-read. They may also
*        be completely omitted in some circumstances.
*
*        If "set" is non-zero, the value will always be explicitly
*        included in the external representation so that it can be
*        re-read.
*     helpful
*        This flag provides a hint about whether a value whose "set"
*        flag is zero (above) should actually appear at all in the
*        external representaton.
*
*        If the external representation allows values to be "commented
*        out" then, by default, values will be included in this form
*        only if their "helpful" flag is non-zero. Otherwise, they
*        will be omitted entirely. When possible, omitting the more
*        obscure values associated with a class is recommended in
*        order to improve readability.
*
*        This default behaviour may be further modified if the
*        Channel's Full attribute is set - either to permit all values
*        to be shown, or to suppress non-essential information
*        entirely.
*     value
*        Pointer to a constant null-terminated string containing the
*        value to be written.
*     comment
*        Pointer to a constant null-terminated string containing a
*        textual comment to be associated with the value.
*
*        Note that this comment may not actually be used, depending on
*        the nature of the Channel supplied and the setting of its
*        Comment attribute.
*-
*/

/* Local Variables: */
   char *line;                   /* Pointer to dynamic output string */
   int i;                        /* Loop counter for characters */
   int nc;                       /* Number of output characters */
   int quote;                    /* Quote character found? */
   int size;                     /* Size of allocated memory */

/* Check the global error status. */
   if ( !astOK ) return;

/* Use the "set" and "helpful" flags, along with the Channel's
   attributes to decide whether this value should actually be
   written. */
   if ( Use( this, set, helpful ) ) {

/* Start building a dynamic string with an initial space, or a comment
   character if "set" is zero. Then add further spaces to suit the
   current indentation level. */
      line = AppendString( NULL, &nc, set ? " " : "#" );
      for ( i = 0; i < current_indent; i++ ) {
         line = AppendString( line, &nc, " " );
      }

/* Append the name string followed by " = " and an opening quote
   character (the string will be quoted to protect leading and
   trailing spaces). */
      line = AppendString( line, &nc, name );
      line = AppendString( line, &nc, " = \"" );

/* We now append the value string, but must inspect each character so
   that quotes (appearing inside quotes) can be doubled. Determine the
   current size of memory allocated for the dynamic string. */
      size = (int) astSizeOf( line );

/* Loop to inspect each character and see if it is a quote. */
      for ( i = 0; value[ i ]; i++ ) {
         quote = ( value[ i ] == '"' );

/* If more memory is required, extend the dynamic string (allowing for
   doubling of quotes and the final null) and save its new size. */
         if ( nc + 2 + quote > size ) {
            line = astGrow( line, nc + 2 + quote, sizeof( char ) );
            if ( astOK ) {
               size = (int) astSizeOf( line );

/* Quit if an error occurs. */
            } else {
               break;
            }
         }

/* Append the value character to the dynamic string, duplicating each
   quote character. */
         line[ nc++ ] = value[ i ];
         if ( quote ) line[ nc++ ] = '"';
      }

/* Append the closing quote. */
      line = AppendString( line, &nc, "\"" );

/* If required, also append the comment. */
      if ( astGetComment( this ) && *comment ) {
         line = AppendString( line, &nc, " \t# " );
         line = AppendString( line, &nc, comment );
      }

/* Write out the resulting line of text. */
      OutputTextItem( this, line );

/* Free the dynamic string. */
      line = astFree( line );
   }
}

/* Functions which access class attributes. */
/* ---------------------------------------- */
/* Implement member functions to access the attributes associated with
   this class using the macros defined for this purpose in the
   "object.h" file. */

/*
*att++
*  Name:
*     Comment

*  Purpose:
*     Include textual comments in output?

*  Type:
*     Public attribute.

*  Synopsis:
*     Integer (boolean).

*  Description:
*     This is a boolean attribute which controls whether textual
*     comments are to be included in the output generated by a
*     Channel. If included, they will describe what each item of
*     output represents.
*
*     If Comment is non-zero (the default), then comments will be
*     included. If it is zero, comments will be omitted.

*  Applicability:
*     Channel
*        All Channels have this attribute.
*att--
*/

/* This is a boolean value (0 or 1) with a value of -INT_MAX when
   undefined but yielding a default of one. */
astMAKE_CLEAR(Channel,Comment,comment,-INT_MAX)
astMAKE_GET(Channel,Comment,int,0,( this->comment != -INT_MAX ? this->comment : 1 ))
astMAKE_SET(Channel,Comment,int,comment,( value != 0 ))
astMAKE_TEST(Channel,Comment,( this->comment != -INT_MAX ))

/*
*att++
*  Name:
*     Full

*  Purpose:
*     Set level of output detail.

*  Type:
*     Public attribute.

*  Synopsis:
*     Integer.

*  Description:
*     This attribute is a three-state flag and takes values of -1, 0
*     or +1.  It controls the amount of information included in the
*     output generated by a Channel.
*
*     If Full is zero (the default), then a modest amount of
*     non-essential but useful information will be included in the
*     output. If Full is negative, all non-essential information will
*     be suppressed to minimise the amount of output, while if it is
*     positive, the output will include the maximum amount of detailed
*     information about the Object being written.

*  Applicability:
*     Channel
*        All Channels have this attribute.

*  Notes:
*     - All positive values supplied for this attribute are converted
*     to +1 and all negative values are converted to -1.
*att--
*/

/* This ia a 3-state value (-1, 0 or +1) with a value of -INT_MAX when
   undefined but yielding a default of zero. */
astMAKE_CLEAR(Channel,Full,full,-INT_MAX)
astMAKE_GET(Channel,Full,int,0,( this->full != -INT_MAX ? this->full : 0 ))
astMAKE_SET(Channel,Full,int,full,( value > 0 ? 1 : ( value < 0 ? -1 : 0 ) ))
astMAKE_TEST(Channel,Full,( this->full != -INT_MAX ))

/*
*att++
*  Name:
*     Skip

*  Purpose:
*     Skip irrelevant data?

*  Type:
*     Public attribute.

*  Synopsis:
*     Integer (boolean).

*  Description:
*     This is a boolean attribute which indicates whether the Object
*     data being read through a Channel are inter-mixed with other,
*     irrelevant, external data.
*
*     If Skip is zero (the default), then the source of input data is
*     expected to contain descriptions of AST Objects and comments and
*     nothing else (if anything else is read, an error will
*     result). If Skip is non-zero, then any non-Object data
*     encountered between Objects will be ignored and simply skipped
*     over in order to reach the next Object.

*  Applicability:
*     Channel
*        All Channels have this attribute.
*     FitsChan
*        The FitsChan class sets the default value of this attribute
*        to 1, so that all irrelevant FITS headers will normally be
*        ignored.
*att--
*/

/* This ia a boolean value (0 or 1) with a value of -INT_MAX when
   undefined but yielding a default of zero. */
astMAKE_CLEAR(Channel,Skip,skip,-INT_MAX)
astMAKE_GET(Channel,Skip,int,0,( this->skip != -INT_MAX ? this->skip : 0 ))
astMAKE_SET(Channel,Skip,int,skip,( value != 0 ))
astMAKE_TEST(Channel,Skip,( this->skip != -INT_MAX ))

/* Destructor. */
/* ----------- */
/* None. */

/* Copy constructor. */
/* ----------------- */
/* None. */

/* Dump function. */
/* -------------- */
static void Dump( AstObject *this_object, AstChannel *channel ) {
/*
*  Name:
*     Dump

*  Purpose:
*     Dump function for Channel objects.

*  Type:
*     Private function.

*  Synopsis:
*     void Dump( AstObject *this, AstChannel *channel )

*  Description:
*     This function implements the Dump function which writes out data
*     for the Channel class to an output Channel.

*  Parameters:
*     this
*        Pointer to the Object whose data are being written.
*     channel
*        Pointer to the Channel to which the data are being written.
*/

/* Local Variables: */
   AstChannel *this;             /* Pointer to the Channel structure */
   char *comment;                /* Pointer to comment string */
   int ival;                     /* Integer value */
   int set;                      /* Attribute value set? */

/* Check the global error status. */
   if ( !astOK ) return;

/* Obtain a pointer to the Channel structure. */
   this = (AstChannel *) this_object;

/* Write out values representing the instance variables for the
   Channel class.  Accompany these with appropriate comment strings,
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

/* Skip. */
/* ----- */
   set = TestSkip( this );
   ival = set ? GetSkip( this ) : astGetSkip( this );
   astWriteInt( channel, "Skip", set, 0, ival,
                ival ? "Ignore data between Objects" :
                       "No data allowed between Objects" );

/* Full. */
/* ----- */
   set = TestFull( this );
   ival = set ? GetFull( this ) : astGetFull( this );
   if ( ival < 0 ) {
      comment = "Suppress non-essential output";
   }else if ( ival == 0 ) {
      comment = "Output standard information";
   } else {
      comment = "Output maximum information";
   }
   astWriteInt( channel, "Full", set, 0, ival, comment );

/* Comment. */
/* -------- */
   set = TestComment( this );
   ival = set ? GetComment( this ) : astGetComment( this );
   astWriteInt( channel, "Comm", set, 0, ival,
                ival ? "Display comments" :
                       "Omit comments" );
}

/* Standard class functions. */
/* ========================= */
/* Implement the astIsAChannel and astCheckChannel functions using the
   macros defined for this purpose in the "object.h" header file. */
astMAKE_ISA(Channel,Object,check,&class_init)
astMAKE_CHECK(Channel)

AstChannel *astChannel_( const char *(* source)( void ),
                         void (* sink)( const char * ),
                         const char *options, ... ) {
/*
*+
*  Name:
*     astChannel

*  Purpose:
*     Create a Channel.

*  Type:
*     Protected function.

*  Synopsis:
*     #include "channel.h"
*     AstChannel *astChannel( const char *(* source)( void ),
*                             void (* sink)( const char * ),
*                             const char *options, ... )

*  Class Membership:
*     Channel constructor.

*  Description:
*     This function creates a new Channel and optionally initialises
*     its attributes.
*
*     A Channel implements low-level input/output for the AST library.
*     Writing an Object to a Channel (using astWrite) will generate a
*     textual representation of that Object, and reading from a
*     Channel (using astRead) will create a new Object from its
*     textual representation.
*
*     Normally, when you use a Channel, you should provide "source"
*     and "sink" functions which connect it to an external data store
*     by reading and writing the resulting text. By default, however,
*     a Channel will read from standard input and write to standard
*     output.

*  Parameters:
*     source
*        Pointer to a "source" function that takes no arguments and
*        returns a pointer to a null-terminated string.
*
*        This function will be used by the Channel to obtain lines of
*        input text. On each invocation, it should return a pointer to
*        the next input line read from some external data store, and a
*        NULL pointer when there are no more lines to read.
*
*        If "source" is NULL, the Channel will read from standard
*        input instead.
*     sink
*        Pointer to a "sink" function that takes a pointer to a
*        null-terminated string as an argument and returns void.
*
*        This function will be used by the Channel to deliver lines of
*        output text. On each invocation, it should deliver the
*        contents of the string supplied to some external data store.
*
*        If "sink" is NULL, the Channel will write to standard output
*        instead.
*     options
*        Pointer to a null-terminated string containing an optional
*        comma-separated list of attribute assignments to be used for
*        initialising the new Channel. The syntax used is identical to
*        that for the astSet function and may include "printf" format
*        specifiers identified by "%" symbols in the normal way.
*     ...
*        If the "options" string contains "%" format specifiers, then
*        an optional list of additional arguments may follow it in
*        order to supply values to be substituted for these
*        specifiers. The rules for supplying these are identical to
*        those for the astSet function (and for the C "printf"
*        function).

*  Returned Value:
*     astChannel()
*        A pointer to the new Channel.

*  Notes:
*     - A NULL pointer value will be returned if this function is
*     invoked with the global error status set, or if it should fail
*     for any reason.
*-

*  Implementation Notes:
*     - This function implements the basic Channel constructor which
*     is available via the protected interface to the Channel class.
*     A public interface is provided by the astChannelId_ function.
*/

/* Local Variables: */
   AstChannel *new;              /* Pointer to new Channel */
   va_list args;                 /* Variable argument list */

/* Check the global status. */
   if ( !astOK ) return NULL;

/* Initialise the Channel, allocating memory and initialising the
   virtual function table as well if necessary. Supply pointers to
   (local) wrapper functions that can invoke the source and sink
   functions with appropriate arguments for the C language. */
   new = Init( NULL, sizeof( AstChannel ), !class_init, &class_vtab,
               "Channel", source, SourceWrap, sink, SinkWrap );

/* If successful, note that the virtual function table has been
   initialised. */
   if ( astOK ) {
      class_init = 1;

/* Obtain the variable argument list and pass it along with the
   options string to the astVSet method to initialise the new
   Channel's attributes. */
      va_start( args, options );
      astVSet( new, options, args );
      va_end( args );

/* If an error occurred, clean up by deleting the new object. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return a pointer to the new Channel. */
   return new;
}

AstChannel *astChannelId_( const char *(* source)( void ),
                           void (* sink)( const char * ),
                           const char *options, ... ) {
/*
*++
*  Name:
c     astChannel
f     AST_CHANNEL

*  Purpose:
*     Create a Channel.

*  Type:
*     Public function.

*  Synopsis:
c     #include "channel.h"
c     AstChannel *astChannel( const char *(* source)( void ),
c                             void (* sink)( const char * ),
c                             const char *options, ... )
f     RESULT = AST_CHANNEL( SOURCE, SINK, OPTIONS, STATUS )

*  Class Membership:
*     Channel constructor.

*  Description:
*     This function creates a new Channel and optionally initialises
*     its attributes.
*
*     A Channel implements low-level input/output for the AST library.
c     Writing an Object to a Channel (using astWrite) will generate a
f     Writing an Object to a Channel (using AST_WRITE) will generate a
*     textual representation of that Object, and reading from a
c     Channel (using astRead) will create a new Object from its
f     Channel (using AST_READ) will create a new Object from its
*     textual representation.
*
*     Normally, when you use a Channel, you should provide "source"
c     and "sink" functions which connect it to an external data store
f     and "sink" routines which connect it to an external data store
*     by reading and writing the resulting text. By default, however,
*     a Channel will read from standard input and write to standard
*     output.

*  Parameters:
c     source
f     SOURCE = SUBROUTINE (Given)
c        Pointer to a source function that takes no arguments and
c        returns a pointer to a null-terminated string.  This function
c        will be used by the Channel to obtain lines of input text. On
c        each invocation, it should return a pointer to the next input
c        line read from some external data store, and a NULL pointer
c        when there are no more lines to read.
c
c        If "source" is NULL, the Channel will read from standard
c        input instead.
f        A source routine, which is a subroutine which takes a single
f        integer error status argument.  This routine will be used by
f        the Channel to obtain lines of input text. On each
f        invocation, it should read the next input line from some
f        external data store, and then return the resulting text to
f        the AST library by calling AST_PUTLINE. It should supply a
f        negative line length when there are no more lines to read.
f        If an error occurs, it should set its own error status
f        argument to an error value before returning.
f
f        If the null routine AST_NULL is suppied as the SOURCE value,
f        the Channel will read from standard input instead.
c     sink
f     SINK = SUBROUTINE (Given)
c        Pointer to a sink function that takes a pointer to a
c        null-terminated string as an argument and returns void.  This
c        function will be used by the Channel to deliver lines of
c        output text. On each invocation, it should deliver the
c        contents of the string supplied to some external data store.
c
c        If "sink" is NULL, the Channel will write to standard output
c        instead.
f        A sink routine, which is a subroutine which takes a single
f        integer error status argument.  This routine will be used by
f        the Channel to deliver lines of output text. On each
f        invocation, it should obtain the next output line from the
f        AST library by calling AST_GETLINE, and then deliver the
f        resulting text to some external data store.  If an error
f        occurs, it should set its own error status argument to an
f        error value before returning.
f
f        If the null routine AST_NULL is suppied as the SINK value,
f        the Channel will write to standard output instead.
c     options
f     OPTIONS = CHARACTER * ( * ) (Given)
c        Pointer to a null-terminated string containing an optional
c        comma-separated list of attribute assignments to be used for
c        initialising the new Channel. The syntax used is identical to
c        that for the astSet function and may include "printf" format
c        specifiers identified by "%" symbols in the normal way.
f        A character string containing an optional comma-separated
f        list of attribute assignments to be used for initialising the
f        new Channel. The syntax used is identical to that for the
f        AST_SET routine.
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
c     astChannel()
f     AST_CHANNEL = INTEGER
*        A pointer to the new Channel.

*  Notes:
f     - The names of the routines supplied for the SOURCE and SINK
f     arguments should appear in EXTERNAL statements in the Fortran
f     routine which invokes AST_CHANNEL. However, this is not generally
f     necessary for the null routine AST_NULL (so long as the AST_PAR
f     include file has been used).
*     - A null Object pointer (AST__NULL) will be returned if this
c     function is invoked with the AST error status set, or if it
f     function is invoked with STATUS set to an error value, or if it
*     should fail for any reason.
f     - Note that the null routine AST_NULL (one underscore) is
f     different to AST__NULL (two underscores), which is the null Object
f     pointer.
*--

*  Implementation Notes:
*     - This function implements the external (public) interface to
*     the astChannel constructor function. It returns an ID value
*     (instead of a true C pointer) to external users, and must be
*     provided because astChannel_ has a variable argument list which
*     cannot be encapsulated in a macro (where this conversion would
*     otherwise occur).
*     - The variable argument list also prevents this function from
*     invoking astChanel_ directly, so it must be a re-implementation
*     of it in all respects, except for the final conversion of the
*     result to an ID value.
*/

/* Local Variables: */
   AstChannel *new;              /* Pointer to new Channel */
   va_list args;                 /* Variable argument list */

/* Check the global status. */
   if ( !astOK ) return NULL;

/* Initialise the Channel, allocating memory and initialising the
   virtual function table as well if necessary. Supply pointers to
   (local) wrapper functions that can invoke the source and sink
   functions with appropriate arguments for the C language. */
   new = Init( NULL, sizeof( AstChannel ), !class_init, &class_vtab,
               "Channel", source, SourceWrap, sink, SinkWrap );

/* If successful, note that the virtual function table has been
   initialised. */
   if ( astOK ) {
      class_init = 1;

/* Obtain the variable argument list and pass it along with the
   options string to the astVSet method to initialise the new
   Channel's attributes. */
      va_start( args, options );
      astVSet( new, options, args );
      va_end( args );

/* If an error occurred, clean up by deleting the new object. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return an ID value for the new Channel. */
   return astMakeId( new );
}

AstChannel *astChannelForId_( const char *(* source)( void ),
                              char *(* source_wrap)( const char *(*)( void ) ),
                              void (* sink)( const char * ),
                              void (* sink_wrap)( void (*)( const char * ),
                                                  const char * ),
                              const char *options, ... ) {
/*
*+
*  Name:
*     astChannelFor

*  Purpose:
*     Initialise a Channel from a foreign language interface.

*  Type:
*     Public function.

*  Synopsis:
*     #include "channel.h"
*     AstChannel *astChannelFor( const char *(* source)( void ),
*                                char *(* source_wrap)( const char *(*)
*                                                       ( void ) ),
*                                void (* sink)( const char * ),
*                                void (* sink_wrap)( void (*)( const char * ),
*                                                    const char * ),
*                                const char *options, ... )

*  Class Membership:
*     Channel constructor.

*  Description:
*     This function creates a new Channel from a foreign language
*     interface and optionally initialises its attributes.
*
*     A Channel implements low-level input/output for the AST library.
*     Writing an Object to a Channel (using astWrite) will generate a
*     textual representation of that Object, and reading from a
*     Channel (using astRead) will create a new Object from its
*     textual representation.
*
*     Normally, when you use a Channel, you should provide "source"
*     and "sink" functions which connect it to an external data store
*     by reading and writing the resulting text. This function also
*     requires you to provide "wrapper" functions which will invoke
*     the source and sink functions. By default, however, a Channel
*     will read from standard input and write to standard output.

*  Parameters:
*     source
*        Pointer to a "source" function which will be used to obtain
*        lines of input text. Generally, this will be obtained by
*        casting a pointer to a source function which is compatible
*        with the "source_wrap" wrapper function (below). The pointer
*        should later be cast back to its original type by the
*        "source_wrap" function before the function is invoked.
*
*        If "source" is NULL, the Channel will read from standard
*        input instead.
*     source_wrap
*        Pointer to a function which can be used to invoke the
*        "source" function supplied (above). This wrapper function is
*        necessary in order to hide variations in the nature of the
*        source function, such as may arise when it is supplied by a
*        foreign (non-C) language interface.
*
*        The single parameter of the "source_wrap" function is a
*        pointer to the "source" function, and it should cast this
*        function pointer (as necessary) and invoke the function with
*        appropriate arguments to obtain the next line of input
*        text. The "source_wrap" function should then return a pointer
*        to a dynamically allocated, null terminated string containing
*        the text that was read. The string will be freed (using
*        astFree) when no longer required and the "source_wrap"
*        function need not concern itself with this. A NULL pointer
*        should be returned if there is no more input to read.
*
*        If "source_wrap" is NULL, the Channel will read from standard
*        input instead.
*     sink
*        Pointer to a "sink" function which will be used to deliver
*        lines of output text. Generally, this will be obtained by
*        casting a pointer to a sink function which is compatible with
*        the "sink_wrap" wrapper function (below). The pointer should
*        later be cast back to its original type by the "sink_wrap"
*        function before the function is invoked.
*
*        If "sink" is NULL, the Channel will write to standard output
*        instead.
*     sink_wrap
*        Pointer to a function which can be used to invoke the "sink"
*        function supplied (above). This wrapper function is necessary
*        in order to hide variations in the nature of the sink
*        function, such as may arise when it is supplied by a foreign
*        (non-C) language interface.
*
*        The first parameter of the "sink_wrap" function is a pointer
*        to the "sink" function, and the second parameter is a pointer
*        to a const, null-terminated character string containing the
*        text to be written.  The "sink_wrap" function should cast the
*        "sink" function pointer (as necessary) and invoke the
*        function with appropriate arguments to deliver the line of
*        output text. The "sink_wrap" function then returns void.
*
*        If "sink_wrap" is NULL, the Channel will write to standard
*        output instead.
*     options
*        Pointer to a null-terminated string containing an optional
*        comma-separated list of attribute assignments to be used for
*        initialising the new Channel. The syntax used is identical to
*        that for the astSet function and may include "printf" format
*        specifiers identified by "%" symbols in the normal way.
*     ...
*        If the "options" string contains "%" format specifiers, then
*        an optional list of additional arguments may follow it in
*        order to supply values to be substituted for these
*        specifiers. The rules for supplying these are identical to
*        those for the astSet function (and for the C "printf"
*        function).

*  Returned Value:
*     astChannelFor()
*        A pointer to the new Channel.

*  Notes:
*     - A null Object pointer (AST__NULL) will be returned if this
*     function is invoked with the global error status set, or if it
*     should fail for any reason.
*     - This function is only available through the public interface
*     to the Channel class (not the protected interface) and is
*     intended solely for use in implementing foreign language
*     interfaces to this class.
*-

*  Implememtation Notes:
*     - This function behaves exactly like astChannelId_, in that it
*     returns ID values and not true C pointers, but it has two
*     additional arguments. These are pointers to the "wrapper
*     functions" which are needed to accommodate foreign language
*     interfaces.
*/

/* Local Variables: */
   AstChannel *new;              /* Pointer to new Channel */
   va_list args;                 /* Variable argument list */

/* Check the global status. */
   if ( !astOK ) return NULL;

/* Initialise the Channel, allocating memory and initialising the
   virtual function table as well if necessary. */
   new = Init( NULL, sizeof( AstChannel ), !class_init, &class_vtab,
               "Channel", source, source_wrap, sink, sink_wrap );

/* If successful, note that the virtual function table has been
   initialised. */
   if ( astOK ) {
      class_init = 1;

/* Obtain the variable argument list and pass it along with the
   options string to the astVSet method to initialise the new
   Channel's attributes. */
      va_start( args, options );
      astVSet( new, options, args );
      va_end( args );

/* If an error occurred, clean up by deleting the new object. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return an ID value for the new Channel. */
   return astMakeId( new );
}

AstChannel *astInitChannel_( void *mem, size_t size, int init,
                             AstChannelVtab *vtab, const char *name,
                             const char *(* source)( void ),
                             void (* sink)( const char * ) ) {
/*
*+
*  Name:
*     astInitChannel

*  Purpose:
*     Initialise a Channel.

*  Type:
*     Protected function.

*  Synopsis:
*     #include "channel.h"
*     AstChannel *astInitChannel( void *mem, size_t size, int init,
*                                 AstChannelVtab *vtab, const char *name,
*                                 const char *(* source)( void ),
*                                 void (* sink)( const char * ) )

*  Class Membership:
*     Channel initialiser.

*  Description:
*     This function is provided for use by class implementations to
*     initialise a new Channel object. It allocates memory (if
*     necessary) to accommodate the Channel plus any additional data
*     associated with the derived class.  It then initialises a
*     Channel structure at the start of this memory. If the "init"
*     flag is set, it also initialises the contents of a virtual
*     function table for a Channel at the start of the memory passed
*     via the "vtab" parameter.

*  Parameters:
*     mem
*        A pointer to the memory in which the Channel is to be
*        initialised.  This must be of sufficient size to accommodate
*        the Channel data (sizeof(Channel)) plus any data used by the
*        derived class. If a value of NULL is given, this function
*        will allocate the memory itself using the "size" parameter to
*        determine its size.
*     size
*        The amount of memory used by the Channel (plus derived class
*        data).  This will be used to allocate memory if a value of
*        NULL is given for the "mem" parameter. This value is also
*        stored in the Channel structure, so a valid value must be
*        supplied even if not required for allocating memory.
*     init
*        A boolean flag indicating if the Channel's virtual function
*        table is to be initialised. If this value is non-zero, the
*        virtual function table will be initialised by this function.
*     vtab
*        Pointer to the start of the virtual function table to be
*        associated with the new Channel.
*     name
*        Pointer to a constant null-terminated character string which
*        contains the name of the class to which the new object
*        belongs (it is this pointer value that will subsequently be
*        returned by the astGetClass method).
*     source
*        Pointer to a "source" function that takes no arguments and
*        returns a pointer to a null-terminated string.
*
*        This function will be used by the Channel to obtain lines of
*        input text. On each invocation, it should return a pointer to
*        the next input line read from some external data store, and a
*        NULL pointer when there are no more lines to read.
*
*        If "source" is NULL, the Channel will read from standard
*        input instead.
*     sink
*        Pointer to a "sink" function that takes a pointer to a
*        null-terminated string as an argument and returns void.
*
*        This function will be used by the Channel to deliver lines of
*        output text. On each invocation, it should deliver the
*        contents of the string supplied to some external data store.
*
*        If "sink" is NULL, the Channel will write to standard output
*        instead.

*  Returned Value:
*     A pointer to the new Channel.

*  Notes:
*     - A null pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-
*/

/* Local Variables: */
   AstChannel *new;              /* Pointer to new Channel */

/* Initialise. */
   new = NULL;

/* Check the global status. */
   if ( !astOK ) return new;

/* Initialise the new Channel, supplying pointers to appropriate local
   source and sink wrapper functions. */
   new = Init( mem, size, init, vtab, name,
               source, SourceWrap, sink, SinkWrap );

/* Return a pointer to the new Channel. */
   return new;
}

AstChannel *astLoadChannel_( void *mem, size_t size, int init,
                             AstChannelVtab *vtab, const char *name,
                             AstChannel *channel ) {
/*
*+
*  Name:
*     astLoadChannel

*  Purpose:
*     Load a Channel.

*  Type:
*     Protected function.

*  Synopsis:
*     #include "channel.h"
*     AstChannel *astLoadChannel( void *mem, size_t size, int init,
*                                 AstChannelVtab *vtab, const char *name,
*                                 AstChannel *channel )

*  Class Membership:
*     Channel loader.

*  Description:
*     This function is provided to load a new Channel using data read
*     from a Channel. It first loads the data used by the parent class
*     (which allocates memory if necessary) and then initialises a
*     Channel structure in this memory, using data read from the input
*     Channel.
*
*     If the "init" flag is set, it also initialises the contents of a
*     virtual function table for a Channel at the start of the memory
*     passed via the "vtab" parameter.

*  Parameters:
*     mem
*        A pointer to the memory into which the Channel is to be
*        loaded.  This must be of sufficient size to accommodate the
*        Channel data (sizeof(Channel)) plus any data used by derived
*        classes. If a value of NULL is given, this function will
*        allocate the memory itself using the "size" parameter to
*        determine its size.
*     size
*        The amount of memory used by the Channel (plus derived class
*        data).  This will be used to allocate memory if a value of
*        NULL is given for the "mem" parameter. This value is also
*        stored in the Channel structure, so a valid value must be
*        supplied even if not required for allocating memory.
*
*        If the "vtab" parameter is NULL, the "size" value is ignored
*        and sizeof(AstChannel) is used instead.
*     init
*        A boolean flag indicating if the Channel's virtual function
*        table is to be initialised. If this value is non-zero, the
*        virtual function table will be initialised by this function.
*
*        If the "vtab" parameter is NULL, the "init" value is ignored
*        and the (static) virtual function table initialisation flag
*        for the Channel class is used instead.
*     vtab
*        Pointer to the start of the virtual function table to be
*        associated with the new Channel. If this is NULL, a pointer
*        to the (static) virtual function table for the Channel class
*        is used instead.
*     name
*        Pointer to a constant null-terminated character string which
*        contains the name of the class to which the new object
*        belongs (it is this pointer value that will subsequently be
*        returned by the astGetClass method).
*
*        If the "vtab" parameter is NULL, the "name" value is ignored
*        and a pointer to the string "Channel" is used instead.

*  Returned Value:
*     A pointer to the new Channel.

*  Notes:
*     - A null pointer will be returned if this function is invoked
*     with the global error status set, or if it should fail for any
*     reason.
*-
*/

/* Local Variables: */
   AstChannel *new;              /* Pointer to the new Channel */

/* Initialise. */
   new = NULL;

/* Check the global error status. */
   if ( !astOK ) return new;

/* If a NULL virtual function table has been supplied, then this is
   the first loader to be invoked for this Channel. In this case the
   Channel belongs to this class, so supply appropriate values to be
   passed to the parent class loader (and its parent, etc.). */
   if ( !vtab ) {
      size = sizeof( AstChannel );
      init = !class_init;
      vtab = &class_vtab;
      name = "Channel";
   }

/* Invoke the parent class loader to load data for all the ancestral
   classes of the current one, returning a pointer to the resulting
   partly-built Channel. */
   new = astLoadObject( mem, size, init, (AstObjectVtab *) vtab, name,
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
      astReadClassData( channel, "Channel" );

/* Set the pointers to the source and sink functions, and their
   wrapper functions, to NULL (we cannot restore these since they
   refer to process-specific addresses). */
      new->source = NULL;
      new->source_wrap = NULL;
      new->sink = NULL;
      new->sink_wrap = NULL;

/* Now read each individual data item from this list and use it to
   initialise the appropriate instance variable(s) for this class. */

/* In the case of attributes, we first read the "raw" input value,
   supplying the "unset" value as the default. If a "set" value is
   obtained, we then use the appropriate (private) Set... member
   function to validate and set the value properly. */

/* Skip. */
/* ----- */
      new->skip = astReadInt( channel, "skip", -INT_MAX );
      if ( TestSkip( new ) ) SetSkip( new, new->skip );

/* Full. */
/* ----- */
      new->full = astReadInt( channel, "full", -INT_MAX );
      if ( TestFull( new ) ) SetFull( new, new->full );

/* Comment. */
/* -------- */
      new->comment = astReadInt( channel, "comm", -INT_MAX );
      if ( TestComment( new ) ) SetComment( new, new->comment );

/* If an error occurred, clean up by deleting the new Channel. */
      if ( !astOK ) new = astDelete( new );
   }

/* Return the new Channel pointer. */
   return new;
}

/* Virtual function interfaces. */
/* ============================ */
/* These provide the external interface to the virtual functions
   defined by this class. Each simply checks the global error status
   and then locates and executes the appropriate member function,
   using the function pointer stored in the object's virtual function
   table (this pointer is located using the astMEMBER macro defined in
   "object.h").

   Note that the member function may not be the one defined here, as
   it may have been over-ridden by a derived class. However, it should
   still have the same interface. */
void astGetNextData_( AstChannel *this, int begin, char **name, char **val ) {
   *name = NULL;
   *val = NULL;
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,GetNextData))( this, begin, name, val );
}
char *astGetNextText_( AstChannel *this ) {
   if ( !astOK ) return NULL;
   return (**astMEMBER(this,Channel,GetNextText))( this );
}
void astPutNextText_( AstChannel *this, const char *line ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,PutNextText))( this, line );
}
AstObject *astRead_( AstChannel *this ) {
   if ( !astOK ) return NULL;
   return (**astMEMBER(this,Channel,Read))( this );
}
void astReadClassData_( AstChannel *this, const char *class ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,ReadClassData))( this, class );
}
double astReadDouble_( AstChannel *this, const char *name, double def ) {
   if ( !astOK ) return 0.0;
   return (**astMEMBER(this,Channel,ReadDouble))( this, name, def );
}
int astReadInt_( AstChannel *this, const char *name, int def ) {
   if ( !astOK ) return 0;
   return (**astMEMBER(this,Channel,ReadInt))( this, name, def );
}
AstObject *astReadObject_( AstChannel *this, const char *name,
                           AstObject *def ) {
   if ( !astOK ) return NULL;
   return (**astMEMBER(this,Channel,ReadObject))( this, name, def );
}
char *astReadString_( AstChannel *this, const char *name, const char *def ) {
   if ( !astOK ) return NULL;
   return (**astMEMBER(this,Channel,ReadString))( this, name, def );
}
int astWrite_( AstChannel *this, AstObject *object ) {
   if ( !astOK ) return 0;
   return (**astMEMBER(this,Channel,Write))( this, object );
}
void astWriteBegin_( AstChannel *this, const char *class,
                     const char *comment ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,WriteBegin))( this, class, comment );
}
void astWriteDouble_( AstChannel *this, const char *name, int set, int helpful,
                      double value, const char *comment ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,WriteDouble))( this, name, set, helpful, value,
                                            comment );
}
void astWriteEnd_( AstChannel *this, const char *class ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,WriteEnd))( this, class );
}
void astWriteInt_( AstChannel *this, const char *name, int set, int helpful,
                   int value, const char *comment ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,WriteInt))( this, name, set, helpful, value,
                                         comment );
}
void astWriteIsA_( AstChannel *this, const char *class, const char *comment ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,WriteIsA))( this, class, comment );
}
void astWriteObject_( AstChannel *this, const char *name, int set,
                      int helpful, AstObject *value, const char *comment ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,WriteObject))( this, name, set, helpful, value,
                                            comment );
}
void astWriteString_( AstChannel *this, const char *name, int set, int helpful,
                      const char *value, const char *comment ) {
   if ( !astOK ) return;
   (**astMEMBER(this,Channel,WriteString))( this, name, set, helpful, value,
                                            comment );
}
