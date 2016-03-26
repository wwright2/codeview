"""
************************************************
Copyright: copyright Luminator 2011 all rights reserved.

project: ctss - chicago transit shelter sign

module: tracker

file: tracker.py

history:
    wwright 4/21/2011    modularize

annotation:

  Generalization

   ..read config
        ..get stopid
        ..build url
    ..initSocket()
    ..create msg Timers thread 

    ..MessageRequests() 
        ..send request.
        ..get response
        ..parse response
        ..wake up displayController w/ UDP message
        ..Write Audio file. udp Audio to copy file.


  Classes:
    CtssMessageTypes -  Enum  syncronize C code with ../common/trackerconf.h
    CtssErrorCodes -  Enum
    
    BusPrediction -
       - Sort Arrival data.
    
    Tracker - Work Horse.  
       - create messages
       - initialize data 
       - process Status
       - process failures start timers to send messages to cta discover Api

      Messages:
          __init__
          getMyTZoffset()
          setDateTime()
          execVolumeRule()
          readConfig()
          

        -ctaMsg-
          initStatus() - we have recieved a initConfig message 
          arrivalStatus() - digest an send to Display and Audio task
          alertIdStatus() - digest determine if we need to request Alerts
          alertStatus()   - digest and send Alerts data to Display and Audio.
          

          generalFailure() - request URL failed.
          
          initFailure()
          arrivalFailure()
          alertIdFailure()
          alertFailure()
          
          sendInit() - send to Display and Audio as needed
          sendArrivals() -
          sendAlerts() -
          sendAudioAlerts() - 
          
        
Todo:

************************************************
"""
import sys, os, string
import socket, urllib2
import time
import calendar

from ctssLogger import CtssLogger

from datetime import datetime
from xml.dom import minidom, Node
from socket import *
from threading import Thread, Timer

from Queue import Queue
import syslog
import subprocess

# from commSupport import CommSupport
from ctaMsgInitData import *
from ctaMsgArrivals import *
from ctaMsgAlertData import *
from ctaMsgAlertId import *


class CtssMessageTypes:
    BusArrival = 1              # upto 8 arrivals sorted by arrival time. >rt1 destination 1m\nrt2 destxyz 8m\n<
    BusAlert   = 2                # alerts >11111111111111\n222222222222222\n333333333333\n< sorted by severity
    BusError   = 3                # error messages >my error message\n<  either no arrivals scheduled, url timeout, modem problem, etc.
    BusConfig  = 4
    BusBlankTheScreen = 5

class CtssErrorCodes:
    # three digit code.
    GeneralFailure = "005"



    
"""
************************************************
class BusPrediction(object)
        - sort the bus predictions objects.
"""
class BusPrediction(object) :
    def __init__(self, *args) :
        #BusPrediction(pdict['speech'],pdict['id'],pdict['route'],pdict['direction'],
        #  pdict['destination'],pdict['minsToArrival'],pdict['fault'],
        #pdict['scheduled'],pdict['delayed'])

        if (len(args) == 9 ) :
            self.speech      = args[0]
            self.id          = args[1]
            self.route       = args[2]
            self.direction   = args[3]
            self.destination = args[4]
            self.minsToArrival   = args[5]
            self.fault       = args[6]
            self.scheduled   = args[7]
            self.delayed     = args[8]
        else :
            speech=''; id=''; route=''; direction=''; destination=''; minsToArrival=''; fault=''; scheduled=''; delayed='';
        
    def __repr__(self):
        return repr((self.speech, self.id, self.route, self.direction, self.destination, self.minsToArrival, self.fault, self.scheduled, self.delayed))

    def sortByArrival(*args):
        busPrediction_objects = args
        return sorted( busPrediction_objects, key=lambda busPrediction: busPrediction.minsToArrival )

        

