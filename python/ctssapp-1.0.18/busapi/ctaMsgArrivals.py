"""
************************************************
Copyright: copyright Luminator 2011 all rights reserved.

project: ctss - chicago transit shelter sign

module: CtaMsgArrivals

file: CtaMsgArrivals.py

history:
    wwright 4/21/2011    modularize

annotation:

  Classes:
    CtaMsgArrivals - handle arrival request/response
    
  Generalization
        
Todo:

************************************************
"""
import string

from xml.dom import minidom, Node

from threading import Thread, Timer
from datetime import datetime
from ctaMessage import *
from ctssLogger import *
from urlRequestStatus import UrlRequestStatus


from xml.sax.saxutils import unescape
   
from HTMLParser import HTMLParser

class MLStripper(HTMLParser):
    def __init__(self):
        self.reset()
        self.fed = []
    def handle_data(self, d):
        self.fed.append(d)
    def get_data(self):
        return ''.join(self.fed)

def strip_tags(html):
    s = MLStripper()
    s.feed(html)
    return s.get_data()


"""
************************************************
class EtaPrediction(object)
        - sort the eta prediction objects.
"""
class EtaPrediction (object):
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
            self.minsToArrival = args[5]
            self.fault       = args[6]
            self.scheduled   = args[7]
            self.delayed     = args[8]
            

        else :
            self.speech=" "; self.id=0; self.route=" "; self.direction=" "; 
            self.destination=" "; self.minsToArrival=0; self.fault=-1 ; self.scheduled=-1; self.delayed=0;
        
    def __repr__(self):
        return repr((self.speech, self.id, self.route, self.direction, self.destination, self.minsToArrival, self.fault, self.scheduled, self.delayed))

    def sortByArrival(*args):
        etaPrediction_objects = args
        sorted( etaPrediction_objects, key=lambda etaPrediction: etaPrediction.minsToArrival )
        return etaPrediction_objects
    

    
    
##  class CtaMsgArrivals (CtaMessage,CtssLogger):
##  wwright
class CtaMsgArrivals (CtaMessage,CtssLogger):

    ##  def __init__(self, config,serial,statusRtn,failRtn) :
    def __init__(self, config,serial,statusRtn,failRtn) :

        CtssLogger.__init__(self)
        self.log ("CtaMsgArrivals =")
        # Create static Portion of url's, use config file if defined, else defaults.
        self.urlcmd = config.get('cmdGetStopArrivals', 'btStopArrivalsSign.php') + '?' \
                + config.get('cmdParmId','serialnum') + '=' \
                + serial
        url = config.get('tracker_url', 'http://signapi.transitchicago.com/ctadiscover/') \
                + self.urlcmd
        CtaMessage.__init__(self, url, config, statusRtn, failRtn)
        self.log ("arrivals="+url)

        self.lastXml = config.get('lastXmlArrival', '/tmp/lastArrivalXml.xml')
        self.success = False
        
        self.parseTag = "eta"
        self.processName = "Arrival eta"
        self.timeout = 20
        
        # No reason to poll faster than 60seconds buses only report every 1 min.            
        self.timeout = int(config.get('tracker_getprediction_timeout_sec', '30'))
        if (self.timeout < 5  or self.timeout > 600 ):
            self.timeout = 20

        # Error on request resend in n Seconds.
        self.resendPredictionRequest = int(config.get('tracker_prediction_error_resend_sec','15'))  # Failed to get a good response Resend in 15 sec.
        if (self.resendPredictionRequest > 90 or self.resendPredictionRequest < 1 ):
            self.resendPredictionRequest = 15   # set default

        self.log( "request Prediction timeout in seconds = " + str(self.timeout))
        self.pingAddr = config.get('tracker_ping_addr','ctabustracker.com')


    """
    """
    ## def parseXml(self, urlr, xml, etaPrediction_objects, pdict):
    #     input: urlr,xml
    #     i/o :  objects, pdict
    #    
    def parseXml(self, urlr, xml, etaPrediction_objects, pdict):
        self.log ("parse Arrivals")
        
        try:
            dom = minidom.parseString(xml)
            nodes = dom.getElementsByTagName(self.parseTag)
        except Exception as xmle:
            self.log( " Error xml parse exception. " + self.parseTag + " " + str( datetime.today()))
            self.log( " Error xml = \n" + str(xml))
            # Normal re-try accroding to message timer
            # Assumes the request fails because there is no data to return. 
            # No reason to request any more than ussual 
            #ww. was self.start(delay=5)

