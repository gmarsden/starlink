/*  This file, swapproc.c, contains general utility routines that are      */
/*  used by other FITSIO routines to swap bytes.                           */

/*  The FITSIO software was written by William Pence at the High Energy    */
/*  Astrophysic Science Archive Research Center (HEASARC) at the NASA      */
/*  Goddard Space Flight Center.  Users shall not, without prior written   */
/*  permission of the U.S. Government,  establish a claim to statutory     */
/*  copyright.  The Government and others acting on its behalf, shall have */
/*  a royalty-free, non-exclusive, irrevocable,  worldwide license for     */
/*  Government purposes to publish, distribute, translate, copy, exhibit,  */
/*  and perform such material.                                             */

#include <stdlib.h>
#include <string.h>
#include "fitsio2.h"
/*--------------------------------------------------------------------------*/
void ffswap2(short *svalues,  /* IO - pointer to shorts to be swapped       */
             long nvals)      /* I  - number of shorts to be swapped        */
/*
  swap the bytes in the input short integers: ( 0 1 -> 1 0 )
*/
{
    register char *cvalues;
    register long ii;

    union u_tag {
        char cvals[2];   /* equivalence an array of 4 bytes with */
        short sval;      /* a short */
    } u;

    cvalues = (char *) svalues;      /* copy the initial pointer value */

    for (ii = 0; ii < nvals;)
    {
        u.sval = svalues[ii++];  /* copy next short to temporary buffer */

        *cvalues++ = u.cvals[1]; /* copy the 2 bytes to output in turn */
        *cvalues++ = u.cvals[0];
    }
    return;
}
/*--------------------------------------------------------------------------*/
void ffswap4(INT32BIT *ivalues,  /* IO - pointer to floats to be swapped    */
                 long nvals)     /* I  - number of floats to be swapped     */
/*
  swap the bytes in the input 4-byte integer: ( 0 1 2 3 -> 3 2 1 0 )
*/
{
    register char *cvalues;
    register long ii;

    union u_tag {
        char cvals[4];      /* equivalence an array of 4 bytes with */
        INT32BIT ival;      /* a float */
    } u;

    cvalues = (char *) ivalues;   /* copy the initial pointer value */

    for (ii = 0; ii < nvals;)
    {
        u.ival = ivalues[ii++];  /* copy next float to buffer */

        *cvalues++ = u.cvals[3]; /* copy the 4 bytes in turn */
        *cvalues++ = u.cvals[2];
        *cvalues++ = u.cvals[1];
        *cvalues++ = u.cvals[0]; 
    }
    return;
}
/*--------------------------------------------------------------------------*/
void ffswap8(double *dvalues,  /* IO - pointer to doubles to be swapped     */
             long nvals)       /* I  - number of doubles to be swapped      */
/*
  swap the bytes in the input doubles: ( 01234567  -> 76543210 )
*/
{
    register char *cvalues;
    register long ii;

    union u_tag {
        char cvals[8];   /* equivalence an array of 8 bytes with */
        double dval;      /* a double */
    } u;

    cvalues = (char *) dvalues;      /* copy the pointer value */

    for (ii = 0; ii < nvals;)
    {
        u.dval = dvalues[ii++];  /* copy next double to buffer */

        *cvalues++ = u.cvals[7]; /* copy the 8 bytes in turn */
        *cvalues++ = u.cvals[6];
        *cvalues++ = u.cvals[5];
        *cvalues++ = u.cvals[4];
        *cvalues++ = u.cvals[3];
        *cvalues++ = u.cvals[2];
        *cvalues++ = u.cvals[1];
        *cvalues++ = u.cvals[0];
    }
    return;
}

