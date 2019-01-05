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
 *      Routines to allocate & attach to mapped memory
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>

#include "et_private.h"

int et_mem_create(const char *name, size_t memsize, void **pmemory, size_t *totalSize)
/* name    = name of file (or possibly shared memory)
 * memsize = necessary et memory size in bytes
 * pmemory = pointer to pointer to useable part of mmapped memory
 * totalsize = total size of mapped mem
 */
{
  int       fd, num_pages, err;
  void     *pmem;
  size_t    wantedsize, totalsize, pagesize;
  mode_t    mode;

  /* get system's pagesize in bytes: 8192-sun, 4096-linux */
  pagesize = (size_t) getpagesize();

  /* Calculate mem size for everything, adding room for initial data in mapped mem */
  wantedsize = memsize + ET_INITIAL_SHARED_MEM_DATA_BYTES;
  num_pages  = (int) ceil( ((double) wantedsize)/pagesize );
  totalsize  = pagesize * num_pages;
  /*printf("et_mem_create: size = %d bytes, requested size = %d bytes\n",totalsize, memsize);*/

  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH | S_IXUSR | S_IXGRP | S_IXOTH ;
  if ((fd = open(name, O_RDWR|O_CREAT|O_EXCL, mode)) < 0) {
    /* file exists already */
    return ET_ERROR_EXISTS;
  }
  else {
    /* set (shared mem, file) size */
    if (ftruncate(fd, (off_t) totalsize) < 0) {
      close(fd);
      unlink(name);
      return ET_ERROR;
    }
  }
    
  /* map mem to process space */
  if ((pmem = mmap((caddr_t) 0, totalsize, PROT_READ|PROT_WRITE,
                   MAP_SHARED, fd, (off_t)0)) == MAP_FAILED) {
    close(fd);
    unlink(name);
    return ET_ERROR;
  }

  /* close fd for mapped mem since no longer needed */
  err = fchmod(fd, mode);
  if (err < 0) {
    perror("et_mem_create: ");
  }
  close(fd);
  
  if (pmemory   != NULL) *pmemory = pmem;
  if (totalSize != NULL) *totalSize = totalsize;
  
  return ET_OK;
}

/***************************************************/
/* Write 6, 32-bit ints and 5, 64-bit ints at the very
 * beginning of the ET system file for a total of 64 bytes.
 * In order this includes:
 *
 * byteOrder      : when read should be 0x0403020
 *
 *
 * 1, if not, byte order
 *                : is reversed from local order.
 * sytemType      : type of local system using the mapped memory.
 *                : Right now there are only 2 types. One is an ET
 *                : system written in C (= ET_SYSTEM_TYPE_C).
 *                : The other is an ET system written in Java
 *                : with a different layout of the shared memory
 *                : (= ET_SYSTEM_TYPE_JAVA).
 * major version  : major version # of this ET software release
 * minor version  : minor version # of this ET software release
 * # select ints  : # station selection integers / event
 * headerByteSize : total size of a header structure in bytes
 * eventByteSize  : total size of a single event's data memory in bytes
 * headerPosition : number of bytes past start of shared memory
 *                : that the headers are stored.
 * dataPosition   : number of bytes past start of shared memory
 *                : that the data are stored.
 * totalsize      : total size of mapped memory (mapped
 *                : memory must be allocated in pages).
 * usedsize       : desired size of mapped memory given as arg
 *                : to et_mem_create.
 */
void et_mem_write_first_block(char *ptr,
                               uint32_t headerByteSize, uint64_t eventByteSize,
                               uint64_t headerPosition, uint64_t dataPosition,
                               uint64_t totalByteSize,  uint64_t usedByteSize)
{
    *((uint32_t *)ptr) = 0x04030201;             ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = ET_SYSTEM_TYPE_C;       ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = ET_VERSION;             ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = ET_VERSION_MINOR;       ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = ET_STATION_SELECT_INTS; ptr += sizeof(uint32_t);
    *((uint32_t *)ptr) = headerByteSize;         ptr += sizeof(uint32_t);

    *((uint64_t *)ptr) = eventByteSize;  ptr += sizeof(uint64_t);
    *((uint64_t *)ptr) = headerPosition; ptr += sizeof(uint64_t);
    *((uint64_t *)ptr) = dataPosition;   ptr += sizeof(uint64_t);
    *((uint64_t *)ptr) = totalByteSize;  ptr += sizeof(uint64_t);
    *((uint64_t *)ptr) = usedByteSize;
}


