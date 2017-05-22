import sys
import os
import shutil
import re

if not os.path.isdir("light"):
	print "No light"
	exit(0);
	
if os.path.isfile("masterdark.fits"):
	pass
else:
	if os.path.isdir("dark"):
		cmd = "./bin/median %s %s" % ("dark","masterdark.fits")
		print cmd
		os.system(cmd)
		
if os.path.isfile("masterdarkflat.fits"):
	pass
else:
	if os.path.isdir("darkflat"):
		cmd = "./bin/median %s %s" % ("darkflat","masterdarkflat.fits")
		print cmd
		os.system(cmd)
		
if os.path.isfile("masterflat.fits"):
	pass
else:
	if os.path.isdir("flat"):
		cmd = "./bin/median %s %s" % ("flat","tmp.fits")
		print cmd
		os.system(cmd)
		if os.path.isfile("masterdarkflat.fits"):
			cmd = "./bin/substract %s %s %s" % ("tmp.fits","masterdarkflat.fits","masterflat.fits");
			print cmd
			os.system(cmd)
		os.remove("tmp.fits")
		
cmd = "./bin/cmyg2lum %s %s" % ("masterdark.fits","md.fits");
print cmd
os.system(cmd)
cmd = "./bin/cmyg2lum %s %s" % ("masterflat.fits","mf.fits");
print cmd
os.system(cmd)
			
os.mkdir("step1")

for root, dirs, files in os.walk("light"):	
	for name in files:
		source = root + os.sep + name
		target = "step1" + os.sep + name
		cmd = "./bin/cmyg2lum %s %s" % (source,target)
		print cmd
		os.system(cmd)	
		
os.mkdir("step2")		
for root, dirs, files in os.walk("step1"):		
	for name in files:
		source = root + os.sep + name
		target = "step2" + os.sep + name
		cmd = "./bin/substract %s %s %s" % (source,"md.fits",target);
		print cmd
		os.system(cmd)				


if os.path.isfile("mf.fits"):
	os.mkdir("step3")
	for root, dirs, files in os.walk("step2"):
		for name in files:
			source = root + os.sep + name
			target = "step3" + os.sep + name
			cmd = "./bin/divide %s %s %s" % (source, "mf.fits", target)
			print cmd
			os.system(cmd)

first = True

if os.path.isdir("step3"):
	sourcedir = "step3";
else:
	sourcedir = "step2";


os.mkdir("step4")

for root, dirs, files in os.walk(sourcedir):
	for name in files:
		source = root + os.sep + name
		if first:
			reference = source
			first = False
		target = "step4" + os.sep + name
		cmd = "./bin/align %s %s %s" % (source, reference, target)
		print cmd
		os.system(cmd)
		
cmd = "./bin/median %s %s" % ("step4","result.fits")
print cmd
os.system(cmd)

cmd = "./bin/cmyg2fits result.fits"
print cmd
os.system(cmd)

cmd = "./bin/cmyg2png result.fits"
print cmd
os.system(cmd)


		

		
		
