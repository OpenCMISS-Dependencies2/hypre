/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision$
 ***********************************************************************EHEADER*/

/******************************************************************************
 *
 * Header file for memory management utilities
 *
 * The abstract memory model has a Host (think CPU) and a Device (think GPU) and
 * three basic types of memory management utilities:
 *
 *    1. Malloc(..., location) 
 *             location=LOCATION_DEVICE - malloc memory on the device
 *             location=LOCATION_HOST   - malloc memory on the host
 *    2. MemCopy(..., method)
 *             method=HOST_TO_DEVICE    - copy from host to device
 *             method=DEVICE_TO_HOST    - copy from device to host
 *             method=DEVICE_TO_DEVICE  - copy from device to device
 *    3. SetExecutionMode
 *             location=LOCATION_DEVICE - execute on the device
 *             location=LOCATION_HOST   - execute on the host
 *
 * Although the abstract model does not explicitly reflect a managed memory
 * model (i.e., unified memory), it can support it.  Here is a summary of how
 * the abstract model would be mapped to specific hardware scenarios:
 *
 *    Not using a device, not using managed memory
 *       Malloc(..., location) 
 *             location=LOCATION_DEVICE - host malloc          e.g., malloc
 *             location=LOCATION_HOST   - host malloc          e.g., malloc
 *       MemoryCopy(..., locTo,locFrom)
 *             locTo=LOCATION_HOST,   locFrom=LOCATION_DEVICE  - copy from host to host e.g., memcpy
 *             locTo=LOCATION_DEVICE, locFrom=LOCATION_HOST    - copy from host to host e.g., memcpy
 *             locTo=LOCATION_DEVICE, locFrom=LOCATION_DEVICE  - copy from host to host e.g., memcpy
 *       SetExecutionMode
 *             location=LOCATION_DEVICE - execute on the host
 *             location=LOCATION_HOST   - execute on the host    
 *
 *    Using a device, not using managed memory
 *       Malloc(..., location) 
 *             location=LOCATION_DEVICE - device malloc        e.g., cudaMalloc
 *             location=LOCATION_HOST   - host malloc          e.g., malloc
 *       MemoryCopy(..., locTo,locFrom)
 *             locTo=LOCATION_HOST,   locFrom=LOCATION_DEVICE  - copy from device to host e.g., cudaMemcpy
 *             locTo=LOCATION_DEVICE, locFrom=LOCATION_HOST    - copy from host to device e.g., cudaMemcpy
 *             locTo=LOCATION_DEVICE, locFrom=LOCATION_DEVICE  - copy from device to device e.g., cudaMemcpy
 *       SetExecutionMode
 *             location=LOCATION_DEVICE - execute on the device
 *             location=LOCATION_HOST   - execute on the host 
 *
 *    Using a device, using managed memory
 *       Malloc(..., location) 
 *             location=LOCATION_DEVICE - managed malloc        e.g., cudaMallocManaged
 *             location=LOCATION_HOST   - host malloc          e.g., malloc
 *       MemoryCopy(..., locTo,locFrom)
 *             locTo=LOCATION_HOST,   locFrom=LOCATION_DEVICE  - copy from device to host e.g., cudaMallocManaged
 *             locTo=LOCATION_DEVICE, locFrom=LOCATION_HOST    - copy from host to device e.g., cudaMallocManaged
 *             locTo=LOCATION_DEVICE, locFrom=LOCATION_DEVICE  - copy from device to device e.g., cudaMallocManaged
 *       SetExecutionMode
 *             location=LOCATION_DEVICE - execute on the device
 *             location=LOCATION_HOST   - execute on the host 
 *
 * Questions:
 *
 *    1. Pinned memory, prefetch?
 *
 *****************************************************************************/

#ifndef hypre_MEMORY_HEADER
#define hypre_MEMORY_HEADER

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HYPRE_MEMORY_DEVICE ( 0)
#define HYPRE_MEMORY_HOST   ( 1)
#define HYPRE_MEMORY_SHARED ( 2)
#define HYPRE_MEMORY_UNSET  (-1)

#if defined(HYPRE_MEMORY_GPU) || defined(HYPRE_USE_MANAGED)

#define hypre_DeviceMemset(ptr, value, type, count) 

#ifdef __cplusplus
extern "C++" {
#endif
#include <cuda.h>
#include <cuda_runtime.h>
#ifdef __cplusplus
}
#endif
#define HYPRE_CUDA_GLOBAL __host__ __device__
#else
#define HYPRE_CUDA_GLOBAL 
#endif

