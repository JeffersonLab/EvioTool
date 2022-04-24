/*----------------------------------------------------------------------------*
 *  Copyright (c) 1998        Southeastern Universities Research Association, *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12H        *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-5800             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      Monitors an ET system by text output
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#include "et_private.h"
#include "et_data.h"

/* prototypes */

/******************************************************/
int main(int argc,char **argv)
{  
  int             c, counter, etdead, mode, errflg=0, locality, tmparg;
  unsigned int    newheartbt, oldheartbt=0;
  int             port = ET_BROADCAST_PORT;
  extern char     *optarg;
  extern int      optind, opterr, optopt;
  uint64_t        prev_out;
  struct timespec timeout, period;
  double	  tperiod, hbperiod;
  et_sys_id       sys_id;
  et_id 	  *id;
  et_openconfig   openconfig;
  char            hostname[ET_MAXHOSTNAMELEN];
  char            etname[ET_FILENAME_LENGTH];
  char            *tmp_etname=NULL, *tmp_hostname=NULL;

  char *pSharedMem, *ptr;
  et_mem etInfo;
  int i, swap = 0;
  
  /* defaults */
  mode = ET_HOST_AS_LOCAL;
  period.tv_sec = 5;
  period.tv_nsec = 0;
  tperiod  = period.tv_sec + (1.e-9)*period.tv_nsec;
  hbperiod = ET_BEAT_SEC   + (1.e-9)*ET_BEAT_NSEC;
  
  /* decode command line options */
  while ((c = getopt(argc, argv, "Hf:t:p:h:")) != EOF) {
      switch (c) {
          case 'f':
              if (strlen(optarg) >= ET_FILENAME_LENGTH) {
                  fprintf(stderr, "%s: ET file name is too long\n", argv[0]);
                  exit(-1);
              }
              strcpy(etname, optarg);
              tmp_etname = etname;
              break;

          case 't':
              tmparg = atoi(optarg);
              if (tmparg <= 0) {
                  fprintf(stderr, "%s: argument for -t <time period (sec)> must be integer >0\n\n", argv[0]);
                  errflg++;
                  break;
              }
              period.tv_sec = tmparg;
              tperiod = period.tv_sec + (1.e-9)*period.tv_nsec;
              break;

          case 'p':
              tmparg = atoi(optarg);
              if ((tmparg <= 1024) || (tmparg > 65535)) {
                  fprintf(stderr, "%s: argument for -p <port #> must be integer > 1024 and < 65536\n\n", argv[0]);
                  errflg++;
                  break;
              }
              port = tmparg;
              break;

          case 'h':
              if (strlen(optarg) >= ET_MAXHOSTNAMELEN) {
                  fprintf(stderr, "host name is too long\n");
                  exit(-1);
              }
              strcpy(hostname, optarg);
              tmp_hostname = hostname;
              break;

          case 'H':
              errflg++;
              break;

          case '?':
              errflg++;
      }
  }
  
  for ( ; optind < argc; optind++) {
      errflg++;
  }
  
  /* Check the ET system name */
  if (tmp_etname == NULL) {
      fprintf(stderr, "%s: No ET file name given and SESSION env variable not defined\n", argv[0]);
      exit(-1);
  }
    
  if (errflg) {
      printf("\nUsage: %s [-f <et_filename>] [-p <port#>] [-h <host>] [-t <time period (sec)>]\n\n", argv[0]);
      printf("           Monitors an ET system given by -f <et_filename> (default = /tmp/et_sys_<SESSION>)\n");
      printf("           Uses port specified by -p <port> for broadcasting on local subnets\n");
      printf("           Updates information every -t <time period> seconds (default = 5)\n");
      printf("           Assumes host is local unless specified by -h <host>\n");
      printf("             which can be: localhost, .local, .remote,\n");
      printf("             anywhere, <host name>, or <host IP address>\n\n");
      exit(2);
  }
  
  
  /* before we open things, find out if we're local or not */
  if (et_look(&sys_id, etname) != ET_OK) {
      printf("%s: et_attach problems\n", argv[0]);
      exit(1);
  }
    
  id = (et_id *) sys_id;
    
  ptr = (char *)id->pmap;
  
  etInfo.byteOrder = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  if (etInfo.byteOrder != 0x01020304) {
      swap = 1;
  }
  
  etInfo.systemType      = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  etInfo.majorVersion    = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  etInfo.minorVersion    = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  etInfo.headerByteSize  = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
    
  etInfo.eventByteSize   = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  etInfo.headerPosition  = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  etInfo.dataPosition    = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  etInfo.totalSize       = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  etInfo.usedSize        = *((uint64_t *)ptr);

  if (swap) {
      etInfo.systemType      = ET_SWAP32(etInfo.systemType);
      etInfo.majorVersion    = ET_SWAP32(etInfo.majorVersion);
      etInfo.minorVersion    = ET_SWAP32(etInfo.minorVersion);
      etInfo.headerByteSize  = ET_SWAP32(etInfo.headerByteSize);
      
      etInfo.eventByteSize   = ET_SWAP64(etInfo.eventByteSize);
      etInfo.headerPosition  = ET_SWAP64(etInfo.headerPosition);
      etInfo.dataPosition    = ET_SWAP64(etInfo.dataPosition);
      etInfo.totalSize       = ET_SWAP64(etInfo.totalSize);
      etInfo.usedSize        = ET_SWAP64(etInfo.usedSize);
  }

  /* print out data */
  ptr = (char *)id->pmap;
  //ptr = (char *)id->pmap + etInfo.dataPosition;
  for (i=0; i < (etInfo.usedSize + ET_INITIAL_SHARED_MEM_DATA_BYTES)/4 ; i++) {
      printf("data[%d]  = %d\n",i,  ET_SWAP32(*((uint32_t *)ptr) ));
      ptr += sizeof(uint32_t);
  }
  
  et_unlook(sys_id);

  return 0;
}