"""
************************************************
class Tracker(object)
        init
            get env
            get conf
            Poll the BusAPI for Bus Predictions
            Parse XML save to file for 'C' bustracker to read and post to display.

    predictionkeys = 'tmstmp','typ','stpnm','stpid','vid','dstp','rt','rtdir','des','prdtm'
            
"""
class Tracker(CtssLogger) :

    config = dict()
    lumSerialNumber = 'DUMMY'

    
    def __del__(self):
        self.UDPSock.close()
        self.UDPSockAudio.close()
        self.initConfig.stop()
        self.arrival.stop()
        self.alertId.stop()

        self.fp.flush()
        self.fp.close()
       
#    def log(self,args):
#        super(Tracker,self).log(args)


    def __getSerial(self):
        serialnumfile = self.config.get('tracker_serial_num_file', '/config/serial-number')
        serialnum=self.lumSerialNumber # init.
        self.log( "getSerial file = " + str(serialnumfile))
        fd = open(serialnumfile)
        for line in fd:
            if (line[0] == '#' or line[0] == "\n" ):
                #print 'skip comment/blank line=' + line
                continue
            
            self.log( line)
            seriallist = line.split("\n")
            serialnum = str(seriallist[0])
            self.log( "serialnum=" + str(serialnum))
            if (len(serialnum) > 0) :
                self.lumSerialNumber = serialnum
                break  
        return serialnum


        
    """
    ************************************************
    Method:  def __init__(self) :
    """
    def __init__(self) :
        CtssLogger.__init__(self)
        self.readconfig()
        self.ping_p = None
        
        self.alertIds_objects = []
        self.prevAlertIds_objects = []
        self.latestAlerts = []
        
        self.latestArrivals = []
        self.numPrevArrivals = 0

        self.lumSerialNumber = self.__getSerial() # read serial number file.
        print "lumSerialNumber={0}\n".format(self.lumSerialNumber)
        self.log("serialnum = " + self.lumSerialNumber)
        
        # Create socket
        self.UDPSock = socket(AF_INET,SOCK_DGRAM)
        self.UDPSockAudio = socket(AF_INET,SOCK_DGRAM)
        self.addr = (self.config.get('tracker_display_ip','127.0.0.1'), int(self.config.get('tracker_display_port','15545')))
        self.addrAudio= ( self.config.get('tracker_audio_ip','127.0.0.1'), int(self.config.get('tracker_audio_port','15546')) )
        self.log ("tracker sock display =" + str(self.addr))
        self.log( "tracker sock audio =" + str(self.addrAudio))        

        self.initConfig = CtaMsgInitData(self.config, self.lumSerialNumber, self.initStatus, self.initFailure)
        self.arrival    = CtaMsgArrivals(self.config, self.lumSerialNumber, self.arrivalStatus, self.arrivalFailure)
        self.alertId    = CtaMsgAlertId(self.config, self.lumSerialNumber, self.alertIdStatus, self.alertIdFailure)
        self.alertData  = CtaMsgAlertData(self.config, self.lumSerialNumber, self.alertStatus, self.alertFailure)
        
        self.curVolume = 10
        self.dayVolume = 5
        self.nightVolume = 5
        self.daymins = 0
        self.nightmins = 0
        self.errorNoServerResponse = self.config.get('error_arrival_unavailable_msg', 'Arrival information temporarily unavailable.')
        
        #  -- - - --
        # Config file is 1..8
        # Zero base number, so Subtract 1 
        self.maxArrivals = int(self.config.get('tracker_maxarrivals','8'))
        #LIMIT to 0..7
        if (self.maxArrivals >= 8):
            self.maxArrivals = 7
        elif (self.maxArrivals <= 0):
            self.maxArrivals = 0
        else:
            self.maxArrivals = self.maxArrivals - 1

        # Setup Volume Control
        self.tracker_setvol = int(self.config.get('tracker_setvol_timer_mins',5))
        self.execVolumeRule()
        self.log("Setup Volume Day/Night Timer...{0}".format(60*self.tracker_setvol))


    """
    ##  def getMyTZoffset(self):
    #
    """
    def getMyTZoffset(self, now):
        #  Starting in 2007, most of the United States and Canada observe DST from the 
        #  -->second Sunday in March to the first Sunday in November<--

        offset = 0
        
        f = open("/etc/TZ","r")
        for i in f : 
            self.log("TZ={0}".format(i))
            (myst,Hours,mydt)="{0},{1},{2}".format(i[0:3],i[3],i[4:7]).split(",")
        f.close()

        try: 
            offset = int(Hours)
        except :
            self.log("API InitData: Exception while trying to convert Hours={0}. Setting to default 6.".format(Hours))
            offset = 6

        if (offset > 10 or offset < 0):
            offset = 6  #default to chicago
        
        # For now rough estimate for DST Apr-Oct.  Below we outline an algo for more exact.
        year   =  now[0]
        month  =  now[1]
        if ( month > 3 and month < 11):
            offset = offset - 1
        else:
            offset = offset
            
        return offset
    

        #### Calculate a More accurat DAYLIGHT savings time.
        #cal =  calendar.Calendar(calendar.MONDAY)  # 0..6 = Monday..Sunday
        ## find startDT = second sunday of Mar = stDoy of year 0..365
        #     get calendar YEAR,MARCH, find second sunday, day of month. convert to day of Year.
        
        ## find endDT = first Sunday of Nov = endDay  of year 0..365
        #     get calendar YEAR,NOVEMBER, find first sunday, day of month, convert to day of year.
        ## get todays day of year
        ## if doy > stday and doy < enDoy
        ##    daylight savings time
        ## else
        ##    Standard time
        
    
    """
    ************************************************
    Method:  def setDateTime(self):
                            <year>2011</year>
                            <month>8</month>
                            <day>24</day>
                            <hour>4</hour>
                            <minute>59</minute>
                            <second>25</second>
                            <meridiem>PM</meridiem>
    """        
    def setDateTime (self):
        offset = 0
        hours12 = 0
        
        if (self.initConfig.servertime['meridiem'] == 'PM'):
            hours12 = 12

        hour = int(self.initConfig.servertime['hour'])
        
        # if 12AM
        if (hours12 == 0 and hour == 12):
            hour = 0
        # if 12pm
        if (hours12 == 12 and hour < 12):
            hour = hour + hours12
        
        t=(int(self.initConfig.servertime['year']),
           int(self.initConfig.servertime['month']),
           int(self.initConfig.servertime['day']),
           hour,
           int(self.initConfig.servertime['minute']),
           int(self.initConfig.servertime['second']),
           0,0,-1)
        
        for i in self.initConfig.servertime:
            self.log( "Servertime. key={0}, value={1}".format(i,self.initConfig.servertime[i]))
        
        self.log("setDateTime() hour={0}".format(hour))    
        self.log("   mktime(t) t={0}".format(t))
        
        # check the Calendar
        # UTC = chicago + 6cst or +5cdt hours
            
        offset = self.getMyTZoffset(t)
        mt = time.mktime(t) #+ (60* 60 * int(offset) )
        self.log ("t={0}, mt={1}".format(t, mt))
        t = time.localtime(mt)
        self.log( "adjusted t={0},hour={1}".format(t,t[3]))
        
        tstr = "{0}-{1}-{2} {3}:{4}:{5}".format(self.initConfig.servertime['year'],
                                         self.initConfig.servertime['month'],
                                         self.initConfig.servertime['day'],
                                         self.initConfig.servertime['hour'],
                                         self.initConfig.servertime['minute'],
                                         self.initConfig.servertime['second']
                                         )
        # Call Bash script to call "date" and "hwclock"
        retcode = subprocess.call(["/ctss/bin/setdatetime.sh", tstr])
        
    
    
    """
    ************************************************
    Method:  def execVolumeRule(self):
    """
    def execVolumeRule(self):

        t = time.localtime()
        h=t[3]
        m=t[4]
        s=t[5]
        curmins = 1 if (s>35) else 0;
        curmins = curmins + h*60 + m
        volume = 0

        self.log ("ExecVolumeRule() daymins={0}, nightmins={1}, dv={2},nv={3}".format(self.daymins,self.nightmins,self.dayVolume,self.nightVolume))
        
        # IF DEFINED, execute volume rules.
        if ((self.daymins != None) and (self.nightmins != None) 
            and (self.dayVolume != None) and (self.nightVolume != None) ):

            self.log("execVolume() curmins={}".format(curmins))
            if (curmins > self.daymins and curmins < self.nightmins ):
                volume = self.dayVolume
            else:
                volume = self.nightVolume
            
            self.log("execVolume() curVolume={}".format(volume))
            if (self.curVolume != volume):
                self.curVolume = volume
                try: 
                    self.log("Setting the Volume(1..10)=(11..29)={0}".format(volume))
                    subprocess.call(["/ctss/bin/unmute.sh", str(volume)])  # Writes to /ctss/cfg/volume.settings
                except :
                    self.log ("Setting the volume failed.")
                    pass
        
        # restart timer.
        self.volumeTimer = None
        self.volumeTimer = Timer(60*self.tracker_setvol, self.execVolumeRule)
        self.volumeTimer.start()
    
    """
    ************************************************
    Method:  def strpmins(self):
        convert string hh:mm:ss to mins;  :35+ sec => +min
        - timestring='hh:mm:ss"
        
    """
    def strpmins(self,timestring):
        
        minutes = 0
        t = timestring.split(':',3)
        smin = 1 if ( int(t[2]) > 35) else 0
        minutes = (int(t[0])*60) + int(t[1]) + smin  
        return minutes
    

    """
    ************************************************
    Method:  def rangeAdjustVolume(self):
        input 1..10
        output 11..29
    """
    def rangeAdjustVolume(self,volume):
        x=0
        v=volume
        if (volume > 10):
            v = 10
        elif (volume < 0):
            v = 1
        
        v = int(9 + (2* v))    # sequence 11..29
        return int(v)
        
        
    """
    ************************************************
    Method:      def requestConfig(self):
        We recieved an SIGINT from busapi and we want to force a request InitConfig from CTA Server 
        Stop config Timer and send initConfig request in delay=? seconds.
    """
    def requestConfig(self):
        self.initConfig.stop()
        self.initConfig.start(delay=2)
                    
    """
    ************************************************
    Method:  def initStatus(self):
    """
    def initStatus(self, *args): 
        
        self.log( "InitStatus Returned. success={0}".format(self.initConfig.success))
        
        
        self.daymins     = self.strpmins(self.initConfig.settings['day_time'])
        self.nightmins   = self.strpmins(self.initConfig.settings['night_time'])
        self.dayVolume   = self.rangeAdjustVolume(int(self.initConfig.settings['day_volume']))
        self.nightVolume = self.rangeAdjustVolume(int(self.initConfig.settings['night_volume']))

        self.log ("InitStatus process: dm={0},nm={1},dayVolume={2},{3},nightV={4}{5}".format( self.initConfig.settings['day_time'],
                                                                              self.initConfig.settings['night_time'],
                                                                              self.initConfig.settings['day_volume'],
                                                                              self.dayVolume,
                                                                              self.initConfig.settings['night_volume'],
                                                                              self.nightVolume
                                                                              ))
        self.setDateTime()
        
        # Set the time first then restart the timers.
        # We wait for a sucessful InitConfig before sending first Arrival request.
        if (self.initConfig.success == True):

            #if ( isinstance(self.volumeTimer,Timer) ):
            try:
                if (self.volumeTimer != None):
                    self.volumeTimer.cancel()
            except: 
                self.log("Error while trying to reset day/night volume timer")
                
            self.execVolumeRule()
            self.log("Setup Volume Day/Night Timer...{0}".format(60*self.tracker_setvol))

            # Let's restart the AlertId, and Arrival requests due to possible Time Update changes.
            self.log("INIT CONFIG UPDATED len(settings)={0}".format(len(self.initConfig.settings)))
            url = self.initConfig.settings['discover_url']
            self.log("initConfig. discover_url={0}".format(url))
            if (len(url) > 20):
                self.log("update urls")
                self.initConfig.urlUpdate(url)
                self.arrival.urlUpdate(url)
                self.alertId.urlUpdate(url)
                self.alertData.urlUpdate(url)
            
            try :
                arrival_poll = int(self.initConfig.settings['arrival_poll_sec'])
                alertid_poll = int(self.initConfig.settings['alertid_poll_sec'])
                initConfig_poll = int(self.initConfig.settings['initconfig_poll_sec'])

                self.log("arrival_poll={0} alertid_poll={1} initConfig_poll={2}".format(arrival_poll,alertid_poll,initConfig_poll))            
                if (arrival_poll): 
                    self.arrival.updateTimeout(arrival_poll)
                if (alertid_poll): 
                    self.alertId.updateTimeout(alertid_poll)
                if (initConfig_poll): 
                    self.initConfig.updateTimeout(initConfig_poll)

            except:
                self.log("Poll time update failed, due to Integer conversion.")
                
                    
            self.arrival.stop()
            self.alertId.stop()
            self.initConfig.stop()   # ...This will cancel all subsequent calls unless start() is called.
            time.sleep(1)
            try:
                self.initConfig.start()  # ...restart the timer to take new values. i.e. url & timeout.
                self.arrival.start(delay=10)
                self.alertId.start(delay=25)
            except :
                self.log ("Error Occurred starting Message Timers, verify Config parms") 

            self.log("INIT CONFIG UPDATED len(settings)={0}".format(len(self.initConfig.settings)))
            
        else:
            self.log ("init status failed")

        
        initData = "" + chr(CtssMessageTypes.BusConfig)
        for b in self.initConfig.settings.keys() : 
             initData = initData + "{0} {1}\n".format(b,self.initConfig.settings[b])
             
        for b in self.initConfig.servertime.keys() :
             initData = initData + "{0} {1}\n".format(b,self.initConfig.servertime[b])

        self.log(initData)
        self.sendInit(initData)


    """
    ************************************************
    Method:  def sendInit(self):
        send the init -> ButtonAudio, Display
    """
    def sendInit(self, mydata):
        self.UDPSockAudio.sendto(mydata,    self.addrAudio)
        self.UDPSock.sendto(mydata,         self.addr)

    
    """
    ************************************************
    Method:  def formatArrivals(self):
            self.speech      
            self.id          
            self.route       
            self.direction   
            self.destination 
            self.minsToArrival
            self.fault       
            self.scheduled   
            self.delayed     
    """
    def formatArrivals(self):
        line = ""
        self.displayData = "" + chr(CtssMessageTypes.BusArrival)

        self.log("number of latestArrivals={0}".format(len(self.latestArrivals)))
        cnt = 0
        for i in self.latestArrivals :
            if (cnt > self.maxArrivals):
                break
