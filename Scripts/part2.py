import sys
import os
import shutil
import re

exepath=r"~/Documents/DSI/AstroPhotoProcessing/"

path = sys.argv[1]

# manually realign each image

c = exepath + "Experimental/Align/movexy "

# TO DO image offset must be computed before ...
ax = [  3,  3,  3,  3,  2,  2,  2,  1,  0,  0, 0, -1, -1, -1, -1, -1, -2, -3, -3, -3]
ay = [-21,-18,-16,-15,-11,-10, -8, -4, -2,  0, 1,  4,  7,  8, 11, 14, 17, 19, 21, 23]

size = len(ax)

for color in ["luminance","red","green","blue"]:
	directory = path + os.sep + color
	path1 = directory + "_aligned"
	if not os.path.isdir(path1):
		os.mkdir(path1)
	for i in range(size):
		ix = "%d %d" % (ax[i],ay[i])
		cmd = c + directory + os.sep + color + "_%03d.fits " % i + path1 + os.sep + color + "_%03d.fits " % i + ix
		print cmd
		os.system(cmd)
	# stacking by color
	c2 = exepath + "Experimental/LowMem/median "
	cmd = c2 + path1 + " " + path + os.sep + color +".fits"
	os.system(cmd)


