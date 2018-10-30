#!/usr/bin/env python

#===========================================================================
#
# Put data to Osp center in Relampago
#
# Triggered by LdataWatcher
#
#===========================================================================

import os
import sys
from stat import *
import time
import datetime
from datetime import timedelta
import string
import ftplib
import subprocess
from optparse import OptionParser

def main():

    global options
    global ftpServer
    global ftpUser
    global ftpPassword
    global ftpDebugLevel

    ftpServer = "181.30.169.202"
    ftpUser = "relopsftp"
    ftpPassword = "rayosyyyy"
    
    # parse the command line

    parseArgs()

    # initialize
    
    beginString = "BEGIN: putDataToCatalog.py"
    today = datetime.datetime.now()
    beginString += " at " + str(today)

    if (options.debug == True):
        print >>sys.stderr, "=============================================="
        print >>sys.stderr, beginString
        print >>sys.stderr, "=============================================="

    #   compute valid time string

    validTime = time.gmtime(int(options.validTime))
    year = int(validTime[0])
    month = int(validTime[1])
    day = int(validTime[2])
    hour = int(validTime[3])
    min = int(validTime[4])
    sec = int(validTime[5])
    validDayStr = "%.4d%.2d%.2d" % (year, month, day)
    validTimeStr = \
        "%.4d%.2d%.2d%.2d%.2d%.2d" % (year, month, day, hour, min, sec)

    if (options.debug == True):
        print "validDayStr: ", validDayStr
        print "validTimeStr: ", validTimeStr
        print "file exists: ", os.path.exists(options.fullFilePath)

    # send the file to the ftp server
    
    ftpFile(options.fileName, options.fullFilePath, validDayStr)

    if (options.debug == True):
        print >>sys.stderr, "========================================="
        print >>sys.stderr, \
              "END: putDataToCatalog.py at " + str(datetime.datetime.now())
        print >>sys.stderr, "========================================="

    sys.exit(0)

########################################################################
# Ftp the file

def ftpFile(fileName, filePath, validDayStr):

    # set ftp debug level

    if (options.debug == True):
        ftpDebugLevel = 2
    else:
        ftpDebugLevel = 0

    targetDir = os.path.join('upload', options.radarName)
    
    if (options.debug == True):
        print "targetDir: ", targetDir
    
    # open ftp connection
    
    ftp = ftplib.FTP(ftpServer, ftpUser, ftpPassword)
    ftp.set_debuglevel(ftpDebugLevel)

    # make target dir

    try:
        ftp.mkd(targetDir)
    except ftplib.all_errors:
        pass

    # go to target dir

    if (options.debug == True):
        print >>sys.stderr, "ftp cwd to: " + targetDir
    
    ftp.cwd(targetDir)

    # put the file to a tmp name

    tmpName = fileName + ".tmp"

    if (options.debug == True):
        print >>sys.stderr, "putting file: ", filePath

    fp = open(filePath, 'rb')
    ftp.storbinary('STOR ' + tmpName, fp)

    # rename file

    ftp.rename(tmpName, fileName)
    
    # close ftp connection
                
    ftp.quit()

    return

########################################################################
# Parse the command line

def parseArgs():
    
    global options

    # parse the command line

    radarName = os.environ['radar_name']
    
    usage = "usage: %prog [options]"
    parser = OptionParser(usage)
    parser.add_option('--debug',
                      dest='debug', default='False',
                      action="store_true",
                      help='Set debugging on')
    parser.add_option('--unix_time',
                      dest='validTime',
                      default=0,
                      help='Valid time for image')
    parser.add_option('--full_file_path',
                      dest='fullFilePath',
                      default='unknown',
                      help='Full path of image file')
    parser.add_option('--file_name',
                      dest='fileName',
                      default='unknown',
                      help='Name of image file')
    parser.add_option('--radar_name',
                      dest='radarName',
                      default=radarName,
                      help='name of radar')

    (options, args) = parser.parse_args()

    if (options.debug == True):
        print >>sys.stderr, "Options:"
        print >>sys.stderr, "  debug? ", options.debug
        print >>sys.stderr, "  validTime: ", options.validTime
        print >>sys.stderr, "  fullFilePath: ", options.fullFilePath
        print >>sys.stderr, "  fileName: ", options.fileName
        print >>sys.stderr, "  radarName: ", options.radarName

########################################################################
# Run a command in a shell, wait for it to complete

def runCommand(cmd):

    if (options.debug == True):
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
