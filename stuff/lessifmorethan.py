#!/usr/bin/env python
import sys, os
newin = os.fdopen(sys.stdin.fileno(), 'r', 1)
sys.stdin = newin
n = 0
lines = []
for line in sys.stdin:
    n += 1
    if (n > int(sys.argv[1])):
	print "meh, calling less ===================================================================================================================================================="
	o = os.popen("less -R", "w")
	for l in lines:
	    o.write(l)
	for line in sys.stdin:
	    o.write(line)
	exit()

    sys.stdout.write(line)
    lines.append(line)



#i guess our input just decides to not be line-buffered since its connected to a pipe, not terminal...so the output is delayed..:|