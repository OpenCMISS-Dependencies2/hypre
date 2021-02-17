#!/bin/sh
# Copyright 1998-2019 Lawrence Livermore National Security, LLC and other
# HYPRE Project Developers. See the top-level COPYRIGHT file for details.
#
# SPDX-License-Identifier: (Apache-2.0 OR MIT)


# global variables
BatchMode=0
NoRun=0
JobCheckInterval=10        # sleep time between jobs finished check
InputString=""
RunPrefix=`type -p mpirun`
RunPrefix="$RunPrefix -np"
RunString=""
RunEcho=""
ExecFileNames=""           # string of executable file names used
TestDirNames=""            # string of names of TEST_* directories used
HOST=`hostname`
NumThreads=0               # number of OpenMP threads to use if > 0
Valgrind=""                # string to add to MpirunString when using valgrind
mpibind=""                 # string to add to MpirunString when using mpibind
SaveExt="saved"            # saved file extension

function usage
{
   printf "\n"
   printf "$0 [options] {test_path}/{test_name}.sh\n"
   printf "\n"
   printf " where: {test_path} is the directory path to the test script;\n"
   printf "        {test_name} is a user defined name for the test script\n"
   printf "\n"
   printf " with options:\n"
   printf "    -h|-help       prints this usage information and exits\n"
   printf "    -mpi <prefix>  MPI run prefix; default is 'mpirun -np'\n"
   printf "    -nthreads <n>  use 'n' OpenMP threads\n"
   printf "    -rtol <tol>    use relative tolerance 'tol' to compare numeric test values\n"
   printf "    -atol <tol>    use absolute tolerance 'tol' to compare numeric test values\n"
   printf "    -save <ext>    use '<test>.saved.<ext> for the saved-file extension\n"
   printf "    -valgrind      use valgrind memory checker\n"
   printf "    -mpibind       use mpibind\n"
   printf "    -n|-norun      turn off execute mode, echo what would be run\n"
   printf "    -t|-trace      echo each command\n"
   printf "    -D <var>       define <var> when running tests\n"
   printf "\n"
   printf " This is the hypre test driver script.  It is run stand-alone\n"
   printf " or by the autotest regression test script.  It is assumed that\n"
   printf " there are test directories test/TEST_{solver} that contain:\n" 
   printf "   1. Individual test scripts named {test_name}.jobs that provide\n"
   printf "         the mpirun execution syntax\n"
   printf "   2. Test run output files named {test_name}.out.{number}\n"
   printf "   3. Individual scripts to compare (usually using diff) output\n"
   printf "         files from corresponding {test_name}.jobs scripts\n"
   printf "\n"
   printf " Ideally, the *.jobs and *.sh scripts can be run as stand-alone\n"
   printf " shell script files.  A test is considered successful when there \n"
   printf " are no error files generated by the *.sh scripts.\n"
   printf "\n"
   printf " NOTE: This script knows about most of the ASC machines\n"
   printf " and will automatically use the Livermore Computing Resource\n" 
   printf " Management (LCRM) batch system as needed.\n"
   printf "\n"
   printf " Example usage: ./runtest.sh -t TEST_sstruct/*.sh\n"
   printf "\n"
}

