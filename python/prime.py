import  math

class Prime:
        def __init__(self, max):
                self.max = max
                self.plist = []
                self.value = 1

        def __iter__(self):
                return self

        def __next__(self):

                found=False

                #print (self.plist)
                if (self.max < 3):
                        return 1

                if ( len(self.plist) == 0):
                        prime = self.value
                        self.plist.append(prime)
                        return prime
                else:
                        while ( not found):
                                self.value += 2
                                found = True
                                for n in self.plist :
                                        if (self.value > self.max):
                                                raise StopIteration
                                        if (len(self.plist) == 1):
                                                break
                                        elif n > math.sqrt(self.value) :
                                                break
                                        # Not dividing by 1 and if divide evenly then not a prime, next
                                        elif ( n > 1 and ((self.value % n) == 0) ) :
                                                found = False  # incr value and retest for prime
                                                break;

                        # found next append to list.
                        prime = self.value

                        self.plist.append(prime)
                        return prime


#----------------TEST CODE ------------
import sys
import argparse
import os

#------------------------------------------------------------------------------------------
# Main() execute the plan
#

defaultPrime=13

def main():
        parser = argparse.ArgumentParser(description='Computation for Primes, given: arguments')
        args = getArgs(parser)

        #print("args=",args)
        for n in Prime(defaultPrime):
                print(n, end=' ')

        # Exit

        print ("\n...printed me primes upto %d...\n" % defaultPrime)


#------------------------------------------------------------------------------------------
# getArgs from command line.
def getArgs(parser):
#    parser.add_argument("-p", "--prime", type=int,  help="Integer value gt 0  "+ defaultPrime)
#    args = parser.parse_args()
        global defaultPrime
        if sys.argv[1] :
                defaultPrime = int(sys.argv[1])

#This idiom means the below code only runs when executed from command line
if __name__ == '__main__':
    main()
