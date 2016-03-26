#!/usr/bin/env python 

import sys


class Fib:
    '''iterator that yields numbers in the Fibonacci sequence'''

    def __init__(self, max):
        self.max = max

    def __iter__(self):
        self.a = 0
        self.b = 1
        return self

    def __next__(self):
        fib = self.a
        if fib > self.max:
            raise StopIteration
        self.a, self.b = self.b, self.a + self.b
        return fib


def main():
	total = len(sys.argv)
	cmdargs = str(sys.argv)
	print ("The total numbers of args passed to the script: %d " % total)
	print ("Args list: %s " % cmdargs)
	print ("Script name: %s" % str(sys.argv[0]))
	for i in xrange(total):
		print ("Argument # %d : %s" % (i, str(sys.argv[i])))		

	print "type of {} ".format(type(sys.argv))
	slice = sys.argv[1:]

	print slice
	for ab in slice:
		print str(ab)
		for n in Fib(int(ab)):
			print(n)

#------------------------------------------------------------------------------------------
#This idiom means the below code only runs when executed from command line
if __name__ == '__main__':
    main()		