#            # EMPTY _ treat as General Error
#            if ((i.id == 0) and (i.destination==" ") and (i.route == " ")) :
#                self.log("Send Arrival Blank")
#                self.displayData = self.displayData + ".    .                           .\n"
                
            # Error Tag found, No Eta's
            if ((i.id == 0) and (i.route == " ")) :
                self.log("Send Arrival Error")
                line = "%(route)1s %(destination)1s %(mins)1s\n" % {"route":"\t ", "destination":i.destination, "mins":" \t"}
                self.displayData = self.displayData + line
            # Normal Arrival
            else:
                # Else
                if (int(i.minsToArrival) < 2):
                    line = "%(route)1s %(destination)1s %(mins)1s\n" % {"route": i.route, "destination":i.destination, "mins":"Due"}
                else:
                    line = "%(route)1s %(destination)1s %(mins)1sm\n" % {"route": i.route, "destination":i.destination, "mins":i.minsToArrival} 
                self.displayData = self.displayData + line
            cnt += 1

        if (cnt == 0):
            pass
            


        print ("msgtype:{0:#x}; data:{1:s}\n".format(ord(self.displayData[0]),self.displayData[1:]))
        self.log ("msgtype:{0:#x}; data:{1:s}".format(ord(self.displayData[0]),self.displayData[1:]))
        
