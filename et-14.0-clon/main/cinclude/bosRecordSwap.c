
/* bosRecordSwap.c */
/******************* Sergey: BOS format swap **********************************/
/* swap FPACK record (includes BOS banks (10-word header format)) */
/* called by Event Builder before injecting event into ET system */
/* this function creates big-endian-style FPACK record including
big-endian signature; record will be shipped to big-endian machine
where it must be recognized as locally-made */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/* print FPACK record */

/* from bosio.h */
#define RECHEADLEN 11
#if defined(Linux) || defined(SunOS_i86pc)
/* DEC format (little-endian IEEE) */
#define LOCAL   4
#else
/* IEEE format */
#define LOCAL   1
#endif



/* define some things for byte swapping */

#define LSWAP(x)        ((((x) & 0x000000ff) << 24) | \
                         (((x) & 0x0000ff00) <<  8) | \
                         (((x) & 0x00ff0000) >>  8) | \
                         (((x) & 0xff000000) >> 24))

#define SSWAP(x)        ((((x) & 0x00ff) << 8) | \
                         (((x) & 0xff00) >> 8))


#if defined(Linux) || defined(SunOS_i86pc)
#define B16TAG 0x20363142
#define B32TAG 0x20323342
#else
#define B16TAG 0x42313620
#define B32TAG 0x42333220

/*Sergey: sometimes have 0x42313600 etc !! check both until resolved .. */
#define B16TAG0 0x42313600
#define B32TAG0 0x42333200

#endif


