#!/usr/bin/env python

#
#
#=======================================================================
#print a short help message
def usage():
    sys.stderr.write("""USAGE: %s [options]
    
    mode 1:
        serialcmd - send cmd string to port and exit
    mode 2:
        serialcmd - interactive talk to modem.

    Example mode 1:
        a) send multiple commands at one time
             serialcmd.py -p /dev/ttyS0 -b 115200 -c "ate1 at&v ate0"
        b) send to devicd ACM0 and exit
		     serialcmd.py -p /dev/ttyACM0 -b 460800 -c 'at!status'
		c) send to USB0 device and exit  
             serialcmd.py -p /dev/ttyUSB0 -b 460800 -c 'at!status'
        
    Example mode 2:
       ./serialcmd.py -V -p /dev/ttyUSB0 -b 460800 -c ''

    options:
    -p, --port=PORT : port, a number, default = 0 or a device name. /dev/ttyACM0 modem
    -b, --baud=BAUD : baudrate, default 9600, 460800 for usb device.
    -c, --command   : command to send to port
    -S, --script=scriptfile  : script file to execute i.e. multiple commands
    -r, --rtscts    : enable RTS/CTS flow control (default off)
    -x, --xonxoff   : enable software flow control (default off)
    -e, --echo      : enable local echo (default off)
    -v, --cr        : do not send CR+LF, send CR only
    -n, --newline   : do not send CR+LF, send LF only
    -V, --verbose   : print some info while executing
    -D, --debug     : debug received data (escape nonprintable chars)

""" % (sys.argv[0], ))
#========================================================================  
#


import sys, os, serial, threading, getopt, time


EXITCHARCTER = '\x04'   #ctrl+D

CONVERT_CRLF = 2
CONVERT_CR   = 1
CONVERT_LF   = 0

def reader():
    """loop forever and copy serial->console"""
    while 1:
        data = s.read()
        if repr_mode:
            sys.stdout.write(repr(data)[1:-1])
        else:
            sys.stdout.write(data)
        sys.stdout.flush()

# -----------------------------------
# Write intput to a serial device
# def writecmd(ser,input)
#    return output from serial write
def writecmd(ser,input):
   ser.write(input + "\r\n")
   time.sleep(.25)
   out = ''
   while ser.inWaiting() > 0:
      out += ser.read(1)
   return out
    


if __name__ == '__main__':

    #initialize with defaults
    # expect USB device

    # /dev/ttyACM0
    port  = "/dev/tty10"
    # 460800 usb device.
    baudrate = 115200

    cmdfileh = 0
    cmdfile=0

    cmd="at"

    echo = 0
    verbose = 0
    convert_outgoing = CONVERT_CRLF
    rtscts = 0
    xonxoff = 0
    repr_mode = 0
    cmds=""

    #parse command line options
    try:
        opts, args = getopt.getopt(sys.argv[1:],
            "hp:b:c:S:rxeVvnD",
            ["help", "port=", "baud=", "command", "script=", "rtscts", "xonxoff", "echo",
            "verbose", "cr", "newline", "debug"]
        )
    except getopt.GetoptError:
        # print help information and exit:
        usage()
        sys.exit(2)

    for o, a in opts:
        if o in ("-h", "--help"):       #help text
            usage()
            sys.exit()
        elif o in ("-S", "--script"):   #specified script file.
            cmdfile=str(a)

            try:
                cmdfileh = open(cmdfile)
            except IOError as e:
                print("({})".format(e))

            line = cmdfileh.readline()
            while (line):
                cmds=cmds+line+"\n"
            cmdfileh.close()

        elif o in ("-c", "--command"):  #specified single command.
            cmds = str(a)
            #print "o is cmd=" + str(a)

        elif o in ("-p", "--port"):     #specified port
            try:
                port = int(a)
            except ValueError:
                port = a
        elif o in ("-b", "--baud"):     #specified baudrate
            try:
                baudrate = int(a)
            except ValueError:
                raise ValueError, "Baudrate must be a integer number, not %r" % a
        elif o in ("-r", "--rtscts"):
            rtscts = 1
        elif o in ("-x", "--xonxoff"):
            xonxoff = 1
        elif o in ("-e", "--echo"):
            echo = 1
        elif o in ("-V", "--verbose"):
            verbose = 1
        elif o in ("-v", "--cr"):
            convert_outgoing = CONVERT_CR
        elif o in ("-n", "--newline"):
            convert_outgoing = CONVERT_LF
        elif o in ("-D", "--debug"):
            repr_mode = 1

    if (verbose):
        print "baud=" + str(baudrate)
        print "port=" + str(port)
        print "cmds=" + str(cmds) + "\n"

    #open the port
    try:
        s = serial.Serial(port, baudrate, rtscts=rtscts, xonxoff=xonxoff)
        s.open()
        s.isOpen()
        
    except:
        sys.stderr.write("Could not open serial Deivce port={0}\n".format(port))
        sys.exit(1)

    #start serial->console thread
    if (verbose):
        print "start Reading\n"
    r = threading.Thread(target=reader)
    r.daemon=True
    r.start()

    cmdlist=cmds.split()
    if (verbose):
       print cmdlist

    # Mode 1:
    if len(cmdlist):
        # write the commands to the serial port.
       for i in cmdlist:
           if (verbose):
               print("sending " + i )
           out = writecmd(s,i)
       
    # Mode 2:
    else:
        while 1 :
           input = raw_input(">> ")
           if input == 'exit' :
               s.close()
               exit()
           else:
               out = writecmd(s,input)
    #wait
    #exit reader
    #close serial
    r.join(1.0) #exit, force after 2 seconds.

    s.close()

    sys.stderr.write("\n--- exit ---\n")

