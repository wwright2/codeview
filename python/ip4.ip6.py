#!/usr/bin/env python

# 0-ffff
# ip6 = "2001:0db8:0000:0000:0000:ff00:0042:8329".split(":")
# ip4 = "121.18.19.20".split(".")

import sys

print ('Number of arguments:', len(sys.argv), 'arguments.')
#print 'Argument List:', str(sys.argv)


def isIp(ipstring):

	print (ipstring)
	ip = ipstring.split(':')
	rv=""
	
	if len(ip) == 8 :
		print (ip)
		rv="ip6"
		for i in ip :
			if i == "" :
				continue
				
			try :
				value = int(i,16)
			except :
				print ("error ip6, conversion %s to hex failed " % (i))
				break
				
			if value < 0xffff and value >= 0 :
				continue;
			else: 
				rv = "error ip6 value %d out of range " % (value)
				break
	else:
		rv="ip4"
		ip = ipstring.split(".")
		print (ip)
		
		if len(ip) == 4 : 
			for i in ip :
				try :
					value = int(i,10)
				except : 
					print ("error ip4 conversion %s to base 10 failed " % (i))
					break
				
				print (value)
				if (value <= 255 and value >= 0) :
					continue;
				else: 
					rv = "error ip4 value %d out of range " % (value)
					break
		else:
			rv= "error"

	return rv


def main():
	print (isIp("2001:0db8:0000:0000:0000:ff00:0042:8329"))
	print (isIp("121.18.19.20"))

	print (isIp("2x01:0yb8:0a00:0000:0000:ff00:0042:8329"))
	print (isIp("1233.18.0.277"))

	print (isIp(":::::::"))
	print (isIp("0.0.0.0"))
	
	
#------------------------------------------------------------------------------------------
#This idiom means the below code only runs when executed from command line
if __name__ == '__main__':
    main()			
	




	
