/*
     eviofmtdump.c - dumps data into XML array

     input: iarr[] - data array (words)
            nwrd - length of data array in words
            ifmt[] - format (as produced by eviofmt.c)
            nfmt - the number of bytes in ifmt[]
     return: the number of bytes in 'xml' if everything fine, negative if error

        Converts the data of array (iarr[i], i=0...nwrd-1)
        using the format code      ifmt[j], j=0...nfmt-1)

     algorithm description:
        data processed inside while(ib < nwrd) loop, where 'ib' is iarr[] index; loop breaks when 'ib'
        reaches the number of elements in iarr[]
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "evio.h"

#define MIN(a,b) ( (a) < (b) ? (a) : (b) )


typedef struct
{
    int left;    /* index of ifmt[] element containing left parenthesis */
    int nrepeat; /* how many times format in parenthesis must be repeated */
    int irepeat; /* right parenthesis counter, or how many times format
                  in parenthesis already repeated */
} LV;

#undef DEBUG

/*---------------------------------------------------------------- */
#define MAXDEPTH  512

static char *xml;
static char indentStr[3*MAXDEPTH + 1];

static void indent() {
    xml += sprintf(xml, indentStr);
}

/** Routine to increase the xml indentation by 3 spaces up to the limit. */
static void increaseIndent() {
    int i, len = strlen(indentStr);
    if (len > 3*MAXDEPTH) return;

    for (i=len-1; i<len+3; i++) {
        indentStr[i] = ' ';
    }
    indentStr[len+3] = '\0';
}

/** Routine to decrease the xml indentation by 3 spaces down to 0. */
static void decreaseIndent() {
    int i, len = strlen(indentStr);
    if (len < 3) return;
    indentStr[len-3] = '\0';
}

/** Routine to increase the indentation by 3 spaces up to the limit
 *  and then add the indentation to the xml string being created. */
static void increaseAndIndent() {
    increaseIndent();
    indent();
}

/** Routine to decrease the indentation by 3 spaces down to 0
 *  and then add the indentation to the xml string being created. */
static void decreaseAndIndent() {
    decreaseIndent();
    indent();
}


/*---------------------------------------------------------------- */


/**
 * This routine creates an xml representation of composite type, evio format data.
 * Note that this routine is used on buffers taken from evRead() and so the data
 * is swapped and ready to read so no swapping will be necessary here.
 *
 * @param iarr         pointer to composite data
 * @param nwrd         number of data words (32-bit ints)
 * @param ifmt         array holding translated composite format
 * @param nfmt         length of array, ifmt, in # of chars
 * @param nextrabytes  number of extra bytes/characters to ignore at end of data
 * @param numIndent    number of spaces to use in top-level xml element
 * @param hex          not 0 to print in hex, else 0
 * @param xmlString    pointer to character array being filled with xml representation.
 *                     Caller needs to make sure sufficient room exists to
 *                     write all strings.
 *
 * @return number of characters written into xmlString
 */