/* OpenMP 4.5 */
#if defined(HYPRE_USE_OMP45)
#include "omp.h"
  
#ifdef __cplusplus
extern "C++" {
#endif
#include <cuda.h>
#include <cuda_runtime.h>
#ifdef __cplusplus
}
#endif

/* stringification:
 * _Pragma(string-literal), so we need to cast argument to a string
 * The three dots as last argument of the macro tells compiler that this is a variadic macro. 
 * I.e. this is a macro that receives variable number of arguments. 
 */
#define HYPRE_STR(s...) #s
#define HYPRE_XSTR(s...) HYPRE_STR(s)

/* OpenMP 4.5 GPU memory management */
/* empty */
#ifndef HYPRE_CUDA_GLOBAL
#define HYPRE_CUDA_GLOBAL
#endif

extern HYPRE_Int hypre__global_offload;
extern HYPRE_Int hypre__offload_device_num;

/* stats */
#define HYPRE_Long long

extern HYPRE_Long hypre__target_allc_count;
extern HYPRE_Long hypre__target_free_count;
extern HYPRE_Long hypre__target_allc_bytes;
extern HYPRE_Long hypre__target_free_bytes;
extern HYPRE_Long hypre__target_htod_count;
extern HYPRE_Long hypre__target_dtoh_count;
extern HYPRE_Long hypre__target_htod_bytes;
extern HYPRE_Long hypre__target_dtoh_bytes;

/* DEBUG MODE: check if offloading has effect 
 * (turned on when configured with --enable-debug) */
#ifdef HYPRE_OMP45_DEBUG
   /* if we ``enter'' an address, it should not exist in device [o.w NO EFFECT] 
      if we ``exit'' or ''update'' an address, it should exist in device [o.w ERROR]
      hypre__offload_flag: 0 == OK; 1 == WRONG
    * */
   #define HYPRE_OFFLOAD_FLAG(devnum, hptr, type) \
      HYPRE_Int hypre__offload_flag = (type[1] == 'n') == omp_target_is_present(hptr, devnum);
#else 
   #define HYPRE_OFFLOAD_FLAG(...) \
      HYPRE_Int hypre__offload_flag = 0; /* non-debug mode, always OK */
#endif

/* OMP 4.5 offloading macro */
#define hypre_omp45_offload(devnum, hptr, datatype, offset, count, type1, type2) \
{\
   /* devnum: device number \
    * hptr: host poiter \
    * datatype: e.g., int, float, double, ... \
    * type1: ``e(n)ter'', ''e(x)it'', or ``u(p)date'' \
    * type2: ``(a)lloc'', ``(t)o'', ``(d)elete'', ''(f)rom'' \
    */ \
   datatype *hypre__offload_hptr = (datatype *) hptr; \
   /* if hypre__global_offload ==    0, or
    *    hptr (host pointer)   == NULL, 
    *    this offload will be IGNORED */ \
   if (hypre__global_offload && hypre__offload_hptr != NULL) { \
      /* offloading offset and size (in datatype) */ \
      HYPRE_Int hypre__offload_offset = offset, hypre__offload_size = count; \
      /* in HYPRE_OMP45_DEBUG mode, we test if this offload has effect */ \
      HYPRE_OFFLOAD_FLAG(devnum, hypre__offload_hptr, type1) \
      if (hypre__offload_flag) { \
         printf("[!NO Effect! %s %d] device %d target: %6s %6s, data %p, [%d:%d]\n", __FILE__, __LINE__, devnum, type1, type2, (void *)hypre__offload_hptr, hypre__offload_offset, hypre__offload_size); exit(0); \
      } else { \
         HYPRE_Int offload_bytes = count * sizeof(datatype); \
         /* printf("[            %s %d] device %d target: %6s %6s, data %p, [%d:%d]\n", __FILE__, __LINE__, devnum, type1, type2, (void *)hypre__offload_hptr, hypre__offload_offset, hypre__offload_size); */ \
         if (type1[1] == 'n' && type2[0] == 't') { \
            /* enter to */\
            hypre__target_allc_count ++; \
            hypre__target_allc_bytes += offload_bytes; \
            hypre__target_htod_count ++; \
            hypre__target_htod_bytes += offload_bytes; \
            _Pragma (HYPRE_XSTR(omp target enter data map(to:hypre__offload_hptr[hypre__offload_offset:hypre__offload_size]))) \
         } else if (type1[1] == 'n' && type2[0] == 'a') { \
            /* enter alloc */ \
            hypre__target_allc_count ++; \
            hypre__target_allc_bytes += offload_bytes; \
            _Pragma (HYPRE_XSTR(omp target enter data map(alloc:hypre__offload_hptr[hypre__offload_offset:hypre__offload_size]))) \
         } else if (type1[1] == 'x' && type2[0] == 'd') { \
            /* exit delete */\
            hypre__target_free_count ++; \
            hypre__target_free_bytes += offload_bytes; \
            _Pragma (HYPRE_XSTR(omp target exit data map(delete:hypre__offload_hptr[hypre__offload_offset:hypre__offload_size]))) \
         } else if (type1[1] == 'x' && type2[0] == 'f') {\
            /* exit from */ \
            hypre__target_free_count ++; \
            hypre__target_free_bytes += offload_bytes; \
            hypre__target_dtoh_count ++; \
            hypre__target_dtoh_bytes += offload_bytes; \
            _Pragma (HYPRE_XSTR(omp target exit data map(from:hypre__offload_hptr[hypre__offload_offset:hypre__offload_size]))) \
         } else if (type1[1] == 'p' && type2[0] == 't') { \
            /* update to */ \
            hypre__target_htod_count ++; \
            hypre__target_htod_bytes += offload_bytes; \
            _Pragma (HYPRE_XSTR(omp target update to(hypre__offload_hptr[hypre__offload_offset:hypre__offload_size]))) \
         } else if (type1[1] == 'p' && type2[0] == 'f') {\
            /* update from */ \
            hypre__target_dtoh_count ++; \
            hypre__target_dtoh_bytes += offload_bytes; \
            _Pragma (HYPRE_XSTR(omp target update from(hypre__offload_hptr[hypre__offload_offset:hypre__offload_size]))) \
         } else {\
            printf("error: unrecognized offloading type combination!\n"); exit(-1); \
         } \
      } \
   } \
}

