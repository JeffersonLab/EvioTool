/*
 * Main2 is for testing the string manipulation
 * routines which facilitate the splitting and automatic
 * naming of files.
 *
 * Main is for testing the evBufToStrings() routine.
 *
 * author: Carl Timmer
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evio.h"

#define MAXBUFLEN  4096
#define MIN(a,b) (a<b)? a : b

extern int evOpenFake(char *filename, char *flags, int *handle, char **evf);

/* e, v, i, o, null, 4, 4, 4*/
static  char good1[8] =  {(char)101, (char)118, (char)105, (char)111,
                          (char)0,   (char)4,   (char)4,   (char)4};

/* e, v, i, o, null, H, E, Y, H, O, null, 4*/
static  char good2[12] =  {(char)101, (char)118, (char)105, (char)111,
                          (char)0,   (char)72,  (char)69,  (char)89,
                          (char)72,  (char)79,  (char)0,   (char)4};

/* e, v, i, o, null, x, y, z*/
static  char good3[8] =  {(char)101, (char)118, (char)105, (char)111,
                          (char)0,   (char)120, (char)121, (char)122};

/* e, v, i, o, -, x, y, z*/
static  char bad1[8] =  {(char)101, (char)118, (char)105, (char)111,
                         (char)45,  (char)120, (char)121, (char)122};

/* e, v, i, o, 3, 4, 4, 4*/  /* bad format 1 */
static  char bad2[8]  =  {(char)101, (char)118, (char)105, (char)111,
                          (char)3,   (char)4,   (char)4,   (char)4};

/* e, v, i, o, 0, 4, 3, 3, 3, 3, 3, 4*/  /* bad format 2 */
static  char bad3[12]  =  {(char)101, (char)118, (char)105, (char)111,
                          (char)0,   (char)4,   (char)3,   (char)3,
                          (char)3,   (char)3,   (char)3,   (char)4};

/* e, v, i, o, null, H, E, Y, H, O, 3, 4*/  /* bad format 4 */
static  char bad4[12]  =  {(char)101, (char)118, (char)105, (char)111,
                          (char)0,   (char)72,  (char)69,  (char)89,
                          (char)72,  (char)79,  (char)3,   (char)4};

/* e, v, i, o, null, 4, 3, 4*/  /* bad format 3 */
static  char bad5[8] =  {(char)101, (char)118, (char)105, (char)111,
                         (char)0,   (char)4,   (char)3,   (char)4};

/* whitespace chars - only printing ws chars are 9,10,13*/
static  char ws[9] =  {(char)9,   (char)10,  (char)11,  (char)12,
                       (char)13,  (char)28,  (char)29,  (char)30,
                       (char)31};

/* e, v, i, o, ws, ws, ws, ws, ws, ws, ws, ws, null, 4, 4 (test whitespace chars) */
static  char bad6[16] = {(char)101, (char)118, (char)105, (char)111,
                         (char)9,   (char)10,  (char)11,  (char)12,
                         (char)13,  (char)28,  (char)29,  (char)30,
                         (char)31,  (char)0,   (char)4,   (char)4};

/* e, 0, 4*/  /* too small */
static char bad7[3] =  {(char)101, (char)0,   (char)4};



int main (int argc, char **argv) {
    char **array;
    int strCount;

    int err = evBufToStrings(bad7, 3, &array,  &strCount);
}


int main2 (int argc, char **argv) {
    int err, handle, specifierCount=-1;
    EVFILE *a;

    char *orig = "My_%s_%3d_$(BLAH)_";
    char *replace = "X";
    char *with = "$(BLAH)";

    char *baseName, *tmp, *result = evStrReplace(orig, replace, with);

    if (argc > 1) orig = argv[1];

    printf("String = %s\n", orig);

    printf("Remove specs : %s\n", evStrRemoveSpecifiers(orig));


    printf("OUT    = %s\n", result);

    /* Replace environmental variables */
    tmp = evStrReplaceEnvVar(result);
    printf("ENV    = %s\n", tmp);
    free(tmp);
    free(result);

    /* Fix specifiers and fix 'em */
    result = evStrFindSpecifiers(orig, &specifierCount);
    if (result == NULL) {
        printf("error in evStrFindSpecifiers routine\n");
    }
    else {
        printf("SPEC    = %s, count = %d\n", result, specifierCount);
    }

    /* Simulate evOpen() */
    err = evOpenFake(strdup(orig), "w", &handle, &tmp);
    if (err != S_SUCCESS) {
        printf("Error in evOpenfake(), err = %x\n", err);
        exit(0);
    }

    a = (EVFILE *) tmp;

    printf("opened file = %s\n", a->baseFileName);


    err = evGenerateBaseFileName(a->baseFileName, &baseName, &specifierCount);
    if (err != S_SUCCESS) {
        printf("Error in evGenerateBaseFileName(), err = %x\n", err);
        exit(0);
    }

    free(a->baseFileName);
    a->baseFileName   = baseName;
    a->specifierCount = specifierCount;
    a->runNumber = 7;
    a->split = 0;
    a->splitNumber = 666;
    a->runType = "runType";

    printf("BASE   = %s, count = %d\n", baseName, specifierCount);

    result = evGenerateFileName(a, a->specifierCount, a->runNumber,
                                a->split, a->splitNumber, a->runType);
    if (result == NULL) {
        printf("Error in evGenerateFileName()\n");
        exit(0);
    }


    printf("FINAL = %s, count = %d\n", result, specifierCount);
    free(result);

    exit(0);
}
