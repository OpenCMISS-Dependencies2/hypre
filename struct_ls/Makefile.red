#BHEADER***********************************************************************
# (c) 1997   The Regents of the University of California
#
# See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
# notice, contact person, and disclaimer.
#
# $Revision$
#EHEADER***********************************************************************

CC = cicc
F77 = ci77

# CFLAGS = -O
CFLAGS = -O -DHYPRE_TIMING
# CFLAGS = -g -DHYPRE_TIMING

FFLAGS = -O

LFLAGS =\
 -L/usr/local/lib\
 -L.\
 -L../struct_matrix_vector\
 -L../utilities\
 -lHYPRE_ls\
 -lHYPRE_mv\
 -lHYPRE_utilities\
 -lmpi\
 -lm

include Makefile.generic

