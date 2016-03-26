#!/usr/bin/env python3

import sys
import argparse
import os
import shutil
from subprocess import call
import subprocess
import hashlib
import glob
import tempfile
import time
import stat


defaultDevice="/dev/sdb"
defaultPackage="cicPartition-1.1.0.tar.gz.enc"

DEBUG=False  # (false | true) => cleanup, dont cleanup.  tempfile /tmp/sd_xdidk 
#DEBUG=True  # (false | true) => cleanup, dont cleanup.  tempfile /tmp/sd_xdidk 


#------------------------------------------------------------------------------------------
# Main() execute the plan 
#
def main():
    parser = argparse.ArgumentParser(description='Flash the SD card using pkg.tar.gz.enc created from mkupdate.py')
    args = getArgs(parser)

    print ("args=",args)
    
    checkDevice(args.dev)
    isRoot()
    print ("*** WARNING you are about to FORMAT %s" % (args.dev))
    print (" if you are unsure what this means exit enter NO or ctrl-C ")
    print ("   We shall Unpack package = %s, in /tmp dir and write to device. " % (args.package))
    inputvar = input("\nContinue y/n? (n)")

    if (inputvar == None or inputvar == "") : 
        die ("Exiting...")

    runbash("date > mkflash.time.txt")
    ans = inputvar[0].upper()
    if ans == 'Y' : 
        extractPackage(args)
    
    runbash("date >> mkflash.time.txt")
    runbash("cat mkflash.time.txt",trace=True)
    
    # Completed.
    print ("  ...goodbye...mkflash.py \n")    


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
def checkDevice (dev):
    print (dev)
    statval=""
    try:
        statval = os.stat(dev)
    except :
        print ("Error: %s device does not exist 1" % (dev))
        
    if statval != None or statval !="" :
        # Mounted
        proc = runbash("mount | grep %s" % (dev))
        print (proc.communicate()[0].decode())
        
#        for line in proc.communicate()[0] :
 #           if re.search(dev,line) : 
  #              print ("Error : %s is mounted or busy" % (dev))
    else:
        print ("Error: %s device does not exist 2" % (dev))
       
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
def getArgs(parser):

    global defaultPackage
    global defaultDevice
    
    if not os.path.isfile(defaultPackage) : 
        encList=glob.glob("*.tar.gz.enc")
        if len(encList)>0 :
            defaultPackage = encList[0]
            if defaultPackage=="" : 
                die ("\nUnable to find a package *.tar.gz.enc\n")
        else :
            die ("\nUnable to find a package *.tar.gz.enc ;\n Have you run bin/mkupdate.py to create a update pkg\n")
    
    print ("pkg=%s" % defaultPackage )


    parser.add_argument("-p", "--package", type=str,  help="specify the package.enc file to use, default="+defaultPackage)
    parser.add_argument("-d", "--dev",   type=str,  help="device to use, default="+defaultDevice)
    args = parser.parse_args()
    
    if args.package==None :
        args.package=defaultPackage
    
    if args.dev==None : 
        args.dev=defaultDevice
    
    # validate Args
    # set defaults

    return args
    
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
#------------------------------------------------------------------------------------------
def extractPackage(args):
    tmpDir = tempfile.mkdtemp(prefix="sd_")
#    newdir = Chdir(tmpDir)
    print (os.getcwd())
    proc = runbash("tar -C %s -xzf %s" % (tmpDir, args.package),trace=True)
    list=glob.glob("%s/*-boot.tar.gz" % (tmpDir))
    boot=list[0]
    list=glob.glob("%s/*-rootfs.tar.gz" % (tmpDir))
    rootfs=list[0]
    print (boot,rootfs)
 
    proc = runbash ("bin/mkflash.sh --prompt=NO --dev=%s --boot=%s --rootfs=%s --part=bin/part.sfdisk --mount=./flash " % (
                                args.dev, boot, rootfs), trace=True)
    #print ("%s" % (proc.communicate()[0].decode()))
    
    
    # Debug=False - Cleanup
    # Debug=True - tmp file left to explore    
    #if not DEBUG :
    if True :
        runbash ("rm -rf %s" % (tmpDir))


#------------------------------------------------------------------------------------------
#This idiom means the below code only runs when executed from command line
if __name__ == '__main__':
    main()

