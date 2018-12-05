import sys
import os
import shutil
import re


def mkdir(d):
	if not os.path.isdir(d):
		os.mkdir(d)
		
def w(d):
	return "work" + os.sep + d

mkdir("work")

if not os.path.isdir("light"):
	print "No light"
	exit(0);
	
if os.path.isfile(w("masterdark.fits")):
	pass
else:
	if os.path.isdir("dark"):
		cmd = "./bin/median %s %s" % ("dark",w("masterdark.fits"))
		print cmd
		os.system(cmd)
		
if os.path.isfile(w("masterdarkflat.fits")):
	pass
else:
	if os.path.isdir("darkflat"):
		cmd = "./bin/median %s %s" % ("darkflat",w("masterdarkflat.fits"))
		print cmd
		os.system(cmd)
		
		
print("--- remove dark from light");
mkdir(w("lmd"))
directory = "lmd"
for root, dirs, files in os.walk("light"):
	for name in files:
		source = root + os.sep + name
		target = directory + os.sep + name
		cmd = "./bin/substract %s %s %s" % (source,w("masterdark.fits"),w(target))		
		os.system(cmd)
		
if os.path.isdir("flat") and os.path.isfile(w("masterdarkflat.fits")):		
	print("--- remove dark from flat");
	mkdir(w("fmd"))
	directory = "fmd"
	for root, dirs, files in os.walk("flat"):
		for name in files:
			source = root + os.sep + name
			target = directory + os.sep + name
			cmd = "./bin/substract %s %s %s" % (source,w("masterdarkflat.fits"),w(target))		
			os.system(cmd)
		
		
mkdir(w("red"))
mkdir(w("green"))
mkdir(w("blue"))

print("---- split light color\n")

for root, dirs, files in os.walk(w("lmd")):
	for name in files:
		source = root + os.sep + name
		cmd = "./bin/bayer2rgb %s" % source
		os.system(cmd)
		for color in ["red","green","blue"]:
			source = color + ".fits"
			target = color + os.sep + name
			shutil.move(source,w(target))
			
			
print("---- split flat color\n")

if os.path.isdir(w("fmd")):
	mkdir(w("redflat"))
	mkdir(w("greenflat"))
	mkdir(w("blueflat"))
	for root, dirs, files in os.walk(w("fmd")):
		for name in files:
			source = root + os.sep + name
			cmd = "./bin/bayer2rgb %s" % source
			os.system(cmd)
			for color in ["red","green","blue"]:
				source = color + ".fits"
				target = color + "flat" + os.sep + name
				shutil.move(source,w(target))
				
print("---- make master flats \n")
if os.path.isdir(w("fmd")):
	for directory in ["redflat","greenflat","blueflat"]:				
		if os.path.isdir(w(directory)):
			tempo = "master" + directory + ".fits"
			cmd = "./bin/median %s %s" % (w(directory),w(tempo))
			os.system(cmd)
			#target = "master" + directory + ".fits"
			#cmd = "./bin/substract %s %s %s" % (w(tempo),w("masterdarkflat.fits"),w(target))		
			#os.system(cmd)
		
##print("---- make light minus dark\n")

##for color in ["red","green","blue"]:
	##directory = color + "1"
	##mkdir(w(directory))	
	##for root, dirs, files in os.walk(w(color)):
		##for name in files:
			##source = root + os.sep + name
			##target = directory + os.sep + name
			##cmd = "./bin/substract %s %s %s" % (source,w("masterdark.fits"),w(target))		
			##os.system(cmd)

print("---- divide by flat\n")			
if os.path.isdir(w("fmd")):					
	for color in ["red","green","blue"]:
		flat = "master" + color + "flat.fits"
		directory1 = color
		directory2 = color + "1"
		mkdir(w(directory2))
		for root, dirs, files in os.walk(w(directory1)):
			for name in files:
				source = root + os.sep + name
				target = directory2 + os.sep + name
				cmd = "./bin/divide %s %s %s" % (source, w(flat), w(target))
				os.system(cmd)
			
##align
print("---- align lights\n")

for color in ["red","green","blue"]:
	directory2 = color + "2"
	mkdir(w(directory2))
	
start = "red"
first = True

for root, dirs, files in os.walk(w(start)):
	for name in files:
		source = root + os.sep + name
		if first:
			first = False
			if os.path.isfile(w("ref.dat")):
				os.remove(w("ref.dat"))
			cmd= "./bin/center reference %s %s" % (source,w("ref.dat"))
			os.system(cmd)
			for color in ["red","green","blue"]:
				if os.path.isdir(w("fmd")):
					directory1 = color + "1"
				else:
					directory1 = color 
				directory2 = color + "2"
				source = w(directory1) + os.sep + name
				target = w(directory2) + os.sep + name
				shutil.copyfile(source,target)
		elif os.path.isfile(w("ref.dat")):
			if os.path.isfile(w("coeff.dat")):
				os.remove(w("coeff.dat"))
			cmd = "./bin/center compare %s %s %s" % (source,w("ref.dat"),w("coeff.dat"))
			os.system(cmd)
			if os.path.isfile(w("coeff.dat")):
				for color in ["red","green","blue"]:
					directory1 = color
					directory2 = color + "2"
					source = w(directory1) + os.sep + name
					target = w(directory2) + os.sep + name
					cmd = "./bin/center translate %s %s %s" %(source,w("coeff.dat"),target)
					os.system(cmd)
				
			
		

#first = True
#for color in ["red","green","blue"]:
	#directory1 = color
	#directory2 = color + "2"
	#mkdir(w(directory2))
	#for root, dirs, files in os.walk(w(directory1)):
		#for name in files:
			#source = root + os.sep + name		
			#if first:
				#first = False
				#refname = name
			#reference = root + os.sep + refname
			#target = directory2 + os.sep + name
			#cmd = "./bin/center %s %s %s" % (reference, source, w(target))
			#os.system(cmd)
			
for color in ["red","green","blue"]:
	directory = color + "2"
	target = "master" + color + ".fits"
	cmd = "./bin/median %s %s" % (w(directory),w(target))
	os.system(cmd)
	
cmd = "ds9 -rgb -red work/masterred.fits -green work/mastergreen.fits -blue work/masterblue.fits"
os.system(cmd)
		
			
		
		