#            self.start(restart=self.restart)  # restart my request  
#            self.failureRtn(code=urlr.XmlParseTagNotFound, message=urlr.getMessage(urlr.XmlParseTagNotFound))
#            return

            # ww. update, set nodes empty and check for <error> tags
            nodes = []

        # Parse ETA Tags 
        if (len(nodes) > 0):
                        
            #..If nodes Empty
            #..Check for "Error"
            for eta in nodes :
                etaElemCnt=0
                for pElem in eta.childNodes :
                    if (pElem.nodeType == Node.ELEMENT_NODE ) :
                        for pValue in pElem.childNodes :
                           if (pElem.nodeName == 'minsToArrival'):
                               pdict[pElem.nodeName] = int (pValue.nodeValue)
                           elif (pElem.nodeName == 'speech'):
                               pdict[pElem.nodeName] = pValue.toxml().strip()
                           else:
                               pdict[pElem.nodeName] = pValue.nodeValue
                               #print pElem.nodeName, pValue.nodeValue
    
                etaElemCnt += 1
                try:
                    etaPred  = EtaPrediction(pdict['speech'],pdict['id'],pdict['route'],pdict['direction'],pdict['destination'],pdict['minsToArrival'],pdict['fault'],pdict['scheduled'],pdict['delayed'])
                    etaPrediction_objects.append(etaPred)
                except :
                    self.log ("Exception: while processing ETA, missing/too many Tags ")
                    slef.log ("Exception: ETA expected tags[speech,id,route,direction,destination,minsToArrival,fault,scheduled,delayed]")

            self.log("Arrivals : returns my_objects {0},len={1}".format(etaPrediction_objects, len(etaPrediction_objects)))

        # else NO ETA tags look for error tag
        else:
            try:
                nodes = dom.getElementsByTagName("error")
            except Exception as errortag:
                # ww. Set failure code no eta tags, no error tags Found.
                self.log( " Error xml looking for error tag. parse exception. " + self.parseTag + " " + str( datetime.today()))
                self.log( " Error xml = \n" + str(xml))
                self.failureRtn(code=urlr.XmlParseTagNotFound, message=urlr.getMessage(urlr.XmlParseTagNotFound))
                #Just take the first one and send as Arrival
                self.log( "generalFailure= blank screen." + "\n")
                return

            for err in nodes :
                if (err.nodeName == 'error') :
                    self.log ( "Arrivals: error found ..pElem={0}, xml={1}".format(err.nodeValue,err.toxml()))
                    for pValue in err.childNodes :
                        self.log ("pElem = {0}, xml={1}".format(pValue.nodeValue,pValue.toxml().strip()))
                        errstring = unescape(pValue.toxml().strip())

                    # Ok now need to break up into 25 char lines, upto 4 lines
                    cnt = 0
                    eta = EtaPrediction()
                    #eta.destination = p
                    eta.destination = "Error: " + errstring
                    etaPrediction_objects.append(eta)

"""
                    errarray = string.split(errstring)
                    p = "" 
                    
                    for x in errarray :
                        if (cnt > 3):
                            break

                        # x long char string. i.e. 25,50,75,100
                        #
                        if len(x) > 24 :                        
                            self.log("1 p={0} cnt={1}, len(objects)={2}".format(p,cnt,len(etaPrediction_objects)))
                            while (len(x) > 24 and cnt < 4):
                                p = x[0:23]
                                swap = x[24:]
                                x = swap
                                eta = EtaPrediction()
                                eta.destination = p
                                p = ""
                                etaPrediction_objects.append(eta)
                                cnt = cnt+1
                            
                        if ( (len(p)+len(x)) < 25 ) :
                            self.log("2 p={0} cnt={1}, len(objects)={2}".format(p,cnt,len(etaPrediction_objects)))
                            if (len(p)):  
                                p = p + " " + x
                            else: 
                                p = p + x
                            x = ""

                        # put P on a line and handle rest of x                            
                        else :
                            self.log("3 p={0} cnt={1}, len(objects)={2}".format(p,cnt,len(etaPrediction_objects)))
                            eta = EtaPrediction()
                            eta.destination = p
                            p = ""
                            etaPrediction_objects.append(eta)
                            cnt = cnt+1

                            if len(p)+len(x) < 25 :  
                                if (len(p)):  
                                    p = p + " " + x
                                else:
                                    p = p + x
        
                    if (cnt < 4):
                        eta = EtaPrediction()
                        #eta.destination = p
                        eta.destination = "Error: " + errstring
                        etaPrediction_objects.append(eta)
                    

        
"""
