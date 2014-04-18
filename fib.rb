#!/usr/local/bin/ruby -w

def fibinacci(limit = nil)
  seed1 = 0
  seed2 = 1
  outstr= []
  while not limit or seed2 <= limit
    outstr << seed2
    seed1,seed2 = seed2, seed1 + seed2
  end
  return outstr
end


if ARGV.length == 0
	abort "\nNothing to do, no arguments. \n\n"
end

ARGV.each { |x| 
  #fibinacci(x.to_i) 
  puts fibinacci(x.to_i).join(" ")
  } 

#fibinacci(x) { |x| puts x }