int eviofmtdump(int *iarr, int nwrd, unsigned char *ifmt, int nfmt,
                int nextrabytes, int numIndent, int hex, char *xmlString) {

    int i, err, len, imt, ncnf, kcnf, lev, iterm;
    int64_t *b64, *b64end;
    int32_t *b32, *b32end;
    int16_t *b16, *b16end;
    int8_t *b8, *b8end;
    LV lv[10];
    char *xml1 = xmlString;
    int rowNum = 2;
    /* track repeat xml element so it can be ended properly */
    int repeat = 0;

    /* Keep track if repeat value is from reading N or
       is a specific int embedded in the format statement. */
    int repeatFromN = 0;

    /* Initialize indent string */
    indentStr[0] = '\0';
    if (numIndent > 0) {
        for (i = 0; i < numIndent; i++)  indentStr[i] = ' ';
    }
    indentStr[numIndent] = '\0';
    xml = xmlString;

    if (nwrd <= 0 || nfmt <= 0) {
        printf("IN eviofmtdump return early, nwrd = %d, nfmt = %d\n", nwrd, nfmt);
        return(0);
    }

    imt   = 0;   /* ifmt[] index */
    lev   = 0;   /* parenthesis level */
    ncnf  = 0;   /* how many times must repeat a format */
    iterm = 0;

    b8    = (char *)&iarr[0];                  /* beginning of data */
    b8end = (char *)&iarr[nwrd] - nextrabytes; /* end of data + 1 */

#ifdef DEBUG
  printf("\n======== eviofmtdump start (xml=0x%08x) ==========\n",xml);
#endif

    increaseAndIndent();
    xml += sprintf(xml,"<row num=\"1\">");
    increaseIndent();

    while(b8 < b8end) {

        /* get next format code */
        while(1) {
            repeatFromN = 0;
            imt++;
            /* end of format statement reached, back to iterm - last parenthesis or format beginning */
            if (imt > nfmt) {
                imt = 0/*iterm*/; /* Sergey: always start from the beginning of the format for now, will think about it ...*/

                decreaseIndent();
                xml += sprintf(xml,"\n%s</row>", indentStr);
                xml += sprintf(xml,"\n%s<row num=\"%d\">", indentStr, rowNum++);
                increaseIndent();
            }
            /* meet right parenthesis, so we're finished processing format(s) in parenthesis */
            else if (ifmt[imt-1] == 0) {
                /* increment counter */
                lv[lev-1].irepeat++;

                /* if format in parenthesis was processed required number of times */
                if (lv[lev-1].irepeat >= lv[lev-1].nrepeat) {
                    /* store left parenthesis index minus 1
                       (if will meet end of format, will start from format index imt=iterm;
                       by default we continue from the beginning of the format (iterm=0)) */
                    iterm = lv[lev-1].left - 1;
                    lev--; /* done with this level - decrease parenthesis level */

                    decreaseIndent();
                    xml += sprintf(xml,"\n%s</paren>", indentStr);
                    decreaseIndent();
                    xml += sprintf(xml,"\n%s</repeat>", indentStr);
                    repeat--;
                }
                /* go for another round of processing by setting 'imt' to the left parenthesis */
                else {
                    /* points ifmt[] index to the left parenthesis from the same level 'lev' */
                    imt = lv[lev-1].left;

                    decreaseIndent();
                    xml += sprintf(xml,"\n%s</paren>", indentStr);
                    xml += sprintf(xml,"\n%s<paren>",  indentStr);
                    increaseIndent();
                }
            }
            else {
                /* how many times to repeat format code */
                ncnf = ifmt[imt-1]/16;
                /* format code */
                kcnf = ifmt[imt-1] - 16*ncnf;

                /* left parenthesis, SPECIAL case: # of repeats must be taken from data */
                if (kcnf == 15) {
                    /* set it to regular left parenthesis code */
                    kcnf = 0;

                    /* get # of repeats from data (watch out for endianness) */
                    b32 = (int32_t *)b8;
                    ncnf = *b32; /* get #repeats from data */
                    b8 += 4;
                    repeatFromN = 1;
                }

                /* left parenthesis - set new lv[] */
                if (kcnf == 0) {

                    indent(9);
                    xml += sprintf(xml,"\n%s<repeat ", indentStr);

                    if (repeatFromN) {
                        xml += sprintf(xml," n=\"%d\">", ncnf);
                    }
                    else {
                        xml += sprintf(xml," count=\"%d\">", ncnf);
                    }
                    repeat++;

                    increaseIndent();
                    xml += sprintf(xml,"\n%s<paren>",  indentStr);

                    if (ncnf == 0) { /*special case: if N=0, skip to the right paren */
                        iterm = imt-1;
                        while (ifmt[imt-1] != 0) {
                            printf("*\n");
                            imt++;
                        }

                        xml += sprintf(xml,"\n%s</paren>", indentStr);
                        decreaseIndent();
                        xml += sprintf(xml,"\n%s</repeat>", indentStr);
                        repeat--;
                        continue;
                    }

                    /* store ifmt[] index */
                    lv[lev].left = imt;
                    /* how many time will repeat format code inside parenthesis */
                    lv[lev].nrepeat = ncnf;
                    /* cleanup place for the right parenthesis (do not get it yet) */
                    lv[lev].irepeat = 0;
                    /* increase parenthesis level */
                    increaseIndent();
                    lev++;
                }
                /* format F or I or ... */
                else {
                    if ((lev == 0) ||
                        (imt != (nfmt-1)) ||
                        (imt != lv[lev-1].left+1)) {
                    }
                    /* if none of above, we are in the end of format */
                    else {
                        /* set format repeat to the big number */
                        ncnf = 999999999;
                    }
                    break;
                }
            }
        } /* while(1) */


        /* if 'ncnf' is zero, get it from data (always in 'int' format) */
        if (ncnf == 0) {
            /* get # of repeats from data */
            b32 = (int32_t *)b8;
            ncnf = *b32;
            b8 += 4;
            repeatFromN = 1;
        }

        /* at that point we are ready to process next piece of data.
           we have following entry info:
             kcnf - format type
             ncnf - how many times format repeated
             b8 - starting data pointer (it comes from previously processed piece)
        */

        /* 64-bits */
        if (kcnf == 8 || kcnf == 9 || kcnf == 10) {
            int count=1, itemsOnLine=2;
            int oneLine = (ncnf <= itemsOnLine) ? 1 : 0;

            if (kcnf == 8) {
                xml += sprintf(xml,"\n%s<float64 ", indentStr);
            }
            else if (kcnf == 9) {
                xml += sprintf(xml,"\n%s<int64 ", indentStr);
            }
            else {
                xml += sprintf(xml,"\n%s<uint64 ", indentStr);
            }

            if (repeatFromN) {
                xml += sprintf(xml,"n=\"%d\">", ncnf);
            }
            else {
                xml += sprintf(xml,"count=\"%d\">", ncnf);
            }

            if (!oneLine) {
                increaseIndent();
                xml += sprintf(xml,"\n");
            }
            else {
                xml += sprintf(xml," ");
            }

            b64 = (int64_t *)b8;
            b64end = b64 + ncnf;
            if (b64end > (int64_t *)b8end) b64end = (int64_t *)b8end;

            while (b64 < b64end) {
                if (!oneLine && count % itemsOnLine == 1) {
                    indent();
                }

                if (kcnf == 8) {
                    xml += sprintf(xml,"%#25.17g  ", *((double *)(b64)));
                }
                else if (hex) {
                    xml += sprintf(xml,"0x%016llx  ", *b64);
                }
                else if (kcnf == 9) {
                    xml += sprintf(xml,"%20lld  ", *b64);
                }
                else {
                    xml += sprintf(xml,"%20llu  ", (uint64_t)(*b64));
                }

                if (!oneLine && count++ % itemsOnLine == 0) {
                    xml += sprintf(xml,"\n");
                }

                b64++;
            }
            b8 = (int8_t *)b64;

            if (!oneLine) {
                decreaseIndent();
                if ((count - 1) % itemsOnLine != 0) {
                    xml += sprintf(xml,"\n%s", indentStr);
                }
            }

            if (kcnf == 8) {
                xml += sprintf(xml,"</float64>");
            }
            else if (kcnf == 9) {
                xml += sprintf(xml,"</int64>");
            }
            else {
                xml += sprintf(xml,"</uint64>");
            }
        }

        /* 32-bits */
        else if ( kcnf == 1 || kcnf == 2 || kcnf == 11 || kcnf == 12) {
            float f;
            int count=1, itemsOnLine=4;
            int oneLine = (ncnf <= itemsOnLine) ? 1 : 0;

            if (kcnf == 2) {
                xml += sprintf(xml,"\n%s<float32 ", indentStr);
            }
            else if (kcnf == 1) {
                xml += sprintf(xml,"\n%s<uint32 ", indentStr);
            }
            else if (kcnf == 11) {
                xml += sprintf(xml,"\n%s<int32 ", indentStr);
            }
            else {
                xml += sprintf(xml,"\n%s<Hollerit ", indentStr);
            }

            if (repeatFromN) {
                xml += sprintf(xml,"n=\"%d\">", ncnf);
            }
            else {
                xml += sprintf(xml,"count=\"%d\">", ncnf);
            }

            if (!oneLine) {
                increaseIndent();
                xml += sprintf(xml,"\n");
            }
            else {
                xml += sprintf(xml," ");
            }

            b32 = (int32_t *)b8;
            b32end = b32 + ncnf;
            if (b32end > (int32_t *)b8end) b32end = (int32_t *)b8end;

            while (b32 < b32end) {
                if (!oneLine && count % itemsOnLine == 1) {
                    indent();
                }

                if (kcnf == 2) {
                    xml += sprintf(xml,"%#15.8g  ", *((float *)(b32)));
                }
                else if (hex) {
                    xml += sprintf(xml,"0x%08x  ", *b32);
                }
                else if (kcnf == 1) {
                    xml += sprintf(xml,"%11u  ", (uint32_t)(*b32));
                }
                else {
                    xml += sprintf(xml,"%11d  ", *b32);
                }

                if (!oneLine && count++ % itemsOnLine == 0) {
                    xml += sprintf(xml,"\n");
                }

                b32++;
            }

            b8 = (int8_t *)b32;

            if (!oneLine) {
                decreaseIndent();
                if ((count - 1) % itemsOnLine != 0) {
                    xml += sprintf(xml,"\n%s", indentStr);
                }
            }

            if (kcnf == 2) {
                xml += sprintf(xml,"</float32>");
            }
            else if (kcnf == 1) {
                xml += sprintf(xml,"</uint32>");
            }
            else if (kcnf == 11) {
                xml += sprintf(xml,"</int32>");
            }
            else {
                xml += sprintf(xml,"</Hollerit>");
            }
        }

        /* 16 bits */
        else if ( kcnf == 4 || kcnf == 5) {
            short s;
            int count=1, itemsOnLine=8;
            int oneLine = (ncnf <= itemsOnLine) ? 1 : 0;

            if (kcnf == 4) {
                xml += sprintf(xml,"\n%s<int16 ", indentStr);
            }
            else {
                xml += sprintf(xml,"\n%s<uint16 ", indentStr);
            }

            if (repeatFromN) {
                xml += sprintf(xml,"n=\"%d\">", ncnf);
            }
            else {
                xml += sprintf(xml,"count=\"%d\">", ncnf);
            }

            if (!oneLine) {
                increaseIndent();
                xml += sprintf(xml,"\n");
            }
            else {
                xml += sprintf(xml," ");
            }

            b16 = (int16_t *)b8;
            b16end = b16 + ncnf;
            if (b16end > (int16_t *)b8end) b16end = (int16_t *)b8end;

            while(b16 < b16end) {
                if (!oneLine && count % itemsOnLine == 1) {
                    indent();
                }

                if (hex) {
                    xml += sprintf(xml,"0x%04hx  ", *b16);
                }
                else if (kcnf == 4)  {
                    xml += sprintf(xml,"%6hd  ", *b16);
                }
                else {
                    xml += sprintf(xml,"%6hu  ", *b16);
                }

                if (!oneLine && count++ % itemsOnLine == 0) {
                    xml += sprintf(xml,"\n");
                }

                b16++;
            }

            b8 = (int8_t *)b16;

            if (!oneLine) {
                decreaseIndent();
                if ((count - 1) % itemsOnLine != 0) {
                    xml += sprintf(xml,"\n%s", indentStr);
                }
            }

            if (kcnf == 4) {
                xml += sprintf(xml,"</int16>");
            }
            else {
                xml += sprintf(xml,"</uint16>");
            }
        }

        /* 8 bits */
        else if ( kcnf == 6 || kcnf == 7 || kcnf == 3) {
            /* String data type */
            if (kcnf == 3) {
                char **strArray = NULL;

                xml += sprintf(xml,"\n%s<string ", indentStr);

                if (repeatFromN) {
                    xml += sprintf(xml,"n=\"%d\">", ncnf);
                }
                else {
                    xml += sprintf(xml,"count=\"%d\">", ncnf);
                }

                increaseIndent();
                xml += sprintf(xml,"\n");

                err = evBufToStrings((char *)b8, ncnf, &strArray, &len);
                for (i=0; i < len; i++) {
                    xml += sprintf(xml,"%s<![CDATA[%s]]>\n", indentStr, strArray[i]);
                    free(strArray[i]);
                }
                free(strArray);

                decreaseIndent();
                xml += sprintf(xml,"%s</string>", indentStr);
                b8 += ncnf;
            }
            /* char & unsigned char ints */
            else {
                int count=1, itemsOnLine=8;
                int oneLine = (ncnf <= itemsOnLine) ? 1 : 0;

                if (kcnf == 6) {
                    xml += sprintf(xml,"\n%s<int8 ", indentStr);
                }
                else {
                    xml += sprintf(xml,"\n%s<uint8 ", indentStr);
                }

                if (repeatFromN) {
                    xml += sprintf(xml,"n=\"%d\">", ncnf);
                }
                else {
                    xml += sprintf(xml,"count=\"%d\">", ncnf);
                }

                if (!oneLine) {
                    increaseIndent();
                    xml += sprintf(xml,"\n");
                }
                else {
                    xml += sprintf(xml," ");
                }

                for (i=0; i < ncnf; i++) {
                    if (!oneLine && count % itemsOnLine == 1) {
                        indent();
                    }

                    if (hex) {
                        xml += sprintf(xml,"0x%02hhx  ", b8[i]);
                    }
                    else if (kcnf == 6)  {
                        xml += sprintf(xml,"%4d  ", b8[i]);
                    }
                    else {
                        xml += sprintf(xml,"%4u  ", ((unsigned char *)b8)[i]);
                    }

                    if (!oneLine && count++ % itemsOnLine == 0) {
                        xml += sprintf(xml,"\n");
                    }
                }

                b8 += ncnf;

                if (!oneLine) {
                    decreaseIndent();
                    if ((count - 1) % itemsOnLine != 0) {
                        xml += sprintf(xml,"\n%s", indentStr);
                    }
                }

                if (kcnf == 6) {
                    xml += sprintf(xml,"</int8>");
                }
                else {
                    xml += sprintf(xml,"</uint8>");
                }
            }
        }
    } /* while(b8 < b8end) */

    if (repeat > 0) {
        decreaseIndent();
        xml += sprintf(xml,"\n%s</paren>", indentStr);
        decreaseIndent();
        xml += sprintf(xml,"\n%s</repeat>", indentStr);
    }

    decreaseIndent();
    xml += sprintf(xml,"\n%s</row>\n", indentStr);

#ifdef DEBUG
  printf("\n======== eviofmtdump finish (xml1=0x%08p, xml=0x%08p, len=%d) ==========\n",xml1,xml,(int)(xml-xml1));
  fflush(stdout);
#endif

    return((int)(xml-xml1));
}
