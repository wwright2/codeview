
"""
************************************************
Copyright: copyright Luminator 2011 all rights reserved.

project: ctss - chicago transit shelter sign

module: CtaMsgInitData

file: CtaMsgInitData.py

history:
    wwright 8/18/2011    modularize Demo, handle new CtaDiscovery API

annotation:

  Classes:
    CtaMsgInitData - handle initConfig request/response
    
  Generalization
        
Todo:

************************************************
"""

"""
<discover>
    <time>
        <month>7</month>
        <day>19</day>
        <year>2011</year>
        <hour>4</hour>
        <minute>15</minute>
        <second>59</second>
        <meridiem>PM</meridiem>
    </time>
    <settings>
        <day_time>06:30:00</day_time>
        <night_time>19:00:00</night_time>
        <day_volume>6</day_volume>
        <night_volume>4</night_volume>
        <arrival_display_sec>8</arrival_display_sec>
        <alert_display_sec>10</alert_display_sec>
        <blank_time>2</blank_time>

[ proposed update 1/13/2012]
        <discover_url>http://signapi.transitchicago.com/ctadiscover/</discover_url>
        <arrival_poll_sec>20</>
        <alertid_poll_sec>86400</alertid_poll_sec>
        <initconfig_poll_sec>28800</tracker_initconfig_timeout_sec>
        
    </settings>
</discover>
"""

from threading import Thread, Timer
from datetime import datetime
from ctaMessage import *
from ctssLogger import *
import traceback

from urlRequestStatus import UrlRequestStatus


class CtaMsgInitData(CtaMessage,CtssLogger):
    
    def __init__(self, config, serial, statusRtn,failRtn) :

        self.urlcmd = config.get('cmdGetInitConfig', 'btInitConfigSign.php') + '?' \
                + config.get('cmdParmId','serialnum') + '=' \
                + serial
 
        url = config.get('tracker_url', 'http://signapi.transitchicago.com/ctadiscover/') \
                + self.urlcmd
                
        CtaMessage.__init__(self, url, config, statusRtn, failRtn)
        CtssLogger.__init__(self)

        
        self.log ("CtaInitData ="+url)
        # Create static Portion of url's, use config file if defined, else defaults.

        self.processName = "Init Config"
        
        self.resendTimeout = 30  #..Seconds.
        self.lastXml = config.get('lastXmlInitConfig', '/home/cta/lastCtaInit.xml')
        
        self.parseTag = "settings"

        self.settings = dict()
        self.servertime = dict()

        self.servertime['month'] = 1
        self.servertime['day']   = 1
        self.servertime['year']  = 2011
        self.servertime['hour']  = 6
        self.servertime['minute'] = 12
        self.servertime['second'] = 30
        self.servertime['meridiem'] = "AM"                

        
        self.settings['day_time']   = "6:30:00"
        self.settings['night_time'] = "19:00:00"
        self.settings['day_volume']   = 27        # 0..31 really  13..28
        self.settings['night_volume'] = 20        # 0..31
        self.settings['arrival_display_sec'] = 8
        self.settings['alert_display_sec']   = 10
        self.settings['blank_time']          = 200

        
        self.success = False
        self.restart = 1        # Yes, restart timer.
        
        self.timeout = int(self.config.get('tracker_initconfig_timeout_sec', 28800))  # retrieve every 8 hours, default.
        self.log( "request CtaMsgInitData timeout in seconds = " + str(self.timeout))
        self.log(config.get('tracker_file','tracker.txt'))
        self.pingAddr = config.get('tracker_ping_addr','signapi.transitchicago.com')

        """
         wwright 1/11/2012 ... requested enhancemsnts.
         - settings Additions to original API
            alt_url - for transitioning to multiple servers.
            arrival_poll_sec - due to congestion may want to change these poll timers.
            alertid_poll_sec - dito
        """
        # set defaults for variables.
        self.settings['discover_url'] = config.get('tracker_url', 'http://signapi.transitchicago.com/ctadiscover/')
        self.settings['arrival_poll_sec'] = int(config.get('tracker_getprediction_timeout_sec',20))
        self.settings['alertid_poll_sec'] = int(config.get('tracker_get_alertId_timeout_sec',86400))
        self.settings['initconfig_poll_sec'] = int(config.get('tracker_initconfig_timeout_sec',28800))

        

    """
    ************************************************
    Method: def initialRequest(self):
      first time we call InitConfig we are starting the System in General.
    """
    def initialRequest(self):
        self.urlRequest()
                

    """
    ************************************************
    Method: def update(self):
    """
    def update(self):
        pass

    
    """
    ************************************************
    Method: def log(self,args):
    """