#        lst = []
#        for c in self.displayData:
#            lst.append (ord(c))
#        self.log ("{0}".format(lst))

    
    """
    ************************************************
    Method:  def formatAudioArrivals(self):
        Expects sorted data in latestArrivals.
    """
    def formatAudioArrivals(self):
        line = ""
        self.audioData = "" + chr(CtssMessageTypes.BusArrival)
        cnt = 0
        for i in self.latestArrivals :
            if (cnt > self.maxArrivals):
                break
            # EMPTY
            if ((i.id==0) and (i.route == " ")) :
                # www-2012Jan5 was 'Have a nice day.' Changed by request.
                #   added config value errorNoServerResponse.
                self.audioData = self.audioData + self.errorNoServerResponse
                break;
            else:
                #self.audioData = self.audioData + i.speech.replace("&amp;","&") + "\n"
                self.audioData = self.audioData + i.speech + "\n"
            cnt += 1

        # ww version 1.0.13+  TEST If Alerts append message else dont append
        if ( len(self.latestAlerts) > 0) :
            self.audioData = self.audioData + " Press button again for alerts.\n"
        else:
            self.audioData = self.audioData 
        # ww version 1.0.13 -
        
        self.log ("Audio: msgtype:{0:#x}; data:{1:s}".format(ord(self.displayData[0]),self.audioData[1:]))
        

        
    """
    ************************************************
    Method:  def sendArrivals(self):
        send info to bustracker-audio-tts
        send info to display
    """
    def sendArrivals(self):
        self.formatArrivals()
        self.formatAudioArrivals()
        self.UDPSockAudio.sendto(self.audioData,    self.addrAudio)
        self.UDPSock.sendto(self.displayData,       self.addr)
        
        #self.UDPSock.sendto(self.mysign.toString(),         self.addr)

    """
    ************************************************
    Method:  def arrivalStatus(self):
    """
    def arrivalStatus(self,*args):

        # ..new
        self.latestArrivals = args[0]
        numArrivals =len(self.latestArrivals)
        if (numArrivals > 0):
            self.latestArrivals = sorted( self.latestArrivals, key=lambda etaPrediction: etaPrediction.minsToArrival )

