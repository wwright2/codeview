#!/usr/local/bin/ruby -w

include Math

class PrimeNumber

 
  #------
  def primes(limit = nil)
    if limit < 1
      puts "limit is less than 1 exiting"
    end
    
    seed = 1
    outarr= []
    flag=1

    nsqrt = 0.0
   
    while seed <= limit
      nsqrt = Math.sqrt(seed) +1
      flag = 1
      outarr.each { |x|
        if (x > 1) and (seed % x == 0)
            flag = 2
            break
        end
        if ( x > nsqrt )
          flag = 3 
          outarr << seed
          break;
        end
      }
      if (flag == 1)
        outarr << seed
      end
      
      seed = seed + 2
    end
    
    return outarr
  end

end

if ARGV.length == 0
        abort "\nNothing to do, no arguments. \n\n"
end

ARGV.each { |x| 
  puts PrimeNumber.new.primes(x.to_i).join(" ")
  }
  