#    def log(self,args):
#        super(CtaMsgInitData,self).log(args)
   
    """
    ************************************************
    Method: def modemFailed(self):
    """
    def modemFailed(self):
        pass

    """
    ************************************************
    Method: def errorCallback(self):
    """
    def errorCallback(self):
        pass

        
    


    """
    ************************************************
    ## Method: def parseXml(self, xml, my_objects, pdict):

    #   <?xml version="1.0" encoding="ISO-8859-1"?>
    #   <discover>
    #       <time>
    #           <month>8</month>
    #           <day>24</day>
    #           <year>2011</year>
    #           <hour>4</hour>
    #           <minute>59</minute>
    #           <second>25</second>
    #           <meridiem>PM</meridiem>
    #       </time>
    #       <settings>
    #           <day_time>06:30:00</day_time>
    #           <night_time>19:00:00</night_time>
    #           <day_volume>6</day_volume>
    #           <night_volume>4</night_volume>
    #           <arrival_display_sec>8</arrival_display_sec>
    #           <alert_display_sec>10</alert_display_sec>
    #           <blank_time>2</blank_time>
    #        </settings>
    #   </discover>
    """
    """
    """
    ## def parseXml(self, urlr, xml, my_objects, pdict):
    #     input: urlr,xml
    #     i/o :  objects, pdict
    #    
    def parseXml(self, urlr, xml, my_objects, pdict):
        self.log ("parse Init Data")
        
        try:
            dom = minidom.parseString(xml)
            servertime = dom.getElementsByTagName("time")
            settings = dom.getElementsByTagName("settings")
            errors = dom.getElementsByTagName("error")
        except Exception as xmle:
            self.log( " Error xml parse exception. " + str( datetime.today()))
            self.log( " Error xml = \n" + str(xml))
            # re-try in 15 secs 
            self.success = True
            self.start(delay=30)
            return
            
        # Ignore Config Init with Error Tags
        self.log("InitData.. Errors=len(errors)={0} ".format(len(errors)))
        if len(errors) :
            self.start(delay=25)
            return
        
        for cfg in settings :
            cnt = 0
            for pElem in cfg.childNodes :
                if (pElem.nodeType == Node.ELEMENT_NODE ) :
                    #print cnt, pElem.toxml(), pElem.childNodes
                    cnt += 1
                    for pValue in pElem.childNodes :
                       self.settings[pElem.nodeName] = pValue.nodeValue

        for atime in servertime :
            cnt = 0
            for pElem in atime.childNodes :
                if (pElem.nodeType == Node.ELEMENT_NODE ) :
                    #print cnt, pElem.toxml(), pElem.childNodes
                    cnt += 1
                    for pValue in pElem.childNodes :
                       self.servertime[pElem.nodeName] = pValue.nodeValue
        
        #self.postUpdate()
        for b in self.settings.keys() : 
            self.log ( "{0}={1}".format(b,self.settings[b]))
        for b in self.servertime.keys() :
            self.log ("{0}={1}".format (b,self.servertime[b]))


    """
    ************************************************
    Method: lastRequest(self) 
    """
    def lastRequest(self):
        return self.success
    

    """
    ************************************************
    Method: 
    
        ..
        
    """



    """
    ************************************************
    Method: 
    
        ..
        
    """


"""    
# end class ctaMsgInitData()
"""