#            for i in self.latestArrivals :
#                self.log(i.speech)
#                self.log("{0}m for route {1}, destination {2}".format(i.minsToArrival, i.route, i.destination))
#                print "{0}m for route {1}, destination {2}".format(i.minsToArrival, i.route, i.destination)

            self.sendArrivals()
            self.numPrevArrivals = numArrivals
            
        elif (numArrivals == 0 and self.numPrevArrivals > 0) :
            pass
        
        #print "\n" +  str( datetime.today()) +"\n"
        # ..send to Display
        # ..send to Audio
        
    
                
    """
    ************************************************
    Method:    def alertsClearDisplay(self):
        
    """
    def alertsClearDisplay(self):
        self.log("alertsClearDisplay...")
        self.alertData.urlAppend(parms=" ")
        self.alertData.start()
        self.prevAlertIds_objects = []  # clear prevAlertIds.

        
    """
    ************************************************
    Method:  def alertIdStatus(self):
    
        - Alert Id request has returned, process id's List.
        .. if GUID diff for any existing ID, retrieve, add to urlAppend()
        .. if id's List is diff from prev id's List of alarmId's, note 
           .. the position in the list of the change and use for 
           .. formatting AlarmsMsg to Display

        self.id   = id
        self.guid = guid
        self.error= error
        
    """
    def alertIdStatus(self,*args):
        self.log("AlertId status")
        # ..new
        alertIds_objects = args[0]
        numAlerts = len(alertIds_objects)
        numPrevAlerts = len(self.prevAlertIds_objects)

        self.log("AlertId: num alerts={0}, num prev={1}".format(numAlerts,numPrevAlerts))

        #Alerts NOT 0
        if (numAlerts>0):
            self.log("AlertId: num>0, sort them")
            alertIds_objects = sorted( alertIds_objects, key=lambda alertIds: int(alertIds.id), reverse=True )