# generate default command based on the first 4 characters of the platform name
function MpirunString
{
   case $HOST in
      *bgl*)
         BatchMode=1
         shift
         MY_NUM_TASKS=$1  
         MY_EXECUTE_DIR=`pwd`
         MY_EXECUTE_JOB=`pwd`/$EXECFILE
         shift
         shift
         MY_ARGS="$*"
         RunString="mpirun -verbose 1 -np $MY_NUM_TASKS -exe $MY_EXECUTE_JOB"
         RunString="${RunString} -cwd $MY_EXECUTE_DIR -args \" $MY_ARGS \" "
         ;;
      up*)
         CPUS_PER_NODE=8
         POE_NUM_PROCS=$2
         POE_NUM_NODES=`expr $POE_NUM_PROCS + $CPUS_PER_NODE - 1`
         POE_NUM_NODES=`expr $POE_NUM_NODES / $CPUS_PER_NODE`
         shift
         shift
         MY_ARGS="$*"
         # RunString="poe $EXECFILE -rmpool pbatch -procs $POE_NUM_PROCS"
         # RunString="${RunString} -nodes $POE_NUM_NODES $MY_ARGS"
         RunString="poe $MY_ARGS -rmpool pdebug -procs $POE_NUM_PROCS -nodes $POE_NUM_NODES"
         ;;
      rztopaz*|aztec*|cab*|quartz*|sierra*|syrah*|vulcan*)
         shift
         if [ $NumThreads -gt 0 ] ; then
            export OMP_NUM_THREADS=$NumThreads
            RunString="srun -p pdebug -c $NumThreads -n$*"
         else
            RunString="srun -p pdebug -n$*"
         fi
         ;;
      surface*)
         shift
         RunString="srun -n$*"
         ;;
      pascal*)
         shift
         RunString="srun -n$*"
         ;;
      rzansel*)
         shift
         RunString="lrun -T$*"
         ;;
      ray*)
         shift
         RunString="lrun -n$*"
         ;;
      lassen*)
         shift
         RunString="lrun -n$*"
         ;;
      *)
         shift
         if [ $NumThreads -gt 0 ] ; then
            export OMP_NUM_THREADS=$NumThreads
         fi
         RunString="$RunPrefix $1"
         shift
         RunString="$RunString $mpibind $Valgrind $*"
         ;;
   esac
}

# determine the "number of nodes" desired by dividing the "number of processes"
# by the "number of CPU's per node" which can't be determined dynamically (real
# ugly hack)
function CalcNodes
{
   NUM_PROCS=1
   NUM_NODES=1
   CPUS_PER_NODE=1
   case $HOST in
      alc*) CPUS_PER_NODE=2
         ;;
      peng*) CPUS_PER_NODE=2
         ;;
      thun*) CPUS_PER_NODE=4
         ;;
      *bgl*) CPUS_PER_NODE=2
         ;;
      up*) CPUS_PER_NODE=8
         ;;
      *dawn*) CPUS_PER_NODE=4
         ;;
      vert*) CPUS_PER_NODE=2
         ;;
      hera*) CPUS_PER_NODE=16
         ;;
      *zeus*) CPUS_PER_NODE=8
         ;;
      *) CPUS_PER_NODE=1
         ;;
   esac

   while [ "$1" ]
   do
      case $1 in
         -n*) NUM_PROCS=$2
            NUM_NODES=`expr $NUM_PROCS + $CPUS_PER_NODE - 1`
            NUM_NODES=`expr $NUM_NODES / $CPUS_PER_NODE`
            return $NUM_NODES
            ;;
         *) shift
            ;;
      esac
   done
   return 1
}

# extract the "number of processes/task"
function CalcProcs
{
   while [ "$1" ]
   do
      case $1 in
         -n*) return $2
            ;;
         *) shift
            ;;
      esac
   done
   return 1
}

# check the path to the executable if the executable exists; save the name to
# ExecFileNames
function CheckPath
{
   while [ "$1" ]
   do
      case $1 in
         -n*) EXECFILE=$3
            if [ -x $StartDir/$EXECFILE ] ; then
               cp -f $StartDir/$EXECFILE $EXECFILE
               ExecFileNames="$ExecFileNames $EXECFILE"
               return 0
            else
               echo $EXECFILE
               echo "Cannot find executable!!!"
               return 1
            fi
            return 0
            ;;
         *) shift
            ;;
      esac
   done
   return 1
}

