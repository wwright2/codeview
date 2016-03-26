#! /usr/bin/env python3

import sys
import argparse
import os
import tarfile
import shutil
from subprocess import call
import subprocess
import hashlib
import time
import tempfile


#parser = argparse.ArgumentParser(description='Make Update package using boot.tar.gz, rootfs.tar.gz to feed into mkupdatepkg mkflash.sdb')

# Neet to use v1.1.0 to get the boot.tar.gz with the proper files.
# v1.1.0 first version with the /boot/cmdline.txt.p2,p3 fstab.p2,p3
defaultBoot="./images/v1.1.0-boot.tar.gz"
defaultRootfs="./images/v1.1.0-rootfs.tar.gz"
defaultCab="../cabinet_console"
defaultSkel="./skel"

defaultInstall="./bin/default_install.sh"
installScript="./install.sh"
logProgress="logProgress.txt"
recordsfile="records.txt"

# Partion, swap partitions on install complet
# Stop Monit Patch just updat some files
# No partition swap Stop Monit etc. update Cur working partition restart
cicStr="cicPartition"   # rootfs, boot
cicPatch="cicPatch"     # files from cabinet_console
cicPackage="cicPackage" # entire cabinet_console
cicUpdateFilename=""    # initialize null


#------------------------------------------------------------------------------------------
def setUpdateFileName(name):
    global cicUpdateFilename 
    cicUpdateFilename = name

#------------------------------------------------------------------------------------------
def getUpdateFileName():
    global cicUpdateFilename 
    return cicUpdateFilename
    
    
#------------------------------------------------------------------------------------------
# Main() execute the plan 
#
# Philosophy:
#  ../cabinet_console is secondary to ./skel files, may contain development only settings
#  so, ./skel is copied on top of cabinet_console in ./target overriding cabinet_console files.
#      ./skel must contain the "Production" settings
#   i.e. ./skel/home/pi/cabinet_console/config.yml has final say when building a SD card
#
#    copyCabinetFiles(args)      # copy cabinet_console to target
#    runMakeBuild(args)          # build, compile, make and copy binaries to ./target
#    copySkeletonFiles(args)     # copy skeleton files to target
#
# ...This documents the original intent: Code may speak differently if some lame brain decides to change the order below.
#   ./skel/install_card.sh looks for /cc_data/db and creates if not there else assumes this card has already been ported to 
#   the new format.
#
# 
#
def main():
 
    runbash("date > mkupdate.time.txt")
    isRoot();
    parser = argparse.ArgumentParser(description='Make Update package using boot.tar.gz, rootfs.tar.gz to feed into mkupdatepkg mkflash.sdb')
    args = getArgs(parser)
    areYouSure(args)            # Are you sure you want to delete existing package
    handleInstallScript(args)   # create if does not exist, else assume ./install.sh is customized and ready
    cleanWorkSpace(args)        # clean up work space, old *.enc,*.tar.gz
    copyImageFiles(args)        # copy base image to be modified
    copyCabinetFiles(args)      # copy cabinet_console to target
    runMakeBuild(args)          # build, compile, make and copy binaries to ./target
    copySkeletonFiles(args)     # copy skeleton files to target
    updateRootfsArchive(args)   # update the base image with new files from target
    

    runbash("date >> mkupdate.time.txt")
    runbash("cat mkupdate.time.txt",trace=True)
    #print ("args=",args)
    print ("created file: %s" % (getUpdateFileName()) )
    print ("  ...goodbye ...mkupdate.py \n")    



#------------------------------------------------------------------------------------------
# Exit Script with error message.
def die(msg):
    print ("Exit(1): "+msg)
    sys.exit(1)    


#------------------------------------------------------------------------------------------
def isRoot():
    if os.geteuid() != 0: #or maybe if not os.geteuid()
        die( "Not running as root!")
        sys.exit(1)


#------------------------------------------------------------------------------------------
# getArgs from command line.
def getArgs(parser):
    parser.add_argument("-b", "--boot",   type=str,  help="boot image file to use, default="+defaultBoot)
    parser.add_argument("-r", "--rootfs", type=str,  help="rootfs image file to use, default="+defaultRootfs)
    parser.add_argument("-c", "--cabinet", type=str, help="cabinet_console directory, default="+defaultCab)
    parser.add_argument("-s", "--skel",   type=str,  help="skeleton directory, default="+defaultSkel)
    
    args = parser.parse_args()
    
    if args.boot==None :
        args.boot=defaultBoot
    
    if args.rootfs==None : 
        args.rootfs=defaultRootfs
    
    if args.cabinet==None :
        args.cabinet=defaultCab
    
    if args.skel==None :
        args.skel = defaultSkel

    if not os.path.isfile(args.boot) : 
        die ("Boot File not found %s" % args.boot)
    if not os.path.isfile(args.rootfs) : 
        die ("Rootfs File not found %s" % args.rootfs)
    if not os.path.isdir(args.cabinet) : 
        die ("Cabinet Directory not found %s" % args.cabinet)
    if not os.path.isdir(args.skel) : 
        die ("Skel Directory not found %s" % args.boot)
                    
    return args