/* NOTE:
 * if HYPRE_OMP45_OFFLOAD is turned off, then the associated 
 * device memory operations in the following macros will have no effect
 */

#define HYPRE_OMP45_SZE_PAD (sizeof(size_t))

#define HYPRE_OMP45_CNT_PAD(elt_size) ((HYPRE_OMP45_SZE_PAD + elt_size - 1) / elt_size)

/* DataCopyToData: HostToDevice 
 * src:  [from] a CPU ptr 
 * dest: [to]   a mapped CPU ptr */
#define hypre_DataCopyToData(src, dest, type, count) \
{\
   /* CPU memcpy */ \
   if (dest != src) { \
      memcpy(dest, src, sizeof(type)*count); \
   } \
   /* update to device */ \
   size_t size_inuse = sizeof(type) * count; \
   hypre_omp45_offload(hypre__offload_device_num, dest, type, 0, count, "update", "to"); \
}

/* DataCopyFromData: DeviceToHost 
 * src:  [from] a mapped CPU ptr
 * dest: [to]   a CPU ptr */
#define hypre_DataCopyFromData(dest, src, type, count) \
{\
   /* update from device */ \
   size_t size_inuse = sizeof(type) * count; \
   hypre_omp45_offload(hypre__offload_device_num, src, type, 0, count, "update", "from"); \
   /* CPU memcpy */ \
   if (dest != src) { \
      memcpy(dest, src, sizeof(type)*count); \
   } \
}

#if 0
/* DeviceMemset 
 * memset: [to] a mapped CPU ptr
 * memset host memory first and the update the device memory */
#define hypre_DeviceMemset(ptr, value, type, count) \
{\
   /* host memset */ \
   memset(ptr, value, (count)*sizeof(type)); \
   /* update to device */ \
   size_t size_inuse = sizeof(type) * count; \
   hypre_omp45_offload(hypre__offload_device_num, ptr, type, 0, count, "update", "to"); \
}
#endif

#define hypre_InitMemoryDebug(id)

#define hypre_FinalizeMemoryDebug()

#endif // OMP45



#define hypre_InitMemoryDebug(id)
#define hypre_FinalizeMemoryDebug()
//#define TRACK_MEMORY_ALLOCATIONS 1
#if defined(TRACK_MEMORY_ALLOCATIONS)
typedef struct {
  char *file;
  int line;
  int type;} pattr_t;
pattr_t *patpush(void *ptr, pattr_t *ss);
#define hypre_TAlloc(type, count, location) \
  ( (type *)hypre_MAllocIns((size_t)(sizeof(type) * (count)), location,__FILE__,__LINE__) )

