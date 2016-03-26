"""
************************************************
Copyright:

project: ctss

module: charbuffer

file: charbuffer.py

history:
    wwright 4/21/2011    modularize

annotation:

  Classes:
    CharBuffer - using to define fixed lenght buffer to pass to 'C' via UDP.
    
  Generalization
        
Todo:

************************************************
"""


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
