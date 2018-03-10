#!/usr/bin/env python

# ========================================================================== #
#
# Trigger the remote archive script
#
# ========================================================================== #

import os
import sys
from optparse import OptionParser
import time
import datetime
from datetime import date
from datetime import timedelta
import subprocess

def main():

    global options
    global dirList
    global driveList
    global deviceTable
    global nowSecs
    global validStartSecs
    global maxAgeSecs
    global isRemote
    global thisHost

    # set some variables from the environment

    dataDir = os.environ['DATA_DIR']
    projDir = os.environ['PROJ_DIR']
    projName = os.environ['PROJECT_NAME']
    defaultDirListPath = \
        os.path.join(projDir, "archive/params/archiveDirList")
    thisHost = os.environ['HOST']
    
    # parse the command line

    usage = "usage: %prog [options]"
    parser = OptionParser(usage)
    parser.add_option('--debug',
                      dest='debug', default=False,
                      action="store_true",
                      help='Set debugging on')
    parser.add_option('--verbose',
                      dest='verbose', default=False,
                      action="store_true",
                      help='Set verbose debugging on')
    parser.add_option('--testMode',
                      dest='testMode', default=False,
                      action="store_true",
                      help='Use preset dates instead of today')
    parser.add_option('--sourceDir',
                      dest='sourceDir', default=dataDir,
                      help='Path of source directory')
    parser.add_option('--projectName',
                      dest='projectName', default=projName,
                      help='Name of project - used to generate target dir')
    parser.add_option('--dirListPath',
                      dest='dirListPath',
                      default=defaultDirListPath,
                      help='Path to file containing directory list')
    parser.add_option('--maxAgeHours',
                      dest='maxAgeHours', default='12',
                      help='Max age of files to be archived (hours)')
    parser.add_option('--remoteHost',
                      dest='remoteHost', default="",
                      help='Host name for remote rsync')
    parser.add_option('--followLinks',
                      dest='followLinks', default=False,
                      action="store_true",
                      help='Backup dirs/files that are links')
    
    (options, args) = parser.parse_args()

    maxAgeSecs = int(float(options.maxAgeHours) * 3600.0)

    if (options.verbose == True):
        options.debug = True

    if (options.debug == True):
        print >>sys.stderr, "Running: ", os.path.basename(__file__)
        print >>sys.stderr, "  Options:"
        print >>sys.stderr, "    debug: ", options.debug
        print >>sys.stderr, "    verbose: ", options.verbose
        print >>sys.stderr, "    testMode: ", options.testMode
        print >>sys.stderr, "    sourceDir: ", options.sourceDir
        print >>sys.stderr, "    projectName: ", options.projectName
        print >>sys.stderr, "    dirListPath: ", options.dirListPath
        print >>sys.stderr, "    maxAgeHours: ", options.maxAgeHours
        print >>sys.stderr, "         (secs): ", maxAgeSecs
        print >>sys.stderr, "    remoteHost: ", options.remoteHost
        print >>sys.stderr, "    followLinks: ", options.followLinks
        
    # compile the list of target drives

    compileDriveList()

    if (options.debug == True):
        print >>sys.stderr, "======================="
        print >>sys.stderr, "Target disk drive list:"
        for drive in driveList:
            print >>sys.stderr, \
                "  drive, device: ", \
                drive, deviceTable[drive]

    # perform the archival, to each drive
        
    for drive in driveList:
        doRemoteToDrive(drive)
        # for now, exit after 1 drive, ignore second drive
        sys.exit(0)

    sys.exit(0)

########################################################################
# Get the list of drives

def compileDriveList():

    global driveList
    global deviceTable

    deviceTable = {}

    # run df to get drive stats
    
    pipe = subprocess.Popen('df', shell=True,
                            stdout=subprocess.PIPE).stdout
    lines = pipe.readlines()

    # load up drive list and associated tables
    
    driveList = []
    for line in lines:
        tokens = line.split()
        if (tokens[0].find('/dev') >= 0):
            partition = tokens[5]
            if (partition.find('DOW') >= 0):
                driveList.append(partition)
                deviceName = tokens[0]
                deviceTable[partition] = deviceName

    # sort drive list
    
    driveList.sort()

########################################################################
# archive remote host to specified drive

def doRemoteToDrive(drive):

    # set up ssh command

    cmd = "ssh " + options.remoteHost + " 'doArchive.py"

    if (options.debug):
        cmd += " --debug"

    if (options.verbose):
        cmd += " --verbose"

    cmd += " --sourceDir " + options.sourceDir
    cmd += " --dirListPath " + options.dirListPath
    cmd += " --maxAgeHours " + options.maxAgeHours
    cmd += " --remoteHost " + thisHost
    cmd += " --remoteDrive " + drive

    if (options.followLinks):
        cmd += " --followLinks "

    cmd += "'"

    # run the command
    
    runCommand(cmd)

########################################################################
# Run a command in a shell, wait for it to complete

def runCommand(cmd):

    if (options.verbose == True):
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
# Run - entry point

if __name__ == "__main__":
   main()
