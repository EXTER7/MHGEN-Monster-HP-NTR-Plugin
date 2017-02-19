#!/usr/bin/python
import sys
import os
import os.path
import ftplib
import glob
import errno 
import shutil

DEVKITARM_ERROR = "DevKitARM is not properly set up.\n Make sure the DEVKITARM environment variable points to the DevKitARM directory.\n"

def copy(files,dest):
  for file in glob.glob(files):
    shutil.copy(file, dest)

def delete(files):
  for file in glob.glob(files):
    os.remove(file)

def mkdirp(path):
  try:
    os.makedirs(path)
  except OSError as exc:  # Python >2.5
    if exc.errno == errno.EEXIST and os.path.isdir(path):
      pass
    else:
      raise

def file_check(file,message):
  if not os.path.isfile(file):
    print("ERROR: " + os.path.basename(file) +" not found.\n")
    print(message)
    exit(1)


DEVKITARM = str(os.getenv("DEVKITARM"))

CC = DEVKITARM + "/bin/arm-none-eabi-gcc"
CP = DEVKITARM + "/bin/arm-none-eabi-g++"
OC = DEVKITARM + "/bin/arm-none-eabi-objcopy" 
LD = DEVKITARM + "/bin/arm-none-eabi-ld"
LIBPATH = '-L./lib'


def allFile(pattern):
    s = "";
    for file in glob.glob(pattern):
        s += file + " "
    return s

def run(cmd):
	os.system(cmd)

def build(region,outpath):
   cwd = os.getcwd() 
   delete(r'*.o')
   delete(r'obj/*.o')
   run(CC+  " -s  -g -I include -I include/libntrplg -D GAME_REGION_" + region+"=1 " + allFile('source/libntrplg/*.c') + allFile('source/ns/*.c') + allFile('source/*.c') + allFile('source/libctru/*.c') + " -c  -march=armv6 -mlittle-endian  ")
   run(CC+"  -Os " + allFile('source/libntrplg/*.s') +  allFile('source/ns/*.s')  + allFile('source/*.s') + allFile('source/libctru/*.s') + " -c -s -march=armv6 -mlittle-endian ")

   run(LD + ' ' + LIBPATH + " -pie --print-gc-sections  -T 3ds.ld -Map=homebrew.map " + allFile("*.o") + " -lc -lgcc --nostdlib"  )
   copy(r'*.o','obj/')
   copy(r'a.out','bin/homebrew.elf')
   run(OC+" -O binary a.out payload.bin -S")
   delete(r'*.o')
   delete(r'*.out')
   copy(r'payload.bin',outpath)
   delete(r'a.out')
   delete(r'bin/homebrew.elf')
   delete(r'obj/*.o')

file_check(CC,DEVKITARM_ERROR)
file_check(CP,DEVKITARM_ERROR)
file_check(LD,DEVKITARM_ERROR)
file_check(OC,DEVKITARM_ERROR)

mkdirp("plugin/0004000000187000")
mkdirp("plugin/0004000000185B00")
mkdirp("plugin/0004000000155400")
build("USA","plugin/0004000000187000/mhgen.plg")
build("EUR","plugin/0004000000185B00/mhgen.plg")
build("JPN","plugin/0004000000155400/mhx.plg")

