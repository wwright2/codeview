#!/usr/bin/env python3 

import sys


class Fib:
    '''iterator that yields numbers in the Fibonacci sequence'''

    def __init__(self, max):
        self.max = max

    def __iter__(self):
        self.a = 1
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
	print ("The total numbers of args passed to the script: {0:g} ".format(total))
	print ("Args list: {} ".format(cmdargs))
	print ("Script name: {}".format(sys.argv[0]))
	#print ("type of {} ".format(type(sys.argv)))
	slice = sys.argv[1:]

	print ("\n")
	for ab in slice:
		print ("The fibonacci series for {} ".format (ab))
		for n in Fib(int(ab)):
			if not n :
				pass
			else:
				print ("{} ".format(n),end="")
		print ("\n")
			

#------------------------------------------------------------------------------------------
#This idiom means the below code only runs when executed from command line
if __name__ == '__main__':
    main()		