# initialize the common part of the " PsubCmd" string, ugly global vars!
# global "RunName" is assumed to be predefined
#
# on ubgl, as of 8/2006, only allowable number of nodes are 32, 128 and 
# multiples of 512
function PsubCmdStub
{
   CalcNodes "$@"
   NumNodes=$?
   CalcProcs "$@"
   NumProcs=$?
   case $HOST in
      alc*) PsubCmd="psub -c alc,pbatch -b casc -r $RunName -ln $NumProcs"
         ;;
      peng*) PsubCmd="psub -c pengra,pbatch -b casc -r $RunName -ln $NumProcs"
         ;;
      thun*) PsubCmd="psub -c thunder,pbatch -b casc -r $RunName -ln $NumNodes -g $NumProcs"
         ;;
      vert*) PsubCmd="psub -c vertex,pbatch -b casc -r $RunName -ln $NumProcs"
         ;;
      *bgl*) PsubCmd="psub -c ubgl -pool pbatch -b science -r $RunName -ln 32"
         ;;
      up*) PsubCmd="psub -c up -pool pbatch -b a_casc -r $RunName -ln $NumProcs"
         ;;
      *dawn*) PsubCmd="psub -c dawndev -pool pdebug -r $RunName"
         ;;
      hera*) PsubCmd="psub -c hera,pbatch -b casc -r $RunName -ln $NumProcs"
         ;;
      *zeus*) PsubCmd="psub -c zeus,pbatch -b casc -r $RunName -ln $NumProcs"
         ;;
      atla*) PsubCmd="psub -c atlas,pbatch -b casc -r $RunName -ln $NumProcs"
         ;;
      *) PsubCmd="psub -b casc -r $RunName -ln $NumProcs"
         ;;
   esac
}

# read job file line by line saving arguments
function ExecuteJobs
{
   StartDir=$1
   WorkingDir=$2
   TestName=$3
   ReturnFlag=0              # error return flag
   BatchFlag=0               # #BATCH option detected flag 
   BatchCount=0              # different numbering for #Batch option
   PrevPid=0
   SavePWD=`pwd`
##
##     move to specified directory
   cd $WorkingDir

##     open *.jobs files for reading
   while read InputLine
   do
      case $InputLine in
         "#BATCH"*) BatchFlag=1
            BatchFile=""
            ;;

         "#END"*) BatchFlag=0
            chmod +x $BatchFile
            PsubCmd="$PsubCmd -o $OutFile -e $ErrFile `pwd`/$BatchFile"
            if [ "$NoRun" -eq 0 ] ; then
               CmdReply=`$PsubCmd`
            fi
            PrevPid=`echo $CmdReply | cut -d \  -f 2`
            while [ "`pstat | grep $PrevPid`" ]
            do
               sleep $JobCheckInterval
            done
            BatchFile=""
            ;;

         *"#"*) :
            ;; 

         *mpirun*)
            RunCmd=`echo $InputLine| sed -e 's/^[ \t]*mpirun[ \t]*//'` # remove 'mpirun'
            RunCmd=`echo $RunCmd | sed -e 's/[ \t]*>.*$//'`            # remove output redirect
            OutFile=`echo $InputLine | sed -e 's/^.*>//'`           # set output file
            OutFile=`echo $OutFile | sed -e 's/ //g'`               # remove extra space
            ErrFile=`echo $OutFile | sed -e 's/\.out\./.err./'`  # set error file
            RunName=`echo $OutFile | sed -e 's/\.out.*$//'`   # set test run name
            CheckPath $RunCmd               # check path to executable
            if [ "$?" -gt 0 ] ; then
               cat >> $RunName.err <<- EOF
Executable doesn't exist command: 
$InputLine 
EOF
               ReturnFlag=1
               break
            fi
            MpirunString $RunCmd            # construct "RunString"
            case $HOST in
               *bgl*) RunString="${RunString} > `pwd`/$OutFile 2>`pwd`/$ErrFile"
                      ;;
               *dawn*) RunString="${RunString} > `pwd`/$OutFile 2>`pwd`/$ErrFile"
                      ;;
            esac
            if [ "$BatchMode" -eq 0 ] ; then
               ${RunEcho} ${RunString} > $OutFile 2> $ErrFile </dev/null
            else
               if [ "$BatchFlag" -eq 0 ] ; then
                  BatchFile=`echo $OutFile | sed -e 's/\.out\./.batch./'`
                  cat > $BatchFile <<- EOF 
