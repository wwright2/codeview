"""
************************************************
Copyright: copyright Luminator 2011 all rights reserved.

project: ctss - chicago transit shelter sign

module: CtaMsgAlertDataId

file: CtaMsgAlertId.py

history:
    wwright 8/18/2011    modularize

annotation:

  Classes:
    CtaMsgAlertData
    
  Generalization
        
Todo:

************************************************
"""

from xml.dom import minidom, Node

from threading import Thread, Timer
from datetime import datetime
from ctaMessage import *
from ctssLogger import *

from xml.dom import minidom, Node
from urlRequestStatus import UrlRequestStatus






"""
************************************************
class AlertIds(object)
        - sort the AlertIds objects.
"""
class AlertIds (object):
    def __init__(self, id="",guid="",error="") :
        #BusPrediction(pdict['speech'],pdict['id'],pdict['route'],pdict['direction'],
        #  pdict['destination'],pdict['minsToArrival'],pdict['fault'],
        #pdict['scheduled'],pdict['delayed'])
        self.id   = id
        self.guid = guid
        self.error= error
        
    def __repr__(self):
        return repr((self.id))

    def sortByID(*args):
        alertIds_objects = args
        sorted( alertIds_objects, key=lambda alertIds: etaPrediction.id )
        return alertIds_objects


class CtaMsgAlertId (CtaMessage,CtssLogger):

    def __init__(self, config, serial,statusRtn,failRtn) :
        # Create static Portion of url's, use config file if defined, else defaults.
        self.urlcmd = config.get('cmdGetAlertIds', 'btAlertIdsSign.php') + '?' \
                + config.get('cmdParmId','serialnum') + '=' \
                + serial
        url = config.get('tracker_url', 'http://signapi.transitchicago.com/ctadiscover/') \
                + self.urlcmd
                
        CtaMessage.__init__(self, url, config, statusRtn, failRtn)
        CtssLogger.__init__(self)
        self.log ("CtaMsgAlertId...")
        self.log ("AlertIds="+url)
        self.timeout = int(config.get('tracker_get_alertId_timeout_sec', 300))

        self.parseTag = "discover"
        self.processName = "Alert ID s"
        
        self.success = False
        self.lastXml = config.get('lastXmlAlertId', '/tmp/lastCtaAlertId.xml')


   
    """
    Method:    def parseXml(self, nodes, my_objects, pdict):
        Parse the xml returned from the url query.
        
        Example:
            <?xml version="1.0" encoding="ISO-8859-1"?>
            <discover>
                    <active_alert GUID="117ed0a5-5a0c-4233-8f94-de34ec38c7a9">7743</active_alert>
                    <active_alert GUID="074ad545-85dd-49a0-93fb-666f6bca08d1">9524</active_alert>
                    <active_alert GUID="b8583a9a-8ac4-4f6e-b3ed-b749294f583f">10334</active_alert>
                    <active_alert GUID="c33216a9-a721-4242-b14c-7e71d1e4a0e2">10886</active_alert>
                    <active_alert GUID="abd9db56-140c-4840-a3c9-47ad98e849a5">11005</active_alert>
            </discover>        
    """
    """
    """
    ## def parseXml(self, urlr, xml, my_objects, pdict):
    #     input: urlr,xml
    #     i/o :  objects, pdict
    #    
    def parseXml(self, urlr, xml, my_objects, pdict):
        self.log ("parse Alert Ids")
        
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
            self.start(restart=self.restart)  # restart my request  
            self.failureRtn(code=urlr.XmlParseTagNotFound, message=urlr.getMessage(urlr.XmlParseTagNotFound))
            return
        
        #        #..If nodes Empty
        #        #..Check for "Error"
        for alertid in nodes :
            elemCnt=0
            for pElem in alertid.childNodes :
                if (pElem.nodeType == Node.ELEMENT_NODE ) :
                    for pValue in pElem.childNodes :
                        if (pElem.nodeName == 'error'):
                            alertId = AlertIds(id="",guid="",error=pValue.nodeValue)
                        elif (pElem.nodeName == 'active_alert'):
#                            print pElem.nodeName, pValue.nodeValue, pElem.attributes['GUID'].value
                            alertId = AlertIds(id=pValue.nodeValue, guid=pElem.attributes['GUID'].value, error="")
                            my_objects.append(alertId)
                            elemCnt +=1

    """
    Method:
    """
    
    """
    Method:
    """


                