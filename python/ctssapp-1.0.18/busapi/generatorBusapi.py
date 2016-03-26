#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
************************************************
Copyright:

project: ctss

module: generatorBusapi

file: generatorBusapi.py

history:
    wwright 4/21/2011    retrieve CTA bus tracker api data

annotation:
    Copy of Busapi for EMI testing
    ...But now just read xml files and send to display. Cycle through
     - get list of files ./generatorXml/*.xml
       do forever
         do while Read from list
            display
            wait
         loop
       loop
     

  Classes:
    CharBuffer - force Strings into a specific size buffer for sharing over UDP or mmap
    RouteDestination - Char buffer to store destination Strings.
    SignData - Char Buffers to store sign data and 3-routes used to transmit UDP to display.
    BusPrediction - Object used to sort fields, particularly the Prediction Time - prdtm
    Tracker - Control the URL request/response
    
  Generalization
   ..read config
        ..get stopid
        ..build url
    ..initSocket()
    ..create Timer thread trackerRequest()

    ..trackerRequest() 
        ..send request.
        ..get response
        ..parse response
        ..wake up displayController w/ UDP message
        ..Write Audio file. udp Audio to copy file.
        
Todo:
  -Check for memory leak ... i.e. re-use of Timer()

************************************************
"""

import sys, os, string
import socket, urllib2
import time

from datetime import datetime
from xml.dom import minidom, Node
from socket import *
from threading import Thread, Timer

from Queue import Queue
import syslog
import subprocess

import logging
from logging.handlers import SysLogHandler




#
#   (5+50+7)*3 +50+6+7+5 = 254
#
#struct routeDestination
#{
#    char        rt[5];              //4+1
#    char        destname[50];
#    char        minutes[7];
#};
#struct signData
#{
#    char                stop[50];
#    char                date[6];
#    char                time[7];
#    char                temp[5];
#
#    struct routeDestination     bus[3];
#};

from operator import itemgetter, attrgetter


logger = logging.getLogger()
logger.setLevel(logging.INFO)
syslog = SysLogHandler(address='/dev/log')
formatter = logging.Formatter('%(name)s: %(levelname)s %(message)s')
syslog.setFormatter(formatter)
logger.addHandler(syslog)


if __name__ == "__main__":
  import sys
  
  logger.info ("argv =" + str(len(sys.argv)) )
  for x in sys.argv : 
    logger.info ( ("argv[n] ="+ x) )
  
  


"""
... trackerconf.h
... Enumerated message Types.
"""
# BusPrediction, ServiceBulletin,ServiceBulletinClear, Temperature = range(1,4)


"""
Class CharBuffer(object)
    Create a fixed length buffer to map to 'C' struct 
"""
class CharBuffer(object):
    def __init__(self,l):
        self.length = l
        self.buf = ""
        self.fmt = "%s"
        self.packString("")

    def packString(self,mystring,fmt="%s"):

        if (mystring):
            lenstr = len(mystring)
        else:
            lenstr = 0
        
        # Enough room
        if (lenstr > self.length):
            return 0
        
        self.buf = ""
        for x in mystring :
            self.buf = self.buf + x
            
        for x in range(lenstr,self.length):
            self.buf = self.buf + '\x00'
        self.fmt = "%"+str(self.length)+"s"

        self.buf.rstrip('\n')
                

    def getString(self):
        astring = self.buf
        return (astring)

    def write(self,fp):
        fp.write( self.fmt %(self.buf))


        
"""
class RouteDestination(object):
      map to 'C' struct char x[]  rt, destname, minutes fields
     trackerconf.h
"""
class RouteDestination(object):
    def __init__(self, *args) :
        if ( len(args) ):
            logger.info( args )
        else :
            self.rt         = CharBuffer(5)
            self.destname   = CharBuffer(50)
            self.minutes    = CharBuffer(7)
            
    def setRoute(self,mystring):
        self.rt.packString(mystring)
            
    def setDestname(self,mystring):
        self.destname.packString(mystring)

    def setMinutes(self,mystring):
            self.minutes.packString(mystring)
    
"""
class Signdata(object):
      map to 'C' struct char x[]  stop,date,time,temp
     trackerconf.h
"""
class Signdata(object):
    def __init__(self) :
        #self.msgType = BusPrediction
        self.datalen = 5 + (50+6+8+5) + 3*(5+50+7)
        
        self.stop = CharBuffer(50)
        self.date = CharBuffer(6)
        self.time = CharBuffer(8)
        self.temp = CharBuffer(5)

        bus  = RouteDestination()
        bus2 = RouteDestination()
        bus3 = RouteDestination()
        
        self.buslist = [bus,bus2,bus3]
    
    def writeSigndata(self, filename):
        with open(filename,"w+b") as file:
            self.stop.write(file)
            self.date.write(file)
            self.time.write(file)
            self.temp.write(file)
            i=0
            for x in self.buslist :
                x.rt.write(file)
                x.destname.write(file)
                x.minutes.write(file)
                i =i+1
                if (i>2):
                    break
        file.close()


    def setStop(self,mystring):
        self.stop.packString(mystring)
    def setDate(self,mystring):
        self.date.packString(mystring)
    def setTime(self,mystring):
        self.time.packString(mystring)
    def setTemp(self,mystring):
        self.temp.packString(mystring)
    def setBus(self,ndx,bus):
        self.buslist[ndx] = bus
        
    def getStop(self):
        return self.stop.getString()
    def getDate(self):
        return self.date.getString()
    def getTime(self):
        return self.time.getString()
    def getTemp(self):
        return self.temp.getString()
    def getBuses(self):
        return self.buslist
    
    def toString(self):
        buf = ""
        xyz = [ self.getStop(),self.getDate(),self.getTime(),self.getTemp() ]
        for b in self.getBuses() :
            xyz.append(b.rt.getString())
            xyz.append(b.destname.getString())
            xyz.append(b.minutes.getString())
        for mystr in xyz :
           buf = buf + mystr
        return buf





"""
class Bulletin(object):
      map to 'C' struct 
     trackerconf.h
     char                service[100];   // rt,stopid
     char                priority[30];   // priority of the bulletin low,high?
     char                detail[255];    // 
     char                brief[255];     // Service bulletin to scroll or other Info.
"""
class Bulletin(object):
    def __init__(self) :
        self.msgType = ServiceBulletin
        self.datalen = 5 + (100+30+255+255)
        
        self.service = CharBuffer(100)
        self.priority = CharBuffer(30)
        self.detail = CharBuffer(255)
        self.brief = CharBuffer(255)

    def setService(self,mystring):
        self.service.packString(mystring)
        
    def getService(self):
        return self.service.getString()

"""
************************************************
class BusPrediction(object)
        - sort the bus predictions objects.
"""
class BusPrediction(object) :
    def __init__(self, *args) :
        #tmstmp='', typ='', stpnm='', stpid='', vid='', dstp='', rt='', rtdir='', des='', prdtm='')
        if (len(args) == 10 ) :
            self.tmstmp = args[0]
            self.typ    = args[1]
            self.stpnm  = args[2]
            self.stpid  = args[3]
            self.vid    = args[4]
            self.dstp   = args[5]
            self.rt     = args[6]
            self.rtdir  = args[7]
            self.des    = args[8]
            self.prdtm  = args[9]
        else :
            tmstmp=''; typ=''; stpnm=''; stpid=''; vid=''; dstp=''; rt=''; rtdir=''; des=''; prdtm='';
        
    def __repr__(self):
        return repr((self.tmstmp, self.typ, self.stpnm, self.stpid, self.vid, self.dstp, self.rt, self.rtdir, self.des, self.prdtm))

    def sortPrdtm(*args):
        busPrediction_objects = args
        return sorted( busPrediction_objects, key=lambda busPrediction: busPrediction.prdtm )

        

"""
************************************************
class GenTracker (object)
        init
            get env
            get conf
            Poll the BusAPI for Bus Predictions
            Parse XML save to file for 'C' bustracker to read and post to display.

    predictionkeys = 'tmstmp','typ','stpnm','stpid','vid','dstp','rt','rtdir','des','prdtm'
            
"""
class GenTracker(object) :

    config = dict()


    
    def __del__(self):
        self.UDPSock.close()
        self.UDPSockAudio.close()
        self.predictionTimer.stop()
        self.fp.flush()
        self.fp.close()
       
    """
    ************************************************
    Method:  def __init__(self) :
    """
    def __init__(self) :
        self.readconfig()
        self.mysign = Signdata()

        self.ping_p = None

        # Create socket
        self.UDPSock = socket(AF_INET,SOCK_DGRAM)
        self.UDPSockAudio = socket(AF_INET,SOCK_DGRAM)
        self.addr = (self.config.get('tracker_display_ip','127.0.0.1'), int(self.config.get('tracker_display_port','15545')))
        self.addrAudio= ( self.config.get('tracker_audio_ip','127.0.0.1'), int(self.config.get('tracker_audio_port','15546')) )
        logger.info ("tracker sock display =" + str(self.addr))
        logger.info( "tracker sock audio =" + str(self.addrAudio))
        
        # Create prediciton url
        #url = "http://www.ctabustracker.com/bustime/api/v1/getvehicles?key=N6pVyqBWg6t5H43nfra6JwYVy&rt=8A"
        busstop = string.rstrip(self.config.get('busStop_Id', '14222'))
        if (busstop == '$ID'):
            busstop = '14222'

        self.predictionUrl = self.config.get('tracker_url', 'http://www.ctabustracker.com/bustime/api/v1/') \
                + self.config.get('cmdGetPredictions', 'getpredictions') \
                + "?key=" + self.config.get('ctakey', 'N6pVyqBWg6t5H43nfra6JwYVy') + "&stpid=" \
                + busstop
                

        maxbuses = self.config['tracker_maxbusprediction']
        if (len(maxbuses)):
            self.predictionUrl = self.predictionUrl + "&top="+ maxbuses
        
        logger.info ("url=" + self.predictionUrl)

        # No reason to poll faster than 60seconds buses only report every 1 min.            
        self.predictionTimeout = int(self.config.get('tracker_getprediction_timeout_sec', '140'))
        if (self.predictionTimeout < 2):
            self.predictionTimeout = 2

        # Error on request resend in n Seconds.
        self.resendPredictionRequest = int(self.config.get('tracker_prediction_error_resend_sec','15'))  # Failed to get a good response Resend in 15 sec.
        if (self.resendPredictionRequest > 90 or self.resendPredictionRequest < 1 ):
            self.resendPredictionRequest = 15   # set default

        logger.info( "request Prediction timout in seconds = " + str(self.predictionTimeout))
        self.predictionTimer = Timer(self.predictionTimeout, self.predictionRequest)
        logger.info(self.config.get('tracker_file','tracker.txt'))
        self.pingAddr = self.config.get('tracker_ping_addr','ctabustracker.com')
        
        # Initialize file list.
        self.generatorPath = self.config.get('tracker_generator_path','./generatorXml')
        self.generatorNdx = 0           # set current ndx in list
        self.generatorFileCount = 0     # init file count
        self.generatorFileList = []
        for root,dir,files in os.walk(self.generatorPath):
            for file in files:    
                if file.endswith('.xml'):
                    self.generatorFileCount += 1    # increment file count
                    self.generatorFileList.append(self.generatorPath+"/"+file) 
        
        print ("filelist count=" + str(self.generatorFileCount) +"\n")
        print ("filelist=" + str(self.generatorFileList) +"\n")
        
    """
    ************************************************
    Method:  def ping(self):
    """
    def ping(self):

        x = 'ping CTA\n\t '+'ping -W6 -c3 -l2 '+self.pingAddr
        logger.info ( x )
        ret = -1
        try:
            self.ping_p = subprocess.Popen(['ping -W6 -c3 -l2 '+self.pingAddr],shell=True)
                            #shell=True,
                            #stdin=subprocess.STDIN,
                            #stdout=open('ping.txt','w'),
                            #stderr=subprocess.STDOUT)
                            
            ret = os.waitpid(self.ping_p.pid, 0)[1]

        except IOError as (errno, strerror):
            logger.info( "PING I/O error({0}): {1}".format(errno, strerror))
            ret=-1
        except ValueError:
            logger.info( "PING Value Error ?.")
            ret=-1
        except OSError, e:
            logger.info( "Execution failed:", e)
            ret=-1
        except:
            logger.info( "PING Unexpected error:", sys.exc_info()[0])
            ret=-1

        if ret == 0:
            logger.info( "%s: is alive" % self.pingAddr)
        else:
            logger.info( "%s: did not respond" % self.pingAddr)
        
        self.ping_p = None
        return ret


    """
    ************************************************
    Method:  def pinging(self,till_success=0):
        - ping Once Default.
        - ping Till Success Response, if till_success!=false
    """
    def pinging(self,till_success=0):
        ret = 0
        if (till_success != 0):
            # Returns 0 on ping is Alive, so we quit
            # If != 0 no response sleep 5sec and try again.
            # Multitech Modem takes upto 20-30sec to bring up link.
            ret = self.ping()
            while(ret != 0):
                ret = self.ping()
                time.sleep(5)

            return ret
        else:
            return self.ping()
    

    """
    ************************************************
    Method:  def getSigndata(self):
    """
    def getSigndata(self):
        return self.mysign
    
         
    """
    ************************************************
    Method:  def startTimerPredictions(self, timeOverride=0):
        Start a timer to make a Request for bus predictions.
    """
    def startTimerPredictions(self, timeOverride=0):
        logger.info( "\nstart Timer Prediction try:")
        
        timeout=self.predictionTimeout
        if (timeOverride > 0) :
            timeout = timeOverride
        self.predictionTimer = None
        self.predictionTimer = Timer(timeout, self.predictionRequest)
        try:
            self.predictionTimer.start()
        except: 
            raise

        
    """
    ************************************************
    Method:  def stopTimerPredictions(self):
        Stop the timer
         however if the url from the urllib2.open has gone out will hang till finished.
            not sure how to stop/pre-empt that request 
    """
    def stopTimerPredictions(self):
        self.predictionTimer.cancel()


    """
    ************************************************
    Method:  def postUpdate(self) :
        Send a udp message to audio task
        Send a udp message to display task
    """
    def postUpdate(self) :
        #UDPSock = socket(AF_INET,SOCK_DGRAM)
        self.UDPSockAudio.sendto(self.mysign.toString(),    self.addrAudio)
        self.UDPSock.sendto(self.mysign.toString(),         self.addr)
    

    """
    ************************************************
    Method: formatSignData (self , *args) :
        wake-up reader
        buses[0] Example XML
            <tmstmp>20110421 17:20</tmstmp>
            <typ>A</typ>
            <stpnm>Wacker (Upper) &amp; Columbus</stpnm>
            <stpid>14222</stpid>
            <vid>1716</vid>
            <dstp>4276</dstp>
            <rt>122</rt>
            <rtdir>South Bound</rtdir>
            <des>Ogilvie Station</des>
            <prdtm>20110421 17:28</prdtm>

         for clarity two loops.
           - Write an ascii Audio file for TTS  'arrival.txt'
           - Update 'C' Struct for Display controller 
            stop + dir
            date
            time
            temp
            [
             rt + destname + minutes
            ]0..3
    """
    
    def formatSignData (self , *args) :
        #print 'formatSignData(args)', args, "\n"

        #print self, len(args)
        busPrediction_objects = args[0]
        #print "bps", busPrediction_objects, "bp len=", len(busPrediction_objects), "\n"
        
        busPrediction_objects = sorted( busPrediction_objects, key=lambda busPrediction: busPrediction.prdtm )

        #print "bps2", busPrediction_objects, "bp len=", len(busPrediction_objects), "\n"

        # Write Display File
        if (len(self.config.get('tracker_file','tracker.txt')) > 0 ) :
            self.mysign = Signdata()
            
            #with open(self.config['tracker_file'], "wb") as fp :
            flag = 1
            i = 0
            for mybus in busPrediction_objects :
                #for bus in mylist : 
                # compute Time in Minutes
                timestamp = mybus.tmstmp
                curdatetime = datetime.strptime(timestamp,"%Y%m%d %H:%M")
                curtime = time.strptime(timestamp,"%Y%m%d %H:%M")
                ampm = curdatetime.strftime("%p")
                timestamp = mybus.prdtm
                predtime = time.strptime(timestamp,"%Y%m%d %H:%M")
                difftime = time.mktime(predtime) - time.mktime(curtime)


                if (flag):
                    mydate = curdatetime.strftime("%m/%d")
                    mytime = ("%s%s" %(curdatetime.strftime("%I:%M"),ampm[0]))
                        
                    self.mysign.setStop("%s (%s. BOUND)" % ( mybus.stpnm.upper() , mybus.rtdir[0].upper()))
                    self.mysign.setDate(string.lstrip(mydate,'0'))
                    self.mysign.setTime(string.lstrip(mytime,'0'))
                    self.mysign.setTemp("75F")
                    flag = 0
                    logger.info( self.mysign.getStop())
                    logger.info( self.mysign.getDate())
                    logger.info( self.mysign.getTime())
                    logger.info( self.mysign.getTemp())

                bus = "%-3s %-26s %2sM" %(mybus.rt,mybus.des.upper(), string.lstrip(str(int(difftime/60)),'0' ))
                logger.info( bus)
                route = RouteDestination()
                route.setRoute(mybus.rt)
                route.setDestname(mybus.des.upper())
                route.setMinutes(string.lstrip(("%02dM" % int(difftime/60)),'0'))
                self.mysign.setBus(i,route)

                # Only Three Bus entries
                i= i + 1
                if (i>2):
                    break;

            self.mysign.writeSigndata(self.config.get('tracker_file','tracker.txt'))
        
        """    
         Write Audio Messages File.
            Todo: need/ a semephore to coordinate with audio. 
                - pend the audio task till file finished.
        """
        audiofile = self.config.get('audio_file','arrival.txt')   # if cant find set default.
        
        logger.info( "configfile = " + audiofile + "len(audiofile)=" + str( len(audiofile)))
        if ( len(audiofile) > 0 ) :
            busPrediction_objects = args[0]
            busPrediction_objects = sorted( busPrediction_objects, key=lambda busPrediction: busPrediction.prdtm )
            
            with open(audiofile, "wb") as fp :
                flag = 1
                i = 0
                curdatetime = 0

                for mybus in busPrediction_objects :
                    #for bus in mylist : 
                    # compute Time in Minutes
                    timestamp = mybus.tmstmp
                    curdatetime = datetime.strptime(timestamp,"%Y%m%d %H:%M")
                    curtime = time.strptime(timestamp,"%Y%m%d %H:%M")
                    timestamp = mybus.prdtm
                    predtime = time.strptime(timestamp,"%Y%m%d %H:%M")
                    difftime = time.mktime(predtime) - time.mktime(curtime)
    
                    if (flag):
                        fp.write("BusStop "+ mybus.stpnm + ".  "+ mybus.rtdir + ".\n") 
                        flag = 0

                    fp.write("Next arrival, " + str(int((difftime/60))) + " minutes for route " + mybus.rt + " and destination " + mybus.des + ".   \n")
                    # Only Three Bus entries
                    i= i + 1
                    if (i>2):
                        break;
    
                try:
                    fp.write("The date is " + curdatetime.strftime("%A %B %d %Y.\nTime is %I:%M %p.\n") )
                except:
                    pass
                    
                fp.close()

    """
    ************************************************
    Method: def predictionRequest(self):
    """
    def predictionRequest(self):
        logger.info( "prediction request..." + str( datetime.today()))
        
#        """ !!! PING could spin forever!!!
#            - wakeup try to connect to internet through multitech modem ppp
#        """
#        if (self.pinging(1) != 0):
#            time.sleep(2)
#        
#        req = urllib2.Request(self.predictionUrl)
#        response = ""
#
#        try:
#            response = urllib2.urlopen(req, None, timeout=3)
#
#        except urllib2.URLError, e:
#            logger.info( "Http request failed reason=",e.reason)
#            
#            # Display Unavailable.
#            i = 0
#            for mybus in self.getSigndata() .getBuses() :
#                logger.info( i, mybus)
#                route = RouteDestination()
#                route.setRoute("*")
#                route.setDestname(self.config.get('tracker_unavailable','Status Unavailable'))
#                route.setMinutes("?? MIN")
#                self.mysign.setBus(i,route)
#                i = i+1
#
#            logger.debug( "tostring = ", self.mysign.toString(), "\n")                    
#            self.postUpdate()
#            self.startTimerPredictions(self.resendPredictionRequest)  # Failed to get a good response Resend in 15 sec.
#            
#        else:
#            
#            try:
#                xml = response.read()
#            except Exception as e:
#                logger.info(" Error response.read() failed. Likely socket timeout.")
#                # re-try in 15 secs 
#                self.startTimerPredictions(15)
#                return
#                
#                
#
         # Get current file
        xmlfile = self.generatorFileList[self.generatorNdx]
        print ("xmlfile=" + xmlfile + "\n")
        
        # Increment the gen. index
        print self.generatorFileCount
        if (self.generatorFileCount > 0):
            self.generatorNdx = (self.generatorNdx + 1) % self.generatorFileCount
            
        with open (xmlfile) as xmlfp:
            xml = xmlfp.read()
        xmlfp.close();

        # Dump Last XML recived
        with open("/tmp/lastprediction.xml", "w+b") as xmlfp :
            xmlfp.write(xml)
        xmlfp.close()

        print
        print ("ndx = " + str(self.generatorNdx) + "\n")
        

                        
        try:
            dom = minidom.parseString(xml)
            predictions = dom.getElementsByTagName("prd")
        except Exception as xmle:
            logger.info( " Error xml parse exception. " + str( datetime.today()))
            logger.info( " Error xml = \n" + str(xml))

            # re-try in 15 secs 
            self.startTimerPredictions(15)
            return
             

                    
        busPrediction_objects = []
        pdict = dict()
        
        for aPred in predictions :
            cnt = 0
            for pElem in aPred.childNodes :
                if (pElem.nodeType == Node.ELEMENT_NODE ) :
                    #print cnt, pElem.toxml(), pElem.childNodes
                    cnt += 1
                    for pValue in pElem.childNodes :
                       pdict[pElem.nodeName] = pValue.nodeValue
                       #print pElem.nodeName, pValue.nodeValue
    
            bus  = BusPrediction(pdict['tmstmp'],pdict['typ'],pdict['stpnm'],pdict['stpid'],pdict['vid'],pdict['dstp'],pdict['rt'],pdict['rtdir'],pdict['des'],pdict['prdtm'])
            busPrediction_objects.append(bus)

            pdict = None
            pdict = dict()

        """
        Key piece here. 
           - format data and update structure to Display, Audio tasks.
           - post updates
           - restart timer
        """
        logger.info( "\nBuses: " + str((len(busPrediction_objects))))
        self.formatSignData (busPrediction_objects)
        logger.info( "tostring = " + self.mysign.toString().upper() + "\n")                    
        self.postUpdate()
        self.startTimerPredictions()

    """
    ************************************************
    Method: def predictionRequest(self):
    """
    def bulletinRequest(self):
        logger.info( "bulletin request..." + str( datetime.today()))

    """
    ************************************************
    Method: def readconfig(self):
    """
    def exitall():
        logger.info( "tracker Exit All()"," self.ping_p=",self.ping_p,"\n")
        
        if (self.ping_p):
            self.ping_p.kill()
            self.ping_p = none
        t.stopTimerPredictions()

    """
    ************************************************
    Method: def readconfig(self):
    """
    def readconfig(self):

        # Read config
        trackerpath = ""
        logger.info( "os.env="+ str(os.environ.keys()) + "\n")
        self.applpath = '/ctss/cfg/gentracker.conf'
        
        for i in os.environ.keys():
          if (i == "LUM_TRACKER_HOME"):
                trackerpath = os.environ['LUM_TRACKER_HOME']

        if (len(trackerpath) > 0):
            self.applpath = trackerpath
        logger.info( "applpath=" + str(self.applpath))
        
        self.configFd = open(self.applpath)
        config = dict()
        for line in self.configFd:
            if (line[0] == '#' or line[0] == "\n" ):
                #print 'skip comment/blank line=' + line
                continue
            l = line.split("\n")
            keyvalue = l[0].split(" ")
            if ( len(keyvalue) >= 2):
                self.config[keyvalue[0]] = ' '.join(keyvalue[1:]) 
            elif ( len(keyvalue) == 1):
                self.config[keyvalue[0]] = '' 
            else:
                logger.info( "error reading config len=" + len(keyvalue)+ " line=" + line)
    
# end class GenTracker()






"""
************************************************
# Main routine
  - create a Tracker Object
  - start prediction timer
  - spin wait for keys

"""
# ..create timer thread
t = GenTracker()
t.startTimerPredictions(1)
# Send messages
while (1):

    try:
        if ( len(sys.argv) > 1 and sys.argv[1] == "input"):
            data = raw_input('>> ')
            if not data :
                #  UDPSock.sendto("q",addr)
                break
            else :
                #  if(UDPSock.sendto(data,addr)):
                if (data == "h"):
                    print "h Help"
                    print "r Re-read config file"
                    print "q Quit to exit"
                    continue
                
                elif ( data == "q"):
                    print "quit..."
                    break
                elif ( data == "r" ):
                    print ("reread config")
                    if (t.ping_p != None):
                        t.ping_p.kill()
                    t.stopTimerPredictions()
                    t = None
                    t = GenTracker()
                    t.startTimerPredictions(5)
                    continue
                    
                print "Message = '",data,"'....."
        else:
            time.sleep(1)

    except (KeyboardInterrupt, SystemExit):
        logger.warning ('KeyboardInterrupt')
        if (t.ping_p != None):
            t.ping_p.kill()  #t.exitall()
        t.stopTimerPredictions()
        sys.exit()
        raise



if (t.ping_p != None):
    t.ping_p.kill()  #t.exitall()
t.stopTimerPredictions()

print "Exit busapi.py"
sys.exit()