cd `pwd`
${RunString}
EOF
                  chmod +x $BatchFile
                  PsubCmdStub ${RunCmd}
                  case $HOST in
                     *bgl*) PsubCmd="$PsubCmd `pwd`/$BatchFile"
                            ;;
                     *dawn*) PsubCmd="$PsubCmd `pwd`/$BatchFile"
                            ;;
                         *) PsubCmd="$PsubCmd -o $OutFile -e $ErrFile `pwd`/$BatchFile"
                            ;;
                  esac
                  if [ "$NoRun" -eq 0 ] ; then
                     CmdReply=`$PsubCmd`
                  fi
                  PrevPid=`echo $CmdReply | cut -d \  -f 2`
                  while [ "`pstat | grep $PrevPid`" ]
                  do
                     sleep $JobCheckInterval
                  done
               else                          # BatchFlag set
                  if [ "$BatchFile" -eq "" ] ; then
                     BatchFile=$TestName.batch.$BatchCount
                     BatchCount=BatchCount+1
                     cat > $BatchFile <<- EOF
cd `pwd`
${RunString}
EOF
                  else
                     cat >> $BatchFile <<- EOF
${RunString}
EOF
                  fi
                  PsubCmdStub ${RunCmd}     # construct a PsubCmd string
               fi                           # BatchFlag set
            fi                              # BatchMode set
            ;;

         *)
            NOTBLANK=`echo $InputLine | sed 's/[ \n\t]//g'`
            if [ "$NOTBLANK" ] ; then
               echo "Found something unexpected in $WorkingDir/$TestName.jobs"
               echo "--> $InputLine"
               exit 1
            fi
            ;;
      esac
   done < $TestName.jobs           # done with open *.jobs file for reading
   cd $SavePWD
   return $ReturnFlag
}

#   compare output files as defined in *.sh files
function ExecuteTest
{
   StartDir=$1
   WorkingDir=$2
   TestName=$3
   SaveName=$TestName.$SaveExt
   RTOL=$4
   ATOL=$5
   SavePWD=`pwd`
   cd $WorkingDir
   (cat $TestName.err.* > $TestName.err)
   (./$TestName.sh $RTOL $ATOL >> $TestName.err 2>&1)
   if [ -z $HYPRE_NO_SAVED ]; then
      if [ -f $SaveName ]; then
         # diff -U3 -bI"time" ${TestName}.saved ${TestName}.out   # old way of diffing
         (../runcheck.sh $TestName.out $SaveName $RTOL $ATOL >> $TestName.err 2>&1)
      fi
   fi
   cd $SavePWD
}

#  report errors from PURIFY and/or INSURE if run 
function PostProcess
{
   StartDir=$1
   WorkingDir=$2
   TestName=$3
   SavePWD=`pwd`
   cd $WorkingDir
   if [ "$BatchMode" -eq 0 ] ; then
      if [ -f purify.log ] ; then
         mv purify.log $TestName.purify.log
         grep -i hypre_ $TestName.purify.log >> $TestName.err
      elif [ -f insure.log ] ; then
         if [ -f ~/insure.log ] ; then
            cat ~/insure.log >> insure.log
            rm -f ~/insure.log*
         fi
         mv insure.log $TestName.insure.log
         grep -i hypre_ $TestName.insure.log >> $TestName.err
      fi
   fi
   cd $SavePWD
}

               
# removes executables from all TEST_* directories
function CleanUp
{
   if [ "$BatchMode" -eq 0 ] ; then
      for i in $TestDirNames
      do 
         for j in $ExecFileNames
         do 
            ExecuteFile=$i/$j
            if [ -x $ExecuteFile ] ; then
               rm -f $ExecuteFile
            fi
         done
         case $i in
            TEST_examples)
               rm -f ex? ex?? ex??f
         esac
      done
   fi
}