/***************************************************/
/* Attach to shared memory already created by
 * the ET system process.
 *
 * The first bit of data in the mapped mem is the total size of
 * mapped memory.
 */
int et_mem_attach(const char *name, void **pmemory, et_mem *pInfo)
{
  int        fd;
  char      *ptr;
  size_t     totalsize;
  void      *pmem;
  et_mem     info;

  /* open file */
  if ((fd = open(name, O_RDWR, S_IRWXU)) < 0) {
    perror("et_mem_attach: open - ");
    return ET_ERROR;
  }
   
  /* map small amount of mem in this process & find some info */
  if ((pmem = mmap((caddr_t) 0, ET_INITIAL_SHARED_MEM_DATA_BYTES, PROT_READ|PROT_WRITE,
                    MAP_SHARED, fd, (off_t)0)) == MAP_FAILED) {
    close(fd);
    perror("et_mem_attach: mmap - ");
    return ET_ERROR;
  }

  ptr = (char *)pmem;
  if (pInfo == NULL) {
      pInfo = &info;
  }
  pInfo->byteOrder       = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  pInfo->systemType      = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  pInfo->majorVersion    = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  pInfo->minorVersion    = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  pInfo->numSelectInts   = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
  pInfo->headerByteSize  = *((uint32_t *)ptr);  ptr += sizeof(uint32_t);
    
  pInfo->eventByteSize   = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  pInfo->headerPosition  = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  pInfo->dataPosition    = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  pInfo->totalSize       = *((uint64_t *)ptr);  ptr += sizeof(uint64_t);
  pInfo->usedSize        = *((uint64_t *)ptr);

  totalsize = (size_t) pInfo->totalSize;
  
  /* unmap mem */
  munmap(pmem, ET_INITIAL_SHARED_MEM_DATA_BYTES);

  /* do some error checking before mapping this whole file */

  /* check endian */
  if (pInfo->byteOrder != 0x04030201) {
      if (pInfo->byteOrder == 0x01020304) {
        et_logmsg("ERROR", "et_mem_attach: ET system file is wrong endian\n");
        /* This error may occur because it's a Java ET system and therefore big endian. */
        if (pInfo->systemType == ET_SWAP32(ET_SYSTEM_TYPE_JAVA)) {
            et_logmsg("ERROR", "et_mem_attach: This ET system file is used only for Java ET systems\n");
            close(fd);
            return ET_ERROR_JAVASYS;
        }
      }
      else {
          et_logmsg("ERROR", "et_mem_attach: ET system file removed but process running - kill ET & restart\n");
      }
      close(fd);
      return ET_ERROR;
  }
  
  /* check system type */   
  if (pInfo->systemType == ET_SYSTEM_TYPE_JAVA) {
      et_logmsg("ERROR", "et_mem_attach: This ET system file is used only for Java ET systems\n");
      close(fd);
      return ET_ERROR_JAVASYS;
  }
  
  /* check major version number */
  if (pInfo->majorVersion != ET_VERSION) {
      et_logmsg("ERROR", "et_mem_attach, ET system file is the wrong version (%d), should be %d\n",
                pInfo->majorVersion, ET_VERSION);
      close(fd);
      return ET_ERROR;
  }
  
  /* check number of station selection ints */
  if (pInfo->numSelectInts != ET_STATION_SELECT_INTS) {
      et_logmsg("ERROR", "et_mem_attach, ET system file is the wrong number of station select ints (%d), should be %d\n",
                pInfo->numSelectInts, ET_STATION_SELECT_INTS);
      close(fd);
      return ET_ERROR;
  }
  
  /* finally, remap with proper size */
  if ((pmem = mmap((caddr_t) 0, totalsize, PROT_READ|PROT_WRITE,
                   MAP_SHARED, fd, (off_t)0)) == MAP_FAILED) {
    close(fd);
    perror("et_mem_attach: remmap - ");
    return ET_ERROR;
  }
  
  close(fd);
   
  if (pmemory != NULL) *pmemory = pmem;
  
  return ET_OK;
}

