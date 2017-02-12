#!/usr/bin/python
import sys
import os
import ftplib
import glob


CC = "arm-none-eabi-gcc"
CP = "arm-none-eabi-g++"
OC = "arm-none-eabi-objcopy" 
LD = "arm-none-eabi-ld"
CTRULIB = '../libctru'
LIBPATH = '-L ./lib'

def allFile(pattern):
    s = "";
    for file in glob.glob(pattern):
        s += file + " "
    return s

def run(cmd):
	os.system(cmd)

def build(region,outpath):
   cwd = os.getcwd() 
   run("rm -f *.o obj/*.o")
   run(CC+  " -s  -g -I include -I include/libntrplg -D GAME_REGION_" + region+"=1 " + allFile('source/libntrplg/*.c') + allFile('source/ns/*.c') + allFile('source/*.c') + allFile('source/libctru/*.c') + " -c  -march=armv6 -mlittle-endian  ")
   run(CC+"  -Os " + allFile('source/libntrplg/*.s') +  allFile('source/ns/*.s')  + allFile('source/*.s') + allFile('source/libctru/*.s') + " -c -s -march=armv6 -mlittle-endian ")

   run(LD + ' ' + LIBPATH + " -pie --print-gc-sections  -T 3ds.ld -Map=homebrew.map " + allFile("*.o") + " -lc -lgcc --nostdlib"  )
   run("cp -r *.o obj/ ")
   run("cp a.out bin/homebrew.elf ")
   run(OC+" -O binary a.out payload.bin -S")
   run("rm -f *.o")
   run("rm -f *.out")
   run('cp payload.bin ' + outpath)
   run("rm -f a.out bin/homebrew.elf ")

run("mkdir -p plugin/0004000000187000")
run("mkdir -p plugin/0004000000185B00")
run("mkdir -p plugin/0004000000155400")
build("USA","plugin/0004000000187000/mhgen.plg")
build("EUR","plugin/0004000000185B00/mhgen.plg")
build("JPN","plugin/0004000000155400/mhx.plg")

