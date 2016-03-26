class UrlRequestStatus (object):
    UrlFailed = 1           # message="Http request failed, response=urlopen failed")
    UrlRead   = 2           # message=" Error response.read() failed. Likely socket timeout.")
    ParseNil  = 3           # message="Error, parseTag is Nil")
    XmlParseTagNotFound = 4 # message="Error, parseTag not found in xml")
    
    message = ["Http request failed, response=urlopen failed", \
               "Error response.read() failed. Likely socket timeout.", \
               "Error, parseTag is Nil", \
               "Error, parseTag not found in xml" \
               ]
    
    def getMessage(self,code):
        msgnum = len(self.message)
        if ( (code < msgnum) and (code > 0)):
            return self.message[code-1]
    