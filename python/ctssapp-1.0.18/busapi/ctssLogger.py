

import logging
from logging.handlers import SysLogHandler

ctssSyslog=None
ctssFormatter = None


class CtssLogger(object):
    
    __instance = None
    def __init__(self) :
        if CtssLogger.__instance is None:
            CtssLogger.__instance = logging.getLogger()
            
            self.__instance = logging.getLogger()

            self.__instance.setLevel(logging.INFO)
            ctssSyslog = SysLogHandler(address='/dev/log')
            ctssFormatter = logging.Formatter('%(name)s: %(levelname)s %(message)s')
            ctssSyslog.setFormatter(ctssFormatter)
            self.__instance.addHandler(ctssSyslog)
        
    def log(self, args):
        if CtssLogger.__instance is None:
            pass
        else:
            try:
                self.__instance.info(args)
            except:
                print "Log encountered an exception."

            

#   high        
    def logcritical(self,args):
        self.__instance.critical(args)

#   medium
    def loginfo(self,args):
        self.__instance.info(args)
#   low
    def logdebug(self,args):
        self.__instance.debug(args)
    
    
