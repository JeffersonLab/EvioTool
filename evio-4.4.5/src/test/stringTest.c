/*
 * This program is for testing the new string manipulation
 * routines which facilitate the splitting and automatic
 * naming of files.
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



int main (int argc, char **argv)
{
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
