#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
************************************************
Copyright: copyright Luminator 2011 all rights reserved.

project: ctss - chicago transit shelter sign

module: bustracker

file: busapi.py

history:
    wwright 4/21/2011    retrieve CTA bus tracker api data

annotation:

  Classes:
    - Alpha Listing-
    BusPrediction - Object used to sort fields, particularly the Prediction Time - prdtm
    CharBuffer - force Strings into a specific size buffer for sharing over UDP or mmap
    Tracker - Main Algo. handle Messaging InitConfig,Arrivals, AlertId, AlertData,  URL request/response

    
  Generalization
   ..Start Tracker
        
Todo:
  -Check for memory leak ... i.e. re-use of Timer()

************************************************
"""
import time
from datetime import datetime
from threading import Thread, Timer

from operator import itemgetter, attrgetter
from tracker import Tracker
import signal
import os


if __name__ == "__main__":
  import sys
  
  print ("argv =" + str(len(sys.argv)) )
  for x in sys.argv : 
    print ( ("argv[n] ="+ x) )

# ..create timer thread
t = Tracker()
t.start()

# Get File name
pathname, scriptname  = os.path.split( __file__ )
filesplit = scriptname.split('.')
fname = "{0}.pid".format(filesplit[0])

# Create PID file
pidfile = "/var/run/{0}".format(fname)
f = open(pidfile, 'w')
f.write(str(os.getpid()))
f.close() 

"""
    Define termination handler for busapi Application.
    Define a Signal handler for SIGTERM
"""
def termination():
    if (t.ping_p != None):
        t.ping_p.kill()  #t.exitall()
        t.ping_p = None

    # delete PID file
    t.stop()
    t = None
    os.remove(pidfile)
    print "Exit busapi.py"
    
def SigTERMHandler(signum, frame):
    termination()
    return

"""
    Define a Signal handler for SIGUSR1  which is to be received
    from the remoteShell cta_admin? or rssh what ever we end up using.
"""  

def SigUSR1Handler(signum, frame):
    if (t):
        t.requestConfig()
    return

signal.signal( signal.SIGUSR1, SigUSR1Handler )
signal.signal( signal.SIGTERM, SigTERMHandler )


"""
************************************************
# Main routine
  - create a Tracker Object
  - start prediction timer
  - spin wait for keys

"""



# Send messages
while (1):

    try:
        if ( len(sys.argv) > 1 and sys.argv[1] == "input"):
            data = raw_input('>> ')
            if not data :
                #  UDPSock.sendto("q",addr)
                continue
            else :
                #  if(UDPSock.sendto(data,addr)):
                if (data == "h"):
                    print "h Help"
                    print "r Re-read config file"
                    print "q Quit to exit"
                    continue
                
                elif ( data == "q"):
                    print "quit..."
                    t.stop()
                    break

                elif ( data == "r" ):
                    print ("reread config")
                    if (t.ping_p != None):
                        t.ping_p.kill()
                    t.stop()
                    
                    t = None
                    t = Tracker()
                    t.start()
                    continue
                    
                print "Message = '",str(data),"'....."
        else:
            time.sleep(1)

    except (KeyboardInterrupt, SystemExit):
        print ('KeyboardInterrupt')
        termination()
        sys.exit()
        raise

termination()
sys.exit()









