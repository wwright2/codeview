
"""
************************************************
Copyright: copyright luminator 2011 all rights reserved 

project: ctss

module: CtaMessage

history:
    wwright 2011aug02   retrieve CTA discovery api data


annotation:
    
    This is a BASE Class for Init,Arrival,AlertId,AlertData
    
    It performs most of the default actions needed to process 
    a query from the sign to the CTA server. tracker.py contains 
    a Callback <msg>"Status" routine.  When the query is returned the 
    ctaMessage process the XML saves a copy to file /tmp/<msg>.xml by default,
    calls a parseXML routine specific to the Message then 
    calls a <?>Status routine in tracker.py (init,arrival,alertId,alert)Status 
    routine is called.
    
    Messages are sent on a timer, the specifics are defined 
    in the __init__ for each of the ctaMsg<type>.py
    
    If a function can be genericized it is stored here, and overridden by the child if needed.
    
    Inherit logging handler
    Store URL def for the message
    Store config data.
    
  Classes:
      CtaMessage(object)
            |_ CtaMsgInitData
            |_  CtaMsgArrivals
            |_  CtaMsgAlertId
            |_  CtaMsgAlertData

  Messages:
      __init__()
      __request()
      updateTimeout() - change the time out, polling time for the message
      start() - start timer
      stop() - stop timer stop sending the message
      parseXml() - Abstract should be overriden by the Message  
      urlAppend() - used to append parameters to the URL before sending
      lastRequest() - return true/false for success of the last URL sent
      clearRebootCnt() - if last request success then clear the reboot counter.
      decode() - check HTTP request for zip, unzip if needed.
      processRequest() - timer expired
      
    
  Generalization
        
Todo:

************************************************
"""

import time

import urllib2
import gzip
import StringIO
import traceback

from xml.dom import minidom, Node
from threading import Thread, Timer
from datetime import datetime
from urlRequestStatus import UrlRequestStatus

# from commSupport import *

    

class CtaMessage(object) :
    def __init__(self, url, config, statusRtn, failRtn) :
        # Create static Portion of url's, use config file if defined, else defaults.
        self.url = url
        self.tmpUrl=url
        self.statusRtn  = statusRtn
        self.failureRtn = failRtn
        
        self.config = config
        self.req = ""
        self.timer = None
        self.debugMode = config.get('tracker_debug', 'false')
        self.GzipRequest = config.get('tracker_gzip_request','true')
        self.httpTimeout = config.get('tracker_http_timeout_sec',8)
        
        self.restart=1
        self.success = False
        self.processName = "cta message"
        
# CML        self.comm = CommSupport(config)
        