/***************************************************/
int et_mem_size(const char *name, size_t *totalsize, size_t *usedsize)
{
  int     fd;
  void   *pmem;
  char   *ptr;

  /* open file */
  if ((fd = open(name, O_RDWR, S_IRWXU)) < 0) {
    return ET_ERROR;
  }
   
  /* map mem to this process & read data */
  if ((pmem = mmap((caddr_t)0, ET_INITIAL_SHARED_MEM_DATA_BYTES, PROT_READ|PROT_WRITE,
                   MAP_SHARED, fd, (off_t)0)) == MAP_FAILED) {
    close(fd);
    return ET_ERROR;
  }
  
  /* find mapped mem's total size */
  ptr = (char *)pmem + 6*sizeof(uint32_t) + 3*sizeof(uint64_t);
  if (totalsize != NULL) {
      *totalsize = (size_t) (*((uint64_t *)ptr));
  }
  
  /* find mapped mem's size used by ET system */
  ptr += sizeof(uint64_t);
  if (usedsize != NULL) {
    *usedsize = (size_t) (*((uint64_t *)ptr));
  }
  
  close(fd);
  
  /* unmap mem */
  munmap(pmem, ET_INITIAL_SHARED_MEM_DATA_BYTES);

  return ET_OK;
}

/***************************************************/
/* Remove main ET mapped memory */
int et_mem_remove(const char *name, void *pmem)
{
  if (et_mem_unmap(name, pmem) != ET_OK) {
    return ET_ERROR;
  }
    
  if (unlink(name) < 0) { 
    return ET_ERROR;
  }
  return ET_OK;
}

/***************************************************/
/* Unmap main ET mapped memory */
int et_mem_unmap(const char *name, void *pmem)
{
  size_t  totalsize;
  
  if (et_mem_size(name, &totalsize, NULL) != ET_OK) {
    return ET_ERROR;
  }
  
  if (munmap(pmem, totalsize) < 0) {
    return ET_ERROR;
  }
  
  return ET_OK;
}

/***************************************************/
/* Create  mapped memory for temp event */
void *et_temp_create(const char *name, size_t size)
/* name    = name of file (or possibly shared memory)
 * memsize = data size in bytes
 */
{
  int   fd;
  void *pmem;

  unlink(name);
  if ((fd = open(name, O_RDWR|O_CREAT|O_EXCL, S_IRWXU)) < 0) {
    /* failed cause it exists already */
    /* perror("open"); */
    /* printf("et_temp_create: open error %d\n", fd); */
    return NULL;
  }
  else {
    /* set (shared mem, file) size */
    if (ftruncate(fd, (off_t) size) < 0) {
      close(fd);
      unlink(name);
      return NULL;
    }
  }
   
  /* map fd to process mem */
  if ((pmem = mmap((caddr_t)0, size, PROT_READ|PROT_WRITE, MAP_SHARED,
                   fd, (off_t)0)) == NULL) {
    close(fd);
    unlink(name);
    return NULL;
  }

  /* close fd for mapped mem since no longer needed */
  close(fd);
  
  return pmem;
}


/***************************************************/
/* Attach to mapped memory already created for temp event */
void *et_temp_attach(const char *name, size_t size)
{
  int    fd;
  void  *pdata;
  
  if ((fd = open(name, O_RDWR, S_IRWXU)) < 0) {
    printf("et_temp_attach: open error %d\n", fd);
    return NULL;
  }
   
  /* map shared mem to this process */
  if ((pdata = mmap((caddr_t)0, size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, (off_t)0)) == NULL) {
    close(fd);
    return NULL;
  }

  close(fd);
  return pdata;
}

/***************************************************/
/* Remove mapped memory created for temp event */
int et_temp_remove(const char *name, void *pmem, size_t size)
{    
  if (munmap(pmem, size) < 0) {
    return ET_ERROR;
  }
  
  if (unlink(name) < 0) { 
    return ET_ERROR;
  }
  
  return ET_OK;
}




 




