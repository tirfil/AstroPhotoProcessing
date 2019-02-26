import sys
import os
import shutil
import re


def mkdir(d):
	if not os.path.isdir(d):
		os.mkdir(d)
		
def w(d):
	return "cr2" + os.sep + d

if not os.path.isdir("cr2"):
	print "No cr2"
	exit(0);

def convert(directory):
	if os.path.isdir(w(directory)):
		os.mkdir(directory)
		for root, dirs, files in os.walk(w(directory)): 
			for name in files:
				base,ext = os.path.splitext(name)
				if (ext != ".pgm"):
					source = root + os.sep + name
					cmd = "dcraw -t 0 -d -4 %s" % source
					os.system(cmd)
					source = root + os.sep + base + ".pgm"
					target = directory + os.sep + base + ".fits"	
					cmd = "./bin/pgm2fits %s %s" % (source,target)
					os.system(cmd)
					
convert("light")
convert("dark")
convert("flat")
convert("darkflat")
						
			
