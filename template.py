#! /usr/bin/env python3.3

import sys
import argparse
import os
import shutil
from subprocess import call
import subprocess
import time


defaultBoot="./images/v1.0.0-boot.tar.gz"
defaultRootfs="./images/v1.0.0-rootfs.tar.gz"
defaultCab="../cabinet_console"
defaultSkel="./skel"

#------------------------------------------------------------------------------------------
# Main() execute the plan 
#
def main():
    parser = argparse.ArgumentParser(description='Make Update package using boot.tar.gz, rootfs.tar.gz to feed into mkupdatepkg mkflash.sdb')
    args = getArgs(parser)
    # print("args=",args)

    # Exit
    print ("...goodbye...") 



#------------------------------------------------------------------------------------------
class Chdir:         
  def __init__( self, newPath ):  
    self.savedPath = os.getcwd()
    os.chdir(newPath)

  def __del__( self ):
    os.chdir( self.savedPath )
        
#------------------------------------------------------------------------------------------
def isRoot():
    if os.geteuid() != 0: #or maybe if not os.geteuid()
        die( "Not running as root!")
        sys.exit(1)
        
#------------------------------------------------------------------------------------------
# Exit Script with error message.
def die(msg):
    print ("Exit(1): "+msg)
    sys.exit(1)    

#------------------------------------------------------------------------------------------
def Trace(proc):
    while proc.poll() is None:
        line = proc.stdout.readline().decode()
        if len(line) > 2 :
            print (" ".join(line.split()))

#------------------------------------------------------------------------------------------
def runbash(cmdstr,cwd=".",trace=False, wait=True, sout=subprocess.PIPE):

    fd = sout
    
    print ("cmd=%s\n" % (cmdstr))
        
    process = subprocess.Popen(cmdstr, stdout=fd, stderr=subprocess.STDOUT, bufsize=-1, shell=True)
    if trace :
        Trace(process)
                                    
    if wait :
        process.wait()
        
    #proc_stdout = process.communicate()[0]
    return process


#------------------------------------------------------------------------------------------
# getArgs from command line.
def getArgs(parser):
    parser.add_argument("-b", "--boot",   type=str,  help="boot image file to use, default="+defaultBoot)
    parser.add_argument("-r", "--rootfs", type=str,  help="rootfs image file to use, default="+defaultRootfs)
    parser.add_argument("-c", "--cabinet", type=str, help="cabinet_console directory, default="+defaultCab)
    parser.add_argument("-s", "--skel",   type=str,  help="skeleton directory, default="+defaultSkel)
    
    args = parser.parse_args()


#This idiom means the below code only runs when executed from command line
if __name__ == '__main__':
    main()
