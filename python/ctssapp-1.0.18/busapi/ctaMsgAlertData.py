"""
************************************************
Copyright: copyright Luminator 2011 all rights reserved.

project: ctss - chicago transit shelter sign

module: ctaMsgAlertData

file: ctaMsgAlertData.py

history:
    wwright 8/18/2011    modularize

annotation:

  Classes:
    CtaMsgAlertData
    AlertMessage
        
Todo:

************************************************
"""

from xml.dom import minidom, Node

from ctaMessage import *
from ctssLogger import *

# from commSupport import CommSupport
from threading import Thread, Timer
from datetime import datetime
from urlRequestStatus import UrlRequestStatus



"""
************************************************
class AlertMessage(object)
        - sort the alert message objects.
"""
class AlertMessage (object):
    def __init__(self, **kwargs) :
        if (len(kwargs)):
            self.id          = kwargs['id']
            self.severity    = kwargs['severity']
            self.speech      = kwargs['speech']
            self.description = kwargs['description']
        else:
            self.id          = 0
            self.severity    = "0"
            self.speech      = " \n"
            self.description = " \n"
            
            
        
    def __repr__(self):
        return repr((self.id, self.severity, self.speech, self.description))

    def sortByAlertSeverity(*args):
        alertMessage_objects = args
        return sorted( alertMessage_objects, key=lambda alertMessage: alertMessage.severity )




class CtaMsgAlertData (CtaMessage,CtssLogger):

    def __init__(self, config,serial,statusRtn,failRtn) :
        self.log ("CtaMsgAlertData ...")
        # Create static Portion of url's, use config file if defined, else defaults.
        self.urlcmd = config.get('cmdGetAlertData', 'btAlertDataSign.php')
        url = config.get('tracker_url', 'http://signapi.transitchicago.com/ctadiscover/') \
                + self.urlcmd
        CtaMessage.__init__(self, url, config, statusRtn, failRtn)
        CtssLogger.__init__(self)
        self.log ("Alerts="+url)
        self.lastXml = config.get('lastXmlAlert', '/tmp/lastCtaAlertData.xml')
        self.timeout = config.get('tracker_get_alertdata_timeout_sec', 2)
        self.timer = None
        self.processName = "Alert Data"
        self.parseTag = "discover"
        self.restart = 0                    # dont poll data Only if alertId info changes.
       
        
    """
    Method:    def parseXml(self, nodes, my_objects, pdict):
        Parse the xml returned from the url query.
        
        Example:
    """
    """
    """
    ## def parseXml(self, urlr, xml, my_objects, pdict):
    #     input: urlr,xml
    #     i/o :  objects, pdict
    #    
    
    def parseXml(self, urlr, xml, my_objects, pdict):
        self.log("parse Alert Data")
        
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
        elemCnt=0
        for alertmsg in nodes :
            # discover
#            print "alertmsg=", alertmsg

            for pElem in alertmsg.childNodes :
                # error, alert
                
                if (pElem.nodeType == Node.ELEMENT_NODE ) :
                    mySpeech = ""
                    myDescription = ""
                    if (pElem.nodeName == 'error'):
                        self.log ( "error..pElem={0}, xml={1}".format(pElem.nodeValue,pElem.toxml()))

                    elif (pElem.nodeName == 'alert'):
                        for pValue in pElem.childNodes :
                            # speech,description
                            if (pValue.nodeType == Node.ELEMENT_NODE ) :
#                                print "pValue in pValue.childNodes, nodeName=", pValue.nodeName, ",", pValue.toxml()
                                for qValue in pValue.childNodes :
#                                    print "qValue in pValue.childNodes, nodeName=",qValue.nodeName, ",", qValue.toxml()
                                    if ( pValue.nodeName == "speech") :
                                        mySpeech = qValue.toxml().strip()
#                                        print "myspeech=" ,mySpeech
                                    elif ( pValue.nodeName == "description" ) :
                                        myDescription = qValue.toxml().strip()
#                                        print "description=",myDescription
                                    else :
                                        print "inet=",qValue.nodeName,"\n"

                    try:
                        myId = pElem.attributes['id'].value
                        mySeverity = pElem.attributes['severity'].value
                        alertMsg = AlertMessage (id=myId, severity=mySeverity, speech=mySpeech, description=myDescription)
                        my_objects.append(alertMsg)
                    except :
                        self.log ("AlertData parse exception trying to retrieve id,severity, probably due to error tag not eta tag")
                        my_objects = []
                                                                
                    elemCnt +=1

# End class
    
    