#            for i in alertIds_objects :
#                self.log ("AlertId: {0} with guid={1}".format(i.id, i.guid))
#                print "alert {0} with guid={1}".format(i.id, i.guid)

            """
            Process Alert IDS
            ...for newAlert in currentAlerts:
                for newAlert in previousAlerts:
                    if newAlert.guid == previousAlerts[newAlert.id]
                ????
            """

            """
            Go get Data
            """
            
            # Defaults.
            changeNdx = 0
            tmpAlerts = self.prevAlertIds_objects
            if (numPrevAlerts > 0):
                self.log("AlertId: cycle through prev,cur objects")
                for newAlert in alertIds_objects:
                    notFound = False
                    for oldAlert in tmpAlerts:
                        if ( str(oldAlert.guid.strip()) == str(newAlert.guid.strip()) ) :
                            #Found it.
                            self.log("AlertId: cmp old,new. |{0}|==|{1}|".format(oldAlert.guid,newAlert.guid))
                            changeNdx = changeNdx +1
                            tmpAlerts.remove(oldAlert)
                            break
                    else:
                        self.log("AlertId: tmpAlerts empty")
                else:
                    self.log("AlertId: newAlert, cur alertIds_objects is empty")
                            
            self.prevAlertIds_objects = alertIds_objects
                        
            # tmpAlerts holds the diff between New and Old alerts
            numTemp = len(tmpAlerts)
            self.log("AlertId: numTmp={2}, ChangeNdx={0}, numAlerts={1}".format(changeNdx,numAlerts,numTemp))


            # Do we need to ask for new Alert Data
            # More Alerts or Less Alerts, requires update.
            # Anything left in Temp(guid not Found or more alerts)
            #  or prevAlert Greater than Current Alerts 
            if (numTemp > 0 or numPrevAlerts < numAlerts):
                params = "?alertid="
                ndx=0
                for i in alertIds_objects :
                    params = params + i.id
                    if ndx < (numAlerts-1) :  
                        params = params + ","
                    ndx += 1
                
                self.log ("AlertId: params={0}".format(params))
                self.alertData.urlAppend(parms=params)
                self.alertData.start()
            
        # ALERTS ==0 But we had alerts so need to tell "display" routine, so he can clear.    
        elif (numAlerts==0 and numPrevAlerts > 0):
            # Case 2 Process to clear Prior alerts
            # Basically requesting blank Alerts get an error tag back and 
            # send display a blank alerts message
            self.alertsClearDisplay()
            

    
    """
    ************************************************
    Method:  def sendAlerts(self,data):
       send to the "display" routine
    """
    def sendAlerts(self, mydata):
        if (mydata):
            self.UDPSock.sendto(mydata,         self.addr)

    """
    ************************************************
    Method:  def sendAudioAlerts(self,data):
       send to the "display" routine
    """
    def sendAudioAlerts(self, mydata):
        if (mydata):
            self.UDPSockAudio.sendto(mydata,    self.addrAudio)

    """
    ************************************************
    Method:  def alertStatus(self, *args):
        args[0] is a list [] of objects, in this case AlertMessages
        process the alert messages.
    """
    def alertStatus(self,*args):
        self.log("tracker ...Alert Data received ")
        # ..new
        self.latestAlerts = args[0]
        numAlerts = len(self.latestAlerts)
        self.log("tracker rcv AlertsData numAlerts={0}".format(numAlerts))
        # build messages
        alertSpeech = "" + chr(CtssMessageTypes.BusAlert)
        alertSpeech = alertSpeech + chr(numAlerts)
        alertSpeech = alertSpeech + chr(0)

        alerts = "" + chr(CtssMessageTypes.BusAlert)
        alerts = alerts + chr(numAlerts)
        alerts = alerts + chr(0)

        if (numAlerts>0):
            try:
                self.latestAlerts = sorted( self.latestAlerts, key=lambda alertMessage: int(alertMessage.severity), reverse=True )
            except:
                self.log("failed to sort, probably do to alertdata error tag, i.e. no alerts available")
                
            for i in self.latestAlerts :
                alerts = alerts + i.description + "\n"

                # speech ? modification ?Do we want to substitute for &amp; &lt; &gt;
                #alertSpeech = alertSpeech + i.speech.replace("&amp;","&") + "\n"
                alertSpeech = alertSpeech + i.speech + "\n"
            
        else:
            # Expect this to clear Alerts ?
            alerts = alerts + " " + "\n"
            alertSpeech = alertSpeech + "There are no, alerts." + "\n"
            

        print(alerts)                                
        self.sendAlerts (alerts)
        self.sendAudioAlerts(alertSpeech)
        self.log ("Sent Alerts...")
        self.log("{0:#x},{1:#x},{2:#x},{3:s}".format(ord(alerts[0]),ord(alerts[1]),ord(alerts[2]),alerts[3:]))


    """
    ************************************************
    Method:  def start():
        Start tracker
    """
    def start(self):
        self.initConfig.initialRequest()  # check ppp is up before sending.
        
        #self.alertData.start() .. caled by alertId message if Id's exist.
    
    """
    ************************************************
    Method:  def stop():
        Stop tracker
    """
    def stop(self):
        self.initConfig.stop()
        self.arrival.stop()
        self.alertId.stop()
        self.alertData.stop()
        if (self.volumeTimer):
            self.volumeTimer.cancel()
