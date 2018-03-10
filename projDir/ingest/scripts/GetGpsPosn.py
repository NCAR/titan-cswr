#!/usr/bin/env python

#===========================================================================
#
# Get GPS posn from Spectracom, write to file in XML
#
#===========================================================================

import os
import sys
import time
import datetime
from datetime import timedelta
import string
#import paramiko
import subprocess
from optparse import OptionParser

def main():

    global options
    global appName
    global nowTimeStr
    global unixTimeStr

    appName = "GetGpsPosn.py"

    # parse the command line

    parseArgs()

    if (options.debug == True):
        print >>sys.stderr, appName, \
            " reads postion from GPS and writes it to a specified file in XML"

    # initialize
    
    epoch = datetime.datetime(1970,1,1,0,0,0)
    startTime = time.gmtime()
    start = datetime.datetime(startTime.tm_year,
                              startTime.tm_mon,
                              startTime.tm_mday,
                              startTime.tm_hour,
                              startTime.tm_min,
                              startTime.tm_sec)
    startTimeStr = start.strftime("%Y/%m/%d-%H:%M:%S")
    startTimeDelta = start - epoch
    startTimeSecs = startTimeDelta.days * 86400 + startTimeDelta.seconds
    startUnixStr = str(startTimeSecs)

    beginStr = "BEGIN: " + appName
    beginStr += " at " + startTimeStr

    if (options.debug == True):
        print "========================================================"
        print beginStr
        print "========================================================"

    # create dirs if necessary

    if not os.path.exists(options.tmpDir):
        runCommand("mkdir -p " + options.tmpDir)

    if not os.path.exists(options.outputDir):
        runCommand("mkdir -p " + options.outputDir)

    # run

    prevRegTime = 0.0

    while (True):

        # get current time
        
        nowTime = time.gmtime()
        now = datetime.datetime(nowTime.tm_year,
                                nowTime.tm_mon,
                                nowTime.tm_mday,
                                nowTime.tm_hour,
                                nowTime.tm_min,
                                nowTime.tm_sec)
        nowTimeStr = now.strftime("%Y/%m/%dT%H:%M:%S")
        unixTimeStr = now.strftime("%s")
        unixTime = float(unixTimeStr);
        
        if (options.debug == True):
            print "================================="
            print "now: ", nowTimeStr
            print "unixTime: ", unixTimeStr
        
        # get position

        getPosition()

        # register with procmap

        if ((unixTime - prevRegTime) >= 30.0):
            cmd = "procmap_register"
            cmd = cmd + " -instance " + options.instance
            cmd = cmd + " -name " + appName
            cmd = cmd + " -pid " + str(os.getpid())
            cmd = cmd + " -start " + startUnixStr
            runCommand(cmd)
            prevRegTime = unixTime
        
        # sleep
        
        time.sleep(float(options.sleepSecs))

    sys.exit(0)


########################################################################
# Parse the command line

def parseArgs():
    
    global options

    defaultOutputDir = "/tmp/gps_posn"
    defaultTmpDir = "/tmp/gps_tmp"
    defaultFileName = "GPS_POSN"
    defaultCommand = "ssh spadmin@ntp /usr/cli/gpsloc"

    # parse the command line

    usage = "usage: %prog [options]"
    parser = OptionParser(usage)
    parser.add_option('--debug',
                      dest='debug', default='False',
                      action="store_true",
                      help='Set debugging on')
    parser.add_option('--outputDir',
                      dest='outputDir',
                      default=defaultOutputDir,
                      help='Path of output directory')
    parser.add_option('--tmpDir',
                      dest='tmpDir',
                      default=defaultTmpDir,
                      help='Path of tmp directory')
    parser.add_option('--fileName',
                      dest='fileName',
                      default=defaultFileName,
                      help='Position file name')
    parser.add_option('--ntpServer',
                      dest='ntpServer',
                      default='ntp',
                      help='Host name for ntp server')
    parser.add_option('--command',
                      dest='command',
                      default=defaultCommand,
                      help='Command to get position')
    parser.add_option('--instance',
                      dest='instance',
                      default='primary',
                      help='Instance for registration with procmap')
    parser.add_option('--sleepSecs',
                      dest='sleepSecs',
                      default=2,
                      help='How log to sleep between checks(secs)')

    (options, args) = parser.parse_args()

    if (options.debug == True):
        print >>sys.stderr, "Options:"
        print >>sys.stderr, "  debug? ", options.debug
        print >>sys.stderr, "  outputDir: ", options.outputDir
        print >>sys.stderr, "  tmpDir: ", options.tmpDir
        print >>sys.stderr, "  fileName: ", options.fileName
        print >>sys.stderr, "  command: ", options.command
        print >>sys.stderr, "  sleepSecs: ", options.sleepSecs