#        self.resend = 3
#        self.timeout = 20
#        self.lastXml = "/tmp/lastArrivalXml.xml"
#        self.success = False
#        self.parseTag = "eta"

    """
    ************************************************
    Method: def urlUpdate(self,alturl):
              config message can change our URL
    """
    def urlUpdate(self,alturl):
        self.log("{0} oldUrl={1}".format(self.processName, self.url))
        self.log("{0} alturl={1} append={2}".format(self.processName,alturl, self.urlcmd))
        if (len(alturl)):
            try:
                url = alturl + self.urlcmd
                self.url   = url
                self.tmpUrl = url
            except :
                self.log("Exception: updating URL and CtaMessage.urlUpdate(self,url)") 
                
            self.log("{0} newUrl={1}".format(self.processName, self.url))

        
    """
    ************************************************
    Method: private def __request (self, callbackRtn):
         send url request then 
         call the callback routine.
    """
    def __request(self,callbackRtn):
        try:
            self.req = urllib2.Request(self.tmpUrl)
            if (self.GzipRequest == 'true'):
                self.req.add_header('Accept-encoding', 'gzip')
            callbackRtn()
        except:
            self.log("Exception: Top level Process URL Request Failed.  General Failure.")
            self.log("Exception: url={0}".format(self.tmpUrl))

    """
    ************************************************
    Method:     def updateTimeout(self,timeout):
    """
    def updateTimeout(self,timeout):
        self.log("updateTimeout old={0}, new={1}".format(self.timeout, timeout))
        
        # DONT allow faster than 20 Seconds timeout IDIOT PROOF
        if (timeout > 20):
            self.timeout = timeout
    
    """
    ************************************************
    Method: def start(self, delay):
        -Entry routine after Creating Msg call Start, to begin sending URL Request 
        - Start the requests, by default the Request are resent based on a timer
           ..if an error occurs may be resent more quickly using delay.
           ..if multiple sequential errors occur..drop into diagnostics mode
              ..try to restart modem, or ethernet connection. 
    """
    def start(self,delay=0,restart=1):
        self.log("Start... " + self.processName )

        self.timer = None
        if (delay > 0):
            # ...Error resend sooner.
            self.timer = Timer(delay, self.urlRequest)
        else:
            self.timer = Timer(int(self.timeout), self.urlRequest)

        try:
            if (restart):
                self.timer.start()
        except:
            self.log (" Error: exception trying to start "+ self.processName)

    """
    ************************************************
    Method: def stop(self):

    """
    def stop(self):
        self.log (" Stop ... "+ self.processName)
        if self.timer is not None :
            self.timer.cancel()
        
    """
    ************************************************
    Method: def parseXml(self, nodes):
         override this to get specific XML parsing.
    """
    def parseXml(self, nodes, my_objects, pdict):
        pass


    """
    ************************************************
    Method: def urlAppend(self,parms):
      Modify the default url, i.e. change parameters.
    """
    def urlAppend(self,parms=""):
        self.log ("Append Url: "+ parms)
        
        self.tmpUrl = self.url + parms
        
        
    """
    ************************************************
    Method: def urlRequest(self):
    """
    def urlRequest(self):
        self.log("entry " + self.processName + " ... "+ str( datetime.today()))
        
        self.rc = self.__request(self.processRequest)
        if (self.lastRequest()== True):
            self.clearRebootCnt()
        else:
            self.log("urlRequest, failed.")
            if (self.failureRtn != None ):
                self.failureRtn()

    """
    ************************************************
    Method: lastRequest(self): 
       ...so we can inherit and add logic if needed.
    """
    def lastRequest(self):
        return self.success
    
    """
    ************************************************
    Method: def clearRebootCnt(self):
        pppMonitor - monitors the ppp0 and if it is down tries to reset
                   - if reset ppp0 fails it reboots upto 3 times
        if we get a message through we are going to 
            clear reboot count.
            clear ppp-restart count.
    """
    def clearRebootCnt(self):
        
        # Clear reboot
        while (1):
            try:
                fp = open ("/ctss/reboot.cnt", 'w')
                if (fp) :
                    fp.write("0\n")
                    fp.close()
                    break
            except:
                pass
            time.sleep (1)

        while (1):
            try:
                fp = open ("/ctss/ppp-restart.cnt", 'w')
                if (fp) :
                    fp.write("0\n")
                    fp.close()
                    break
            except:
                pass
            time.sleep (1)

    """
    ************************************************
    Method: def decode(self):
        General URL Processing.
    """
    def decode (self, response):
        encoding = response.info().get("Content-Encoding")
        if encoding in ('gzip', 'x-gzip', 'deflate'):
            content = response.read()
            if encoding == 'deflate':
                self.log("DECODE...deflate format returned")
                data = StringIO.StringIO(zlib.decompress(content))
            else:
                self.log("DECODE...gzip format returned")
                data = gzip.GzipFile('', 'rb', 9, StringIO.StringIO(content))
            try:                
                returndata = data.read()
            except e:
                self.log("Exception trying to unzip data")
                
        else:
            self.log("DECODE...Normal format returned")
            returndata = response.read()
            
        return returndata
            
    """
    ************************************************
    Method: def processRequest(self):
        General URL Processing.
    """
    def processRequest(self):
        self.log("processRequest, "+ self.processName + " ...")
        self.success = False
        urlr = UrlRequestStatus()
        
        try:
            self.response = urllib2.urlopen(self.req, None, float(self.httpTimeout))

        except urllib2.URLError, e:
            self.log( "Http request failed, response=urlopen failed")
            self.start(delay=12)
            self.failureRtn(code=urlr.UrlFailed, message=urlr.getMessage(urlr.UrlFailed))
            return

        except :
            self.log( "Http request failed, response unknown")
            self.log("{0}".format(traceback.print_exc()))
            self.start(delay=12)
            self.failureRtn(code=urlr.UrlFailed, message=urlr.getMessage(urlr.UrlFailed))
            return
            
        else:
            
            try:
                xml = self.decode(self.response)

            except Exception as e:
                self.log(" Error response.read() failed. Likely socket timeout.")
                # re-try in 15 secs 
                self.start(delay=3)
                self.failureRtn(code=urlr.UrlRead, message=urlr.getMessage(urlr.UrlRead))
                return

            self.success = True     # Query Succeeded in sent and recieved response

            # Dump Last XML recived
            if (self.debugMode.lower() == "true"):
                with open(self.lastXml, "w+b") as xmlfp :
                    xmlfp.write(xml)
                xmlfp.close()
           
            if ((self.parseTag is None) or (self.parseTag == "")) :
                self.log ( "Error, parseTag is not defined, return")
                self.failureRtn(code=urlr.ParseNil, message=urlr.getMessage(urlr.ParseNil))
                return 

            my_objects = []
            pdict = dict()
            try:
                self.parseXml(urlr,xml, my_objects, pdict)
            except:
                self.log ("Exception: parseXml() name={0}".format(self.processName))
                
            self.log("{0} : parseXml returned my_objects {1},len={2}".format(self.processName,my_objects, len(my_objects)))
            
            self.start(restart=self.restart)  # restart my request  

            if (self.statusRtn != None):
                self.statusRtn(my_objects)
            else:
                self.log("statusRtn == NONE, name=" + self.processName)
    

    
"""    
# end class ctaMessage
"""    