# process files
function StartCrunch
{
   rm -f ~/insure.log*

   TestName=$3

   ExecuteJobs "$@"
   ExecuteTest "$@"
   PostProcess "$@"
}

#==========================================================================
#==========================================================================

# main
# Set default check tolerance
while [ "$*" ]
do
   case $1 in
      -h|-help)
         usage
         exit
         ;;
      -mpi)
         shift
         MPIRunPrefix=$1
         shift
         ;;
      -nthreads)
         shift
         NumThreads=$1
         shift
         ;;
      -rtol)
         shift
         RTOL=$1
         shift
         ;;
      -atol)
         shift
         ATOL=$1
         shift
         ;;
      -save)
         shift
         SaveExt=$SaveExt.$1
         shift
         ;;
      -valgrind)
         shift
         Valgrind="valgrind -q --suppressions=`pwd`/runtest.valgrind --leak-check=yes --track-origins=yes"
         ;;
      -mpibind)
         shift
         mpibind="mpibind"
         ;;
      -n|-norun)
         NoRun=1
         RunEcho="echo"
         shift
         ;;
      -t|-trace)
         set -xv
         shift
         ;;
      -D)
         shift
         eval export `echo $1`=1
         shift
         ;;
      *) InputString=$1
         if [ "$InputString" ] ; then
            if [ -r $InputString ] ; then
               FilePart=`basename $InputString .sh`
               DirPart=`dirname $InputString`
               CurDir=`pwd`
               TestDirNames="$TestDirNames $DirPart"
               case $DirPart in
                  TEST_examples)
#                     ExampleFiles="ex1 ex2 ex3 ex4 ex5 ex5f ex6 ex7 ex8 ex9 ex10 ex11 ex12 ex12f ex13 ex14 ex15"
                     cd ../examples
                     for file in ex*
                     do
                        if [ -x $file ]
                        then
                           cp -f $file $CurDir
                        fi
                     done
                     cd $CurDir
                     ;;
               esac
               if [ -r $DirPart/$FilePart.jobs ] ; then

                  # Check for an mpirun routine
                  if [ x$MPIRunprefix != "x" ]
                  then
                     RunPrefix=$MPIRunPrefix
                  fi

                  StartCrunch $CurDir $DirPart $FilePart $RTOL $ATOL
               else
                  printf "%s: test command file %s/%s.jobs does not exist\n" \
                     $0 $DirPart $FilePart
                  exit 1
               fi
            else
               printf "%s: test command file %s does not exist\n" \
                  $0 $InputString
               printf "can not find .sh file\n"
               exit 1
            fi
         else
            printf "%s: Strange input parameter=%s\n" $0 $InputString
            exit 1
         fi
         shift
         ;;
   esac
done
#
#     remove exectutable files from TEST_* directories
CleanUp $TestDirNames $ExecFileNames

# Filter misleading error messages
cat > runtest.filters <<EOF
lrun warning: default mapping forced to idle
hypre_MPI_Init
job [0-9]* queued and waiting for resources
job [0-9]* has been allocated resources
SLURMINFO: Job [0-9]* is pending allocation of resources.
slurmstepd: error: _is_a_lwp:
ATTENTION: [0-9\-]*  Couldn't create .*, job may not be checkpointable
ATTENTION: [0-9\-]* Error opening file
### .*File.cc.*
EOF
for dir in $TestDirNames
do
  for errfile in $( find $dir -name "*.err" )
  do
    if (egrep -f runtest.filters $errfile > /dev/null) ; then
        original=`dirname $errfile`/`basename $errfile .err`.fil
        echo "This file contains the original copy of $errfile before filtering" > $original
        cat $errfile >> $original
        mv $errfile $errfile.tmp
        egrep -v -f runtest.filters $errfile.tmp > $errfile
        rm -f $errfile.tmp
    fi
  done
done
rm -f runtest.filters