########################################################################
# Get position
# Write to file in XML

def getPosition():

    global latitudeDeg
    global longitudeDeg
    global altitudeM

    # query GPS for position

    sshCommand = "ssh spadmin@" + options.ntpServer + " /usr/cli/gpsloc"

    pipe = subprocess.Popen(sshCommand, shell=True,
                            stdout=subprocess.PIPE).stdout
    lines = pipe.readlines()

    latitudeDeg = -9999.0
    longitudeDeg = -9999.0
    altitudeM = -9999.0

    for line in lines:
        line = line.rstrip()
        if (options.debug == True):
            print >>sys.stderr, line

        if (line.find("Lat") >= 0):
            parts = line.split()
            if (len(parts) == 5):
                latitudeDeg = \
                    (float(parts[2]) + float(parts[3]) / 60.0 +
                     float(parts[4]) / 3600.0)
                if (parts[1] == "S"):
                    latitudeDeg = latitudeDeg * -1.0
            if (options.debug == True):
                print >>sys.stderr, "latitude(deg): ", latitudeDeg

        elif (line.find("Lon") >= 0):
            parts = line.split()
            if (len(parts) == 5):
                longitudeDeg = \
                    (float(parts[2]) + float(parts[3]) / 60.0 +
                     float(parts[4]) / 3600.0)
                if (parts[1] == "W"):
                    longitudeDeg = longitudeDeg * -1.0
            if (options.debug == True):
                print >>sys.stderr, "longitude(deg): ", longitudeDeg
            
        elif (line.find("Alt") >= 0):
            parts = line.split()
            if (len(parts) == 3):
                altitudeM = float(parts[1])
            if (options.debug == True):
                print >>sys.stderr, "altitudeM: ", altitudeM

    # ensure tmp and output dirs exist

    try:
        os.makedirs(options.outputDir)
        os.makedirs(options.tmpDir)
    except Exception as ee:
        if (options.debug == True):
            print >>sys.stderr, "WARNING: makeing dirs for output files"
            print >>sys.stderr, "  ", ee

    # open tmp file

    tmpFileName = "getGpsPosn_tmp_" + str(os.getpid());
    tmpPathName = os.path.join(options.tmpDir, tmpFileName)

    if (options.debug == True):
        print >>sys.stderr, "Writing to tmp file: ", tmpPathName

    tmpFile = None
    try:
        tmpFile = open(tmpPathName, "w")
    except Exception as ee:
        print >>sys.stderr, "ERROR - opening tmp file: ", tmpPathName
        print >>sys.stderr, "  ", ee
        return

    # write tmp file

    tmpFile.write("<GpsPosn>\n")
    tmpFile.write("  <Time>{0}</Time>\n".format(nowTimeStr))
    tmpFile.write("  <UnixSecs>{0}</UnixSecs>\n".format(unixTimeStr))
    tmpFile.write("  <LatitudeDeg>{0}</LatitudeDeg>\n".format(latitudeDeg))
    tmpFile.write("  <LongitudeDeg>{0}</LongitudeDeg>\n".format(longitudeDeg))
    tmpFile.write("  <AltitudeM>{0}</AltitudeM>\n".format(altitudeM))
    tmpFile.write("  <Command>{0}</Command>\n".format(options.command))
    tmpFile.write("</GpsPosn>\n")
    tmpFile.close()

    # move to final path
            
    outputPathName = os.path.join(options.outputDir, options.fileName)
    try:
        os.rename(tmpPathName, outputPathName)
    except Exception as ee:
        print >>sys.stderr, "ERROR - rename tmp file: ", tmpPathName
        print >>sys.stderr, "         to output file: ", outputPathName
        print >>sys.stderr, "  ", ee
        return

    if (options.debug == True):
        print >>sys.stderr, "Renamed tmp file to output file: ", outputPathName

########################################################################
# Run a command in a shell, wait for it to complete

def runCommand(cmd):

    if (options.debug == True):
        print >>sys.stderr, "-----------------------------"
        print >>sys.stderr, "running cmd:",cmd
    
    try:
        retcode = subprocess.call(cmd, shell=True)
        if retcode < 0:
            print >>sys.stderr, "Child was terminated by signal: ", -retcode
        else:
            if (options.debug == True):
                print >>sys.stderr, "Child returned code: ", retcode
    except OSError, e:
        print >>sys.stderr, "Execution failed:", e

########################################################################
# kick off main method

if __name__ == "__main__":

   main()