#        self.alertData.stop() 

    """
    ************************************************
    Method: def generalFailure(self): 
    """
    def generalFailure(self):
        self.log( "generalFailure= blank screen." + "\n")
        
        blank = []
        # Must fit in 25 chars x 3lines 4line=code,message#
        destinations = [" \t ","\x1f\x1f Arrival  information", " temporarily  unavailable."]
        # ww version 1.0.12 requested no code,serial
        # destinations.append("{0}, {1}".format(CtssErrorCodes.GeneralFailure, self.lumSerialNumber) )
        # ww 1.0.12 --
        
        cnt = 0
        for i in range(len(destinations)) :
            if (i < 4) :
                eta = EtaPrediction()
                eta.destination = destinations[i]
                blank.append(eta)
                cnt = cnt + 1
            else:
                break
        self.arrivalStatus(blank)
        #self.alertsClearDisplay()


                
    """
    ************************************************
    Method: def initFailure(self,**kwargs): 
    """
    def initFailure(self,**kwargs):
#        for key in kwargs:
#            print "{0} reason {}".format(key, kwargs[key])
        self.generalFailure()
    
    """
    ************************************************
    Method: arrivalFailure(self,**kwargs): 
    """
    def arrivalFailure(self, code=CtssErrorCodes.GeneralFailure, message="No message provided"):
        self.log ("arrivalFailure {0}={1}.".format(code,message))
        #    code = kwargs['code']
        #    msg = kwargs['message']
        self.generalFailure()
        
    
    """
    ************************************************
    Method: def alertIdFailure(self,**kwargs): 
    """
    def alertIdFailure(self,**kwargs):
        pass
    
    """
    ************************************************
    Method:     def alertFailure(self,**kwargs):
    """
    def alertFailure(self,**kwargs):
        pass
    


    """
    ************************************************
    Method: def readconfig(self):
    """
    def readconfig(self):

        # Read config
        trackerpath = ""
        self.log( "os.env=" + str(os.environ.keys()) + "\n")
        self.applpath = '/ctss/cfg/tracker.conf'
        
        for i in os.environ.keys():
          if (i == "LUM_TRACKER_HOME"):
                trackerpath = os.environ['LUM_TRACKER_HOME']

        if (len(trackerpath) > 0):
            self.applpath = trackerpath
        self.log( "applpath=" + str(self.applpath))
        
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
                self.log( "error reading config len=" + len(keyvalue)+ " line=" + line)





"""    
# end class Tracker()
"""