#------------------------------------------------------------------------------------------
# Are you sure you want to delete existing package
def areYouSure(args):
    print ("\n... !!!We are about to delete files and Build new Package ...")
    print ("\n         - if you need to keep prior build output")
    print ("             you need to move/copy from this directory to some safe area.")
    print ("\n... Be sure everything is in place, this usually takes 15min+  ...")
    print ("...Are you sure you want to delete Previouse package, and begin the build \n \n...VALIDATE BELOW...")
    print ("\nboot=%s" % (args.boot))
    print ("rootfs=%s" % (args.rootfs))
    print ("cabinetconsole=%s" % (args.cabinet))
    print ("skeleton=%s\n" % (args.skel))
    inputvar = input("\nContinue y/n? (n)")
    print ("Processing : " + inputvar)

    if inputvar==None or inputvar=="" : 
        die ("Exiting...")
        
    # if First Char is 'Y' continue else exit.
    if isinstance(inputvar,str) :
        answer=inputvar[0].upper()
        if answer != 'Y' : 
            die (" You have chosen to exit the build...Nothing done...chicken")
        

#------------------------------------------------------------------------------------------
def handleInstallScript(args):
    print ("handleInstallScript")
    # If not exist create an install script from the default script.
    if not os.path.isfile(installScript) :
        if (os.path.isfile(defaultInstall) != True):
            die("Unable to create %s from %s " % (installScript,defaultInstall))
        else:
            shutil.copy2(defaultInstall,installScript)
            print ("copy default to ./install.sh")
    else :        
        print (installScript+" found, using the current install script, delete if you want to use the default")
            
#------------------------------------------------------------------------------------------
# clean up work space, old *.enc,*.tar.gz
def cleanWorkSpace(args):
    
    targetlist=os.listdir("./target/")
    cmd = ["rm","-rf","*.tar.gz.enc","*.tar.gz"]
    if len(targetlist) > 0 :
        print (".target/* =", targetlist)
        targetstr = "./target/"
        for item in targetlist : 
            targetstr+item
            cmd.append(targetstr+item)

    print ("cleanWorkSpace ",cmd)
    return_code = call(cmd)
    print ("returned = ", return_code)  

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
# build, compile, make and copy binaries to ./target
def runMakeBuild(args):
    print ("runMakeBuild")
    command="cd pkghandler; make clean ; make config ; make ; make rootfs "
    proc=runbash(command,trace=True)
    if ( proc.returncode != 0):
        die ("Make failed, check that $HOME/local/rpi is defined. If not you need to setup the Cross Compiler. see readme.md")
    print (proc.communicate()[0].decode())
    
                  
#------------------------------------------------------------------------------------------
# copy base image to be modified
def copyImageFiles(args):
    print ("copyImageFiles to .here.")

    print (args.boot)
    if os.path.isfile(args.boot):
        shutil.copy2(args.boot, ".")
    else:
        die ("File not found boot="+args.boot )
    print (args.rootfs)
    if os.path.isfile(args.rootfs):
        shutil.copy2(args.rootfs, ".")
    else:
        die ("File not found boot="+args.rootfs )
    
#------------------------------------------------------------------------------------------
# copy cabinet_console to target
def copyCabinetFiles(args):
    print ("copyCabinetFiles "+ args.cabinet)
    if os.path.isdir(args.cabinet):
        shutil.copytree(args.cabinet,"./target/home/pi/cabinet_console",ignore=shutil.ignore_patterns("frontend"))
    else:
        die ("Dir not found"+args.cabinet)
    
#------------------------------------------------------------------------------------------
# copy skeleton files to target
def copySkeletonFiles(args):

    skelfiles = os.listdir(args.skel)
    recurse= "-r"
    target = "./target/"
    copycmd = "cp"
    cmd=[]
    
        
    print ("copySkeletonFiles ", cmd)
    
    if os.path.isdir(args.skel):
        for item in skelfiles :
            del cmd[:]
            cmd.append(copycmd)
            fname = args.skel+"/"+item
            if os.path.isdir(fname):
                cmd.append(recurse)
            cmd.append(fname)    
            cmd.append(target)
            print (cmd)
             
            return_code = call(cmd)
            print ("returned = ", return_code)
              
#        shutil.copytree(args.skel+"/","./target/",symlinks=True)
    else:
        die ("Dir not found"+args.cabinet)


    
#------------------------------------------------------------------------------------------
def sha1sum(filepath):
    with open(filepath, 'rb') as f:
        return hashlib.sha1(f.read()).hexdigest()
