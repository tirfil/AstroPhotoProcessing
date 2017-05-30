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
			
		
os.mkdir("step1")

for root, dirs, files in os.walk("light"):
	
	for name in files:
		source = root + os.sep + name
		target = "step1" + os.sep + name
		cmd = "./bin/substract %s %s %s" % (source,"masterdark.fits",target);
		print cmd
		os.system(cmd)
		

if os.path.isfile("masterflat.fits"):
	os.mkdir("step2")
	for root, dirs, files in os.walk("step1"):
		for name in files:
			source = root + os.sep + name
			target = "step2" + os.sep + name
			cmd = "./bin/divide %s %s %s" % (source, "masterflat.fits", target)
			print cmd
			os.system(cmd)



if os.path.isdir("step2"):
	sourcedir = "step2";
else:
	sourcedir = "step1";

os.mkdir("red")
os.mkdir("green")
os.mkdir("blue")
os.mkdir("luminance")

for root, dirs, files in os.walk(sourcedir):
	for name in files:
		source = root + os.sep + name
		cmd = "./bin/cmyg2rgb %s " % source
		print cmd
		os.system(cmd)
		for item in ["red","green","blue","luminance"]:
			target = item + os.sep + name
			source = item + ".fits"
			shutil.move(source,target)


for sourcedir in ["red","green","blue","luminance"]:
	first = True
	dest = sourcedir + "_align"
	os.mkdir(dest)
	for root, dirs, files in os.walk(sourcedir):
		for name in files:
			source = root + os.sep + name
			if first:
				reference = source
				first = False
			target = dest + os.sep + name
			cmd = "./bin/center %s %s %s 80" % (reference, source, target)
			print cmd
			os.system(cmd)
		
for sourcedir in ["red","green","blue","luminance"]:
	source = sourcedir + "_align"
	target = "master" + sourcedir + ".fits"
	cmd = "./bin/median %s %s" % (source,target)
	print cmd
	os.system(cmd)


cmd = "ds9 -rgb -red masterred.fits -green mastergreen.fits -blue masterblue.fits"
os.system(cmd)

		

		
		
