import sys
import os
import shutil
import re

exepath=r"~/Documents/DSI/AstroPhotoProcessing/"
exepath2=r"~/Documents/DSI/CMeadeDSIColor/"

# Assume this directory exists:
# image, flat, darkflat, flat

path = sys.argv[1]

os.chdir(path)

# Create masters
cmd = exepath + 'Experimental/LowMem/median dark masterdark.fits' 
os.system(cmd)

cmd = exepath + 'Experimental/LowMem/median darkflat masterdarkflat.fits' 
os.system(cmd)

cmd = exepath + 'Experimental/LowMem/median flat mflat.fits'
os.system(cmd)

cmd = exepath + 'OffsetDark/substract mflat.fits masterdarkflat.fits masterflat.fits'
os.system(cmd)

# Substract masterdark
if not os.path.isdir("step1"):
	os.mkdir("step1")

for root, dirs, files in os.walk("image"):
	for name in files:
		source = root + os.sep + name
		base,ext = os.path.splitext(name)
		if ext == ".fits":
			cmd = exepath + 'OffsetDark/substract ' + source + " " + "masterdark.fits " + "step1" + os.sep + name
			os.system(cmd)
# Divide by masterflat
if not os.path.isdir("step2"):
	os.mkdir("step2")

for root, dirs, files in os.walk("step1"):
	for name in files:
		source = root + os.sep + name
		base,ext = os.path.splitext(name)
		if ext == ".fits":
			cmd = exepath + 'Flat/divide ' + source + " " + "masterflat.fits " + "step2" + os.sep + name
			os.system(cmd)

# Split R G B and L channel
if not os.path.isdir("red"):
	os.mkdir("red")
	
if not os.path.isdir("green"):
	os.mkdir("green")
	
if not os.path.isdir("blue"):
	os.mkdir("blue")
	
if not os.path.isdir("luminance"):
	os.mkdir("luminance")


for root, dirs, files in os.walk("step2"):
	for name in files:
		source = root + os.sep + name
		base,ext = os.path.splitext(name)
		if ext == ".fits":
			cmd = exepath2 + 'cmyg2rgb ' + source
			os.system(cmd)
			shutil.move("red.fits","red/"+base+".fits")
			shutil.move("green.fits","green/"+base+".fits")
			shutil.move("blue.fits","blue/"+base+".fits")
			shutil.move("luminance.fits","luminance/"+base+".fits")

# Rename fits (for Siril)
rx = re.compile("_([^_]*)$")

for color in ["luminance","red","green","blue"]:
	directory = path + os.sep + color
	for root, dirs, files in os.walk(directory):
		for name in files:
			source = root + os.sep + name
			base,ext = os.path.splitext(name)
			if ext == ".fits":
				print name,base
				md = rx.search(base)
				if md:
					num = md.group(1)
					fname = directory + os.sep + color + "_%03d" % int(num) + ".fits"
					shutil.move(source,fname)
			