int
BOSrecordSwap(unsigned int *src, unsigned int *dest)
{
  unsigned int lwd;
  unsigned short swd;
  int ii, jj, ix, k;
  int hlen, tlen, blen, dlen, lform, dtype, typ;
  short shd;
  char cd;
  char *cp;
  unsigned short *sp, *spd;
  unsigned int *lp, *lpd;
  unsigned int bankid, banknum, ncol, nrow, lname, lfmt, fmtlen, nwords, bb;
  char *form;
  int nevent;

  /* if 'dest' == NULL, swap 'src' buffer in place */
  if(dest == NULL)
  {
    dest = src;
  }
  /*printf("src=0x%08x dest=0x%08x\n",src,dest);*/

  ii = 0;
  lp = (unsigned int *)&src[ii];
  lpd = (unsigned int *)&dest[ii];

  blen = *(lp+RECHEADLEN-1); /* last word in record header */

  /*
printf("\n\n\nFPACK record length = %d\n",blen);
  */

  /*
printf("3rd: 0x%08x %d\n",lp[2],lp[2]);
lp[2] = 0;
  */

  /* swap record header: RECHEADLEN words */
  for(jj=0; jj<RECHEADLEN; jj++)
  {
	/*
if(jj==5) printf("befor %d 0x%08x\n",*lp,*lp);
	*/
    /* do not swap record name */
    if((jj==3) || (jj==4))
    {
      *lpd++ = *lp;
      lp++;
      continue;
    }

	lwd = LSWAP(*lp);

    /* set endian signature (or automatically ???) */
    if(jj==1)
    {
#if defined(Linux) || defined(SunOS_i86pc)
      lwd = 1*16 + 8;
#else
      lwd = 4*16 + 8;
#endif
      lwd = LSWAP(lwd);
    }

	*lpd++ = lwd;
    lp++;
	/*
if(jj==5) printf("after %d 0x%08x\n",lwd,lwd);
	*/

  }

  ii += RECHEADLEN;



  /*********************************/
  /* loop over all banks in record */

  lp = (unsigned int *)&src[ii];
  lpd = (unsigned int *)&dest[ii];
  bb = 0;

  while(bb < blen)
  {

    hlen = *lp;            /* record header length (words) */
    dlen = *(lp+8);        /* data length (words) */
    lname = *(lp+1);       /* bank name */
    lform = *(lp+9);       /* first word of format string */
    form = (char *)(lp+9); /* remember the position of format description */


#ifdef SunOS_sun4u
	/*Sergey: temporary !!!*/
    if(lform==B16TAG0)
	{
      /*printf("WARN: B16TAG0 correction: befor >%4.4s<, ",form);*/
      lform = *(lp+9) = B16TAG;
      /*printf("after >%4.4s< (bank >%4.4s<) (nev=%d)\n",form,(char *)&lname,nevent);*/
	}
    if(lform==B32TAG0)
	{
	  /*printf("WARN: B32TAG0 correction: befor >%4.4s<, ",form);*/
      lform = *(lp+9) = B32TAG;
      /*printf("after >%4.4s< (bank >%4.4s<)\n",form,(char *)&lname);*/
	}
#endif


/*
printf("\nBANK record length=%d, header length=%d, name >%4.4s",
    dlen,hlen,&lname);
*/
	
/*if(blen==1406)*/
/*
{
  char *ffff = form;
  printf("\nBANK record length=%d, header length=%d, name >%4.4s<, format >",
    dlen,hlen,&lname);
  for(jj=0; jj<(hlen-9); jj++)
  {
    printf("%4.4s",ffff);
    ffff += 4;
  }
  printf("<\n");
}
*/

    /* swap BOS bank header; header length is 'hlen' but we will swap 9 first words
    only, excluding second and third which are bank name; words starting from 10th
    are format description and they should not be swapped as well */

    *lpd++ = LSWAP(*lp); lp++; /* swap first word */
    *lpd++ = *lp++;            /* copy second word (bank name) */
    *lpd++ = *lp++;            /* copy third word (bank name) */
    *lpd++ = LSWAP(*lp); lp++;
    *lpd++ = LSWAP(*lp); lp++;
    *lpd++ = LSWAP(*lp); lp++;
    *lpd++ = LSWAP(*lp); lp++;
    *lpd++ = LSWAP(*lp); lp++;
    *lpd++ = LSWAP(*lp); lp++;
    for(jj=9; jj<hlen; jj++) *lpd++ = *lp++; /* copy format description */
    bb += hlen;


    if(lform==B16TAG) /* short (look for 'B16 ')*/
	{
	  /*printf("B16 >%4.4s< (0x%08x)\n",form,lform);*/
      sp = (unsigned short *)lp;
      spd = (unsigned short *)lpd;
      for(jj=0; jj<dlen*2; jj++) {*spd++ = SSWAP(*sp); sp++;}
      lp += dlen;
      lpd += dlen;
    }
    else if(!strncmp(form,"(A)",3))
    {
      /*printf("format >%3.3s<\n",form);*/
      for(jj=0; jj<dlen; jj++) *lpd++ = *lp++;
    }
    else if(!strncmp(form,"(F,8A)",6))
    {
      /*printf("format >%6.6s< dlen=%d\n",form,dlen);*/
      for(jj=0; jj<dlen; jj+=9)
      {
        /*printf("value=%f\n",*((float *)lp));*/
        {*lpd++ = LSWAP(*lp); lp++;}
        for(k=0; k<8; k++) *lpd++ = *lp++;
	  }
    }
    else if(lform == B32TAG) /* int (look for 'B32 ')*/
    {
      for(jj=0; jj<dlen; jj++) {*lpd++ = LSWAP(*lp); lp++;}
    }
    else if( (!strncmp(form,"I",1)) || (!strncmp(form,"(I)",3)) )
    {
      if(!strncmp((char *)&lname,"HEAD",4))
      {
        nevent=lp[2];
        /*printf("nev=%d\n",nevent);*/
	  }
      for(jj=0; jj<dlen; jj++) {*lpd++ = LSWAP(*lp); lp++;}
    }
    else if( (!strncmp(form,"F",1)) || (!strncmp(form,"(F)",3)) )
    {
      for(jj=0; jj<dlen; jj++) {*lpd++ = LSWAP(*lp); lp++;}
    }
    else if( (!strncmp(form,"A",1)) || (!strncmp(form,"(A)",3)) ) /* ????? */
    {
      for(jj=0; jj<dlen; jj++) {*lpd++ = LSWAP(*lp); lp++;}
    }

    /* some special cases - until we have generic algorithm */
    else if( (!strncmp(form,"3I,6F",5)) )
    {
      for(jj=0; jj<dlen; jj++) {*lpd++ = LSWAP(*lp); lp++;}
    }

    else
    {
	  printf("ERROR: unsupported format >");
      for(jj=0; jj<hlen; jj++) printf("%c",form[jj]);
	  printf("< - copy 'as is'\n");

      for(jj=0; jj<dlen; jj++) {*lpd++ = *lp; lp++;}
	}

    bb += dlen;
  }

  return(0);
}