#define hypre_CTAlloc(type, count, location) \
  ( (type *)hypre_CAllocIns((size_t)(count), (size_t)sizeof(type), location,__FILE__,__LINE__) )

#define hypre_TReAlloc(ptr, type, count, location) \
  ( (type *)hypre_ReAllocIns((char *)ptr, (size_t)(sizeof(type) * (count)), location,__FILE__,__LINE__) )

void assert_check(void *ptr, char *file, int line);

void assert_check_host(void *ptr, char *file, int line);


#define ASSERT_MANAGED(ptr)\
  ( assert_check((ptr),__FILE__,__LINE__))

#define ASSERT_HOST(ptr)\
  ( assert_check_host((ptr),__FILE__,__LINE__))

#else

#define ASSERT_MANAGED(ptr) (ptr)

#define ASSERT_HOST(ptr) (ptr)

#define hypre_TAlloc(type, count, location) \
  ( (type *)hypre_MAlloc((size_t)(sizeof(type) * (count)), location) )

#define hypre_CTAlloc(type, count, location) \
  ( (type *)hypre_CAlloc((size_t)(count), (size_t)sizeof(type), location) )

#define hypre_TReAlloc(ptr, type, count, location) \
( (type *)hypre_ReAlloc((char *)ptr, (size_t)(sizeof(type) * (count)), location) )

#endif



#define hypre_TFree(ptr,location) \
( hypre_Free((char *)ptr, location), ptr = NULL )

#define hypre_TMemcpy(dst, src, type, count, locdst, locsrc) \
(hypre_Memcpy((char *)(dst),(char *)(src),(size_t)(sizeof(type) * (count)),locdst, locsrc))

//#define hypre_DeviceMemset(ptr,value,type,count)	memset(ptr,value,count*sizeof(type))
  
#define hypre_PinnedTAlloc(type, count)\
( (type *)hypre_MAllocPinned((size_t)(sizeof(type) * (count))) )

/*--------------------------------------------------------------------------
 * Prototypes
 *--------------------------------------------------------------------------*/

/* hypre_memory.c */
HYPRE_Int hypre_OutOfMemory ( size_t size );
char *hypre_MAlloc( size_t size , HYPRE_Int location );
char *hypre_MAllocIns( size_t size , HYPRE_Int location,char *file,int line);
char *hypre_CAlloc( size_t count ,  size_t elt_size , HYPRE_Int location);
char *hypre_CAllocIns( size_t count ,  size_t elt_size , HYPRE_Int location,char *file, int line);
char *hypre_MAllocPinned( size_t size );
char *hypre_ReAlloc( char *ptr ,  size_t size , HYPRE_Int location);
char *hypre_ReAllocIns( char *ptr ,  size_t size , HYPRE_Int location,char *file, int line);
void hypre_Free( char *ptr , HYPRE_Int location );
char *hypre_CAllocHost( size_t count,size_t elt_size );
char *hypre_MAllocHost( size_t size );
char *hypre_ReAllocHost( char   *ptr,size_t  size );
void hypre_FreeHost( char *ptr );
char *hypre_SharedMAlloc ( size_t size );
char *hypre_SharedCAlloc ( size_t count , size_t elt_size );
char *hypre_SharedReAlloc ( char *ptr , size_t size );
void hypre_SharedFree ( char *ptr );
void hypre_Memcpy( char *dst, char *src, size_t size, HYPRE_Int locdst, HYPRE_Int locsrc );
void hypre_MemcpyAsync( char *dst, char *src, size_t size, HYPRE_Int locdst, HYPRE_Int locsrc );	
HYPRE_Real *hypre_IncrementSharedDataPtr ( HYPRE_Real *ptr , size_t size );

/* memory_dmalloc.c */
HYPRE_Int hypre_InitMemoryDebugDML( HYPRE_Int id );
HYPRE_Int hypre_FinalizeMemoryDebugDML( void );
char *hypre_MAllocDML( HYPRE_Int size , char *file , HYPRE_Int line );
char *hypre_CAllocDML( HYPRE_Int count , HYPRE_Int elt_size , char *file , HYPRE_Int line );
char *hypre_ReAllocDML( char *ptr , HYPRE_Int size , char *file , HYPRE_Int line );
void hypre_FreeDML( char *ptr , char *file , HYPRE_Int line );

#ifdef __cplusplus
}
#endif

#endif

