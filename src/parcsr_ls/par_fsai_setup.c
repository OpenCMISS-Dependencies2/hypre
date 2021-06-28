/******************************************************************************
 *  Copyright 1998-2019 Lawrence Livermore National Security, LLC and other
 *  HYPRE Project Developers. See the top-level COPYRIGHT file for details.
 *  
 *  SPDX-License-Identifier: (Apache-2.0 OR MIT)
 ******************************************************************************/

#include "_hypre_parcsr_ls.h"
#include "_hypre_blas.h"
#include "par_fsai.h"

#define DEBUG 0
#define PRINT_CF 0
#define DEBUG_SAVE_ALL_OPS 0

/*****************************************************************************
 *  
 * Routine for driving the setup phase of FSAI
 *
 ******************************************************************************/

/******************************************************************************
 * Helper functions. Will move later.
 ******************************************************************************/

/* TODO - Extract A[P, P] into an IJMatrix */
void
hypre_ParCSRMatrixExtractIJMatrix(hypre_IJMatrix *A_sub, HYPRE_Int *A_rows, HYPRE_Int *A_cols, HYPRE *A_vals, HYPRE_Int *rows, HYPRE_Int nrows){
   
/*
 * Reference par_restr.c ~ line 398-408. (Marker = indices list)
 */


}

/* Extract the data from A[i, P] */
void
hypre_ParCSRMatrixExtractRowData(HYPRE_Int *A_j, HYPRE_Real *A_data, HYPRE_Real *A_sub_j, HYPRE_Real *A_sub_data, HYPRE_Int start_i, HYPRE_Int end_i, HYPRE_Int *needed_cols, HYPRE_Int ncols, HYPRE_Int row){

/*
 * Base on previous function
 */

}

/*****************************************************************************
 * hypre_FSAISetup
 ******************************************************************************/

HYPRE_Int
hypre_FSAISetup( void               *fsai_vdata,
                      hypre_ParCSRMatrix *A  )
{
   MPI_Comm                comm = hypre_ParCSRMatrixComm(A);
   hypre_ParFSAIData       *fsai_data = (hypre_ParFSAIData*) fsai_vdata;
   hypre_MemoryLocation    memory_location = hypre_ParCSRMatrixMemoryLocation(A);

   /* Data structure variables */

   HYPRE_Real              kap_tolerance           = hypre_ParFSAIDataKapTolerance(fsai_data);
   HYPRE_Int               max_steps               = hypre_ParFSAIDataMaxSteps(fsai_data);
   HYPRE_Int               max_step_size           = hypre_ParFSAIDataMaxStepSize(fsai_data);
   HYPRE_Int               logging                 = hypre_ParFSAIDataLogging(fsai_data);
   HYPRE_Int               print_level             = hypre_ParFSAIDataPrintLevel(fsai_data);
   HYPRE_Int               debug_flag;             = hypre_ParFSAIDataDebugFlag(fsai_data);

   /* Declare Local variables */

   HYPRE_Int               num_procs, my_id;

   hypre_MPI_Comm_size(comm, &num_procs);
   hypre_MPI_Comm_rank(comm, &my_id);

   HYPRE_CSRMatrix         *A_diag;
   HYPRE_CSRMatrix         *G;
   HYPRE_Real              *G_temp;
   HYPRE_Real              *kaporin_gradient;
   HYPRE_Real              *A_data;
   HYPRE_Int               *A_i;
   HYPRE_Int               *A_j;
   HYPRE_Int               *row_partition;
   HYPRE_Int               *S_Pattern;
   HYPRE_Real              old_psi, new_psi;
   HYPRE_Real              row_scale;
   HYPRE_Int               global_start, global_end, local_num_rows;
   HYPRE_Int               max_nnz;
   HYPRE_Int               i, j, k;       /* Loop variables */
   
   HYPRE_ParCSRMatrixGetRowPartitioning(A, &row_partition);
   global_start   = row_partition[my_id];
   global_end     = row_partition[my_id+1];
   local_num_rows = global_end - global_start;
   hypre_TFree(row_partition, HYPRE_MEMORY_HOST);
   
   /* Allocating local variables */
   
   min_row_size      = min(max_steps*max_step_size, local_num_rows-1);
   kaporin_gradient  = (HYPRE_Real*) HYPRE_CTAlloc(min_row_size, sizeof(HYPRE_Real), HYPRE_MEMORY_HOST);
   G_temp            = (HYPRE_Real*) HYPRE_CTAlloc(min_row_size, sizeof(HYPRE_Real), HYPRE_MEMORY_HOST);
   S_Pattern         = (HYPRE_Int*) HYPRE_CTAlloc(min_row_size, sizeof(HYPRE_Int), HYPRE_MEMORY_HOST);

   /* Setting local variables */

   A_diag   = hypre_ParCSRMatrixDiag(A);
   A_i      = hypre_CSRMatrixI(A_diag);
   A_j      = hypre_CSRMatrixJ(A_diag);
   A_data   = hypre_CSRMatrixData(A_diag);

   for( i = 0; i < local_num_rows; i++ ){    /* Cycle through each of the local rows */

      for( k = 0; k < max_steps; k++ ){      /* Cycle through each iteration for that row */
         
         /* Steps:
         * Compute Kaporin Gradient
         *  1) kaporin_gradient[j] = 2*( InnerProd(A[j], G_temp[i]) + A[j][i])
         *     kaporin_gradient = 2 * MatVec(A[0:j], G_temp[i]') + 2*A[i] simplified
         *  2) Need a kernel to compute A[P, :]*G_temp - TODO
         
         * Grab max_step_size UNIQUE positions from kaporian gradient
         *  - Need to write my own function. A binary array can be used to mark with locations have already been added to the pattern.
         *
         * Gather A[P, P], G[i, P], and -A[P, i]
         *  - Adapt the hypre_ParCSRMatrixExtractBExt function. Don't want to return a CSR matrix because we're looking for a dense matrix.
         *
         * Determine psi_{k} = G_temp[i]*A*G_temp[i]'
         *
         * Solve A[P, P]G[i, P]' = -A[P, i]
         *
         * Determine psi_{k+1} = G_temp[i]*A*G_temp[i]'
         */
         if(abs( psi_new - psi_old )/psi_old < kaporin_tol)
            break;

      }

      /* row_scale = 1/sqrt(A[i, i] -  abs( InnerProd(G_temp, A[i])) )
      *  G[i] = row_scale * G_temp  
      */

   }

   hypre_TFree(kaporin_gradient);
   hypre_TFree(G_temp);
   hypre_TFree(S_Pattern);

   return(hypre_error_flag);

}
