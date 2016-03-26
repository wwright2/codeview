#!/bin/bash
#
# This exmaple demonstrates the following concepts
# How to read command line options with short or long arguments that contains or not parameters?
# How to define a function and call it?
# How to debug script with extra information available on request from the command line?
# NOTE the trace level apply only after the command line has been processed.
#
# Trace level used in this script
# Error  : 1 * This is the default
# Warning: 2
# Info   : 3
# Debug  : 4
# Verbose: 5
#
# Few special variables, you should know
# $# The number of arguments
# $* The entire argument string
# $? The return code from the last command issued
#

# debug reference: http://www.cyberciti.biz/tips/debugging-shell-script.html
# To be also known. If you uncomment the line below the full trace of execution is echoed on your console.
# You can also run the following command to get the same result:
# bash -x ./getopt-long.sh -a hello
#set -x
# Display the command as they are executed.
# bash -v ./getopt-long.sh -a hello
#set -v

TraceLvl=1

#
# Trace function. Check trace level with threshold trace set
# Echo only if the trace selected is bigger or equal to the threshold
# Param:
#   $1: trace level
#   $2: statement
#
function Trace()
{
   if [ $TraceLvl -ge $1 ]; then
      echo "($1): $2"
   fi
}

#
# Select threshold level
# We validate the trace level provided. Only digit allowed.
# It also convert the argument from string to number
# Param:
#   $1: select trace level
#
function SetTraceLevel() 
{
   TraceLvl=$1
   echo "Trace level set to $TraceLvl"
}


#
# RunAndCheck
# First execute the command passed as first argument
# Then check error code. If an error occur
# Display message and abort building process.
# Depending on the trace level print before and after
# the command is executed.
# Param:
#   $1: statement to execute
#
function RunAndCheck()
{
	Trace 3 "Entering $1..."
	$1
	if [ $? -ne 0 ]; then
		echo "**** ERROR **** Abort script on: $1"
		exit 1
    fi
	Trace 3 "... exiting $1"
}

# GenerateError
# Unless the file abcdef.txt is present on your /tmp folder
# it will generate an error when we try to read it.
# Param:
#   None
function GenerateError()
{
    FILE="/tmp/abcdef.txt"
    RunAndCheck "cat $FILE"    
}

#
# GenerateRandomNb
# function that generate random numbers
# Param:
#   $1: number of random number desired
#
function GenerateRandomNb()
{
    for ((  i = 0 ;  i < $1;  i++  ))
    do
        echo "Rand($i): $RANDOM"
    done
}


#
# Option Handler shows how to read argument passed
# Also call either GenerateRandomNb or GenerateError
# Param:
#   $1: option
#   $2: argument for the option
#
function OnOption()
{
    echo "OnOption($1 $2)"
    if [ $2 == "error" ]; then
        RunAndCheck "GenerateError"
    else
        RunAndCheck "GenerateRandomNb 5"
    fi
}

#
# Display the usage of the script.
# List all command line argument supported.
#
function usage()
{
   echo "  -a|--advance : require argument"
   echo "  -b|--balance : require argument"
   echo "  -h|--help : Show quick help"
   echo "  -t|--trace : Level of trace"
   echo "Usage: $0 -a bonjour -b 1234"
}

#
# Set options variables to support short as well as long arguments
#
getoptions="a:b:ht:"
getlongoptions="advance:,balance:,help,trace:"
if ! options=$(getopt -o $getoptions -l $getlongoptions -- "$@"); 
then
   # something went wrong, getopt will put out an error message for us
   exit 1
fi

#
# Here we start execution
# The first line is a tracing call. By default it is not showing unless you change the 
# default trace level accordingly
#
Trace 3 "Start execution...$#:$0 $1 $2 $3 $4 $5 $6"

#
# Here we test if argument are present at all. By default we can see that the option always 
# show a double dash -- even if the user is not passing any param. If $# == 1 means no arguments.
#
if [ $# -eq 0 ]; then
   echo ".. no argument .."
fi

#
# Here we are parsing all command line arguments.
# Option can be short or long in both case we dispatch the execution
# to an associated function.
#
while [ $# -gt 0 ]
do
   Trace 5 "$#...$0 $1 $2"	# set trace option first to enable the prefered level as soon as possible.
   case $1 in
      -a|--advance) OnOption $1 $2 break ;;
      -b|--balance) OnOption $1 $2 break ;;
      -h|--help) usage; exit 1;;
      -t|--trace) SetTraceLevel $2 break ;;
      (--) shift; break ;;
      (-*) echo "$0: error - unrecognized option $1" 1>&2; exit 1 ;;
   esac
   shift
done

#
# Right here we are ready to exit the shell script. If the trace level is 
# set to Info or above we are echoing the trace.
#
Trace 3 "Stop execution"
