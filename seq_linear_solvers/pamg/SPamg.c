
/*BHEADER**********************************************************************
 * (c) 1998   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/
/******************************************************************************
 *
 * Serial-Parallel AMG driver   (SPamg)
 *
 *****************************************************************************/


#include "headers.h"


/*--------------------------------------------------------------------------
 *Spamg
 *--------------------------------------------------------------------------*/

int
main( int   argc,
      char *argv[] )
{
   void             *amg_data;

   hypre_CSRMatrix  *A;

   hypre_Vector     *f;
   hypre_Vector     *u;

   double            strong_threshold;
   char              *filename;
   int               solve_err_flag;
   int               num_fine;

   int               cycle_type;
   int               j;
   double           *tmp;

#if 1
   int     *num_grid_sweeps;  
   int     *grid_relax_type;   
   int    **grid_relax_points; 

   num_grid_sweeps = hypre_CTAlloc(int,4);
   grid_relax_type = hypre_CTAlloc(int,4);
   grid_relax_points = hypre_CTAlloc(int *,4);

   /* fine grid */
   num_grid_sweeps[0] = 2;
   grid_relax_type[0] = 0; 
   grid_relax_points[0] = hypre_CTAlloc(int, 2); 
   grid_relax_points[0][0] = 1;
   grid_relax_points[0][1] = -1;

   /* down cycle */
   num_grid_sweeps[1] = 2;
   grid_relax_type[1] = 0; 
   grid_relax_points[1] = hypre_CTAlloc(int, 2); 
   grid_relax_points[1][0] = 1;
   grid_relax_points[1][1] = -1;

   /* up cycle */
   num_grid_sweeps[2] = 2;
   grid_relax_type[2] = 0; 
   grid_relax_points[2] = hypre_CTAlloc(int, 2); 
   grid_relax_points[2][0] = -1;
   grid_relax_points[2][1] = 1;

   /* coarsest grid */
   num_grid_sweeps[3] = 1;
   grid_relax_type[3] = 9;
   grid_relax_points[3] = hypre_CTAlloc(int, 1);
   grid_relax_points[3][0] = 0;
#endif

   if (argc < 4)
   {
      fprintf(stderr, "Usage:  SPamg <file> <strong_threshold>  <mu>\n");
      exit(1);
   }

  /*-------------------------------------------------------
    * Set up debugging tools
    *-------------------------------------------------------*/
   
   hypre_InitMemoryDebug(0); 

  /*-------------------------------------------------------
    * Begin AMG driver
    *-------------------------------------------------------*/
           
   strong_threshold = atof(argv[2]);
   cycle_type = atoi(argv[3]);
   

   amg_data = hypre_AMGInitialize();
   hypre_AMGSetStrongThreshold(strong_threshold, amg_data);
   hypre_AMGSetLogging(3,"amg.out.log",amg_data);
   hypre_AMGSetCycleType(cycle_type, amg_data);

#if 1
   hypre_AMGSetNumGridSweeps(num_grid_sweeps, amg_data);
   hypre_AMGSetGridRelaxType(grid_relax_type, amg_data);
   hypre_AMGSetGridRelaxPoints(grid_relax_points, amg_data);
#endif

   filename = argv[1];
   A = hypre_ReadCSRMatrix(filename);

   num_fine = hypre_CSRMatrixNumRows(A);

   f = hypre_CreateVector(num_fine);
   hypre_InitializeVector(f);
   hypre_SetVectorConstantValues(f, 0.0);
                              
   u = hypre_CreateVector(num_fine);
   hypre_InitializeVector(u);

   tmp = hypre_CTAlloc(double, num_fine);
   for (j = 0; j < num_fine; j++)
   {
       tmp[j] = hypre_Rand();
   }
   hypre_VectorData(u) = tmp;

/*   hypre_SetVectorConstantValues(u, 1.0); */

   hypre_AMGSetup(amg_data,A);

   solve_err_flag = hypre_AMGSolve(amg_data, f, u);

   hypre_FinalizeMemoryDebug(); 
                
   return 0;
}