#------------------------------------------------------------------------------------------
# update the base image with new files from target
def updateRootfsArchive(args):

    # Expect to pick up the copy of: so pull ./images off the string.
    rootfs = os.path.basename(args.rootfs)
    boot = os.path.basename(args.boot)
    print ("updateRootfsArchive, %s, %s" % (boot,rootfs) )
    
    proc = runbash("cd ./target/home/pi/cabinet_console; sed -i  -e 's/cu.usbserial/ttyAMA0/' config.yml")
    if not os.path.isfile("./target/home/pi/version") : 
        die (" Error missing version file ")

    print ("ok...files in place")

    print ("This will take a while ... ")
    print ("    Updating %s with Target/* files " % rootfs )
    print ("    we must unzip %s to append/update with target/* " % rootfs)
 
    proc = runbash ("gunzip %s " % rootfs)

    ndxTargz = rootfs.find(".gz")
    rootfsSubstr = rootfs[0:ndxTargz]
    print ("rootfsSubstr=:%s:" % rootfsSubstr)
    if not os.path.isfile(rootfsSubstr) : 
        die ("Failed to gunzip %s " % rootfsSubstr)
        
    print ( "    append newer files tar -C target -rvf %s " % rootfs)
    proc = runbash("tar -C target -rvf %s . " % rootfsSubstr )
    print ("%s" % (proc.communicate()[0].decode()))
    
    # Extraction is taking 32minutes create log Progress indicator
    # Create file to indicate % of tar file extracted to log progress
    print( "...counting Records")
    print("...calculate 5%% records for logging progress and store in %s" % (logProgress))
    tarsize = os.path.getsize(rootfsSubstr)

    print ("records=%d " % (int(tarsize/(10240))))
    rf = open(recordsfile,'w')
    rf.write ("%d" % (int(tarsize/(10240))))
    rf.close()
    f = open(logProgress,'w')
    f.write("%d" % (int(tarsize/(10240)/20)) )   # 5%
    f.close()

    print ("    Zip it up  %s => %s " % (rootfsSubstr,rootfs) )
    runbash ("gzip %s " % rootfsSubstr )

    print ("Done w/ updating root package %s" % rootfs)


    #######  SIGNATURE

    print ("Signature process...")

    passhash="ssl/passhash.txt"
    privatekey="ssl/server.priv.pem"
    publickey="ssl/server.pub.pem"

    distsum="tmp/dist.sum"
    distsig="tmp/dist.sig"
    version = "target/home/pi/version"

    # Create Path
    if not os.path.isdir("tmp"):
        try:  
            os.mkdir ("tmp",'0755')
        
        except ValueError: 
            print ("Directory tmp exists")

    print (" Generate SHA1 checksum file")
    
    try:
        digest=sha1sum(rootfs)
        f = open(distsum,'w')
        f.write("%s  %s\n" % (digest,rootfs) )
        digest=sha1sum(boot)
        f.write("%s  %s\n" % (digest,boot) )
        f.close()

    except ValueError:
        die ("Error creating sha1sum for file=%s" % rootfs )

    print (" Create signature of sha1 ")

    # version file should have been copied from ./skel to ./target 
    #    during the skelCopy() in main()
    # while not a comment '#'
    #    expect version string 
    shutil.copy2(version, ".")
    f = open("version",'r')
    line=f.readline()
    while True :
        if line[0] != '#' :
            break;
        line=f.readline()
        
    versionStr = "".join(line.split())
    f.close()

    pkgenc="%s-%s.tar.gz.enc" % (cicStr,versionStr)
    setUpdateFileName(pkgenc)
    print ("pkg=%s" % pkgenc)
    
    proc=runbash("openssl rsautl -sign -inkey %s  -passin pass:asdf1234 -in %s -out %s" % (privatekey,distsum,distsig) )
    if proc.returncode != 0 : 
         die ("Error signing update tarball %s" % pkgenc)

    #### TAR Signature, payload

    
    files="%s %s %s %s %s %s %s %s" % (distsig, 
            rootfs, 
            "version", 
            boot, 
            distsum, 
            installScript,
            logProgress,
            recordsfile)

    print (" Tar %s %s" % (pkgenc,files) )
    proc = runbash ("tar cvzf %s %s" % (pkgenc,files) )
    if proc.returncode != 0 : 
        die ("Error creating final tarball $pkg \n")
    
    f = open("version",'r')
    versionStr = f.readline()
    f.close()
    print ("created %s with signature=%s and tarfile=%s version %s, %s,  %s, %s" % (pkgenc,
                                                                    distsig,
                                                                    rootfs,
                                                                    versionStr,
                                                                    installScript,
                                                                    logProgress,
                                                                    recordsfile
                                                                   ))


            
# -----------------------------------------------------------------------------------------------
#This idiom means the below code only runs when executed from command line
if __name__ == '__main__':
    main()


