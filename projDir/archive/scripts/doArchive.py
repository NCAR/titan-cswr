#!/usr/bin/env python

# ========================================================================== #
#
# Trigger the archive script
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
    projectName = os.environ['PROJECT_NAME']
    radarName = os.environ['RADAR_NAME']
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
                      dest='projectName', default=projectName,
                      help='Name of project - used to generate target dir')
    parser.add_option('--radarName',
                      dest='radarName', default=radarName,
                      help='Name of radar - used to generate target dir')
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
    parser.add_option('--remoteDrive',
                      dest='remoteDrive', default="",
                      help='Drive name for remote rsync')
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
        print >>sys.stderr, "    radarName: ", options.radarName
        print >>sys.stderr, "    dirListPath: ", options.dirListPath
        print >>sys.stderr, "    maxAgeHours: ", options.maxAgeHours
        print >>sys.stderr, "         (secs): ", maxAgeSecs
        print >>sys.stderr, "    remoteHost: ", options.remoteHost
        print >>sys.stderr, "    remoteDrive: ", options.remoteDrive
        print >>sys.stderr, "    followLinks: ", options.followLinks
        
    # compile the list of target drives

    if (len(options.remoteHost) == 0):
        isRemote = False
    else:
        isRemote = True

    if (isRemote):
        driveList = []
        driveList.append(options.remoteDrive)
    else:
        compileDriveList()

    if (options.debug == True):
        print >>sys.stderr, "======================="
        print >>sys.stderr, "Target disk drive list:"
        for drive in driveList:
            if (isRemote == True):
                print >>sys.stderr, "  drive: ", drive
            else:
                print >>sys.stderr, \
                    "  drive, device: ", \
                    drive, deviceTable[drive]

    # read in the directory list

    readDirList()

    if (options.debug == True):
        print >>sys.stderr, "======================="
        print >>sys.stderr, "Dir list:"
        for dir in dirList:
            print >>sys.stderr, "  -->> ", dir

    # compute day string for today
    # in test mode we set the daystr directly

    if (options.testMode == True):
        now = time.strptime("20110425010000", "%Y%m%d%H%M%S")
    else:
        now = time.gmtime()

    nowTime = datetime.datetime(now.tm_year, now.tm_mon, now.tm_mday,
                                now.tm_hour, now.tm_min, now.tm_sec)
    nowStr = nowTime.strftime("%Y%m%d")
    nowSecs = time.mktime(nowTime.timetuple())

    # compute the earliest valid time

    validPeriodDays = timedelta(float(options.maxAgeHours) / 24.0)
    validStartTime = nowTime - validPeriodDays
    validStartSecs = nowSecs - maxAgeSecs

    if (options.debug == True):
        print >>sys.stderr, "======================="
        print >>sys.stderr, "Time details: "
        print >>sys.stderr, "  now time: ", nowTime
        print >>sys.stderr, "      secs: ", nowSecs
        print >>sys.stderr, "  files valid from: ", validStartTime
        print >>sys.stderr, "              secs: ", validStartSecs

    # perform the archival, to each drive
        
    for drive in driveList:
        for dir in dirList:
            doArchiveToDrive(drive, dir)
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
            if ((partition.find('DOW') >= 0) or (partition.find('COW') >= 0)):
                driveList.append(partition)
                deviceName = tokens[0]
                deviceTable[partition] = deviceName

    # sort drive list
    
    driveList.sort()

########################################################################
# Read in the directory list

def readDirList():

    global dirList

    fp = open(options.dirListPath, 'r')
    lines = fp.readlines()
    dirList = []
    for line in lines:
        if (line[0] != '#' and len(line) > 1):
            dirList.append(line.strip())


########################################################################
# archive specified dir to specified drive

def doArchiveToDrive(drive, dir):

    # compute source dir

    sourceDir = options.sourceDir
    if (dir.find('logs') < 0):
        # dont add radar name for logs
        sourceDir = os.path.join(sourceDir, options.radarName)
    sourceDir = os.path.join(sourceDir, dir)

    if (options.debug == True):
        print >>sys.stderr, "====================================="
        print >>sys.stderr, "Syncing, dir: ", dir
        print >>sys.stderr, "    to drive: ", drive
        print >>sys.stderr, "   sourceDir: ", sourceDir

    # compute target dir
    
    targetDir = drive
    targetDir = os.path.join(targetDir, "data")
    targetDir = os.path.join(targetDir, options.projectName)
    targetDir = os.path.join(targetDir, options.radarName)
    if (isRemote == True):
        targetDir = os.path.join(targetDir, thisHost)
    targetDir = os.path.join(targetDir, dir)

    # compile the list of files to be archived

    processDir(sourceDir, targetDir, 0)

########################################################################
# process this dir

def processDir(sourceDir, targetDir, level):

    if (options.verbose == True):
        print >>sys.stderr, \
        "processDir: sourceDir, targetDir, level: ", \
            sourceDir, ", ", targetDir, ", ", level
    
    if (level > 5):
        # too deep
        print >>sys.stderr, "ERROR - recursion too deep"
        return

    if (os.path.isdir(sourceDir) == False):
        print >>sys.stderr, \
            "WARNING: sourceDir does not exist: ", sourceDir
        return

    # read through directory looking for files to archive

    fileList = []
    contents = os.listdir(sourceDir)

    for entry in contents:

        if (entry[0] == '_'):
            # ignore files starting with '_'
            continue

        entryPath = os.path.join(sourceDir, entry)

        if os.path.islink(entryPath) and not options.followLinks:
            continue

        if (os.path.isdir(entryPath)):

            # dir, so recurse

            newSourceDir = entryPath
            newTargetDir = os.path.join(targetDir, entry)

            if (options.verbose == True):
                print >>sys.stderr, "  ===>> recursing to dir: ", newSourceDir

            processDir(newSourceDir, newTargetDir, level + 1)

        elif (os.path.isfile(entryPath)):

            # file, so add to list

            entryStat = os.stat(entryPath)
            fileModSecs = entryStat.st_mtime
            timeDiffSecs = nowSecs - fileModSecs

            if (options.verbose == True):
                print >>sys.stderr, \
                    "  ===>> found path, modSecs, timeDiffSecs: ", \
                    entryPath, ", ", fileModSecs, ", ", timeDiffSecs

            # append to fileList

            if (fileModSecs > validStartSecs):
                if (options.verbose == True):
                    print >>sys.stderr, "  ===>> appending file: ", entry
                fileList.append(entry)

    if (len(fileList) < 1):
        return

    # sort the list alphabetically
    
    fileList.sort(lambda x, y: cmp(x.lower(),y.lower()))
    
    if (options.debug == True):
        print >>sys.stderr, "Full file list:"
        for fileName in fileList:
            print >>sys.stderr, "  ", fileName
        print >>sys.stderr, "Only the most recent items will be rsync'd"

    # create file list as string

    fileListStr = ""
    for fileName in fileList:
        fileListStr = fileListStr + fileName
        fileListStr = fileListStr + " "

    # ensure target dir exists

    cmd = ""
    if (isRemote == True):
        cmd += "ssh "
        cmd += options.remoteHost
        cmd += " '"
    cmd += "mkdir -p "
    cmd += targetDir
    if (isRemote == True):
        cmd += " '"
    runCommand(cmd)

    # do rsync

    cmd = "cd "
    cmd += sourceDir
    cmd += "; rsync -av -6 --links "
    cmd += fileListStr
    cmd += " "
    if (isRemote == True):
        cmd += options.remoteHost
        cmd += ":"
    cmd += targetDir
    runCommand(cmd)

########################################################################
# compile file list archive

def compileFileList(topdir, subdir, level):

    global fileList
    if (level == 0):
        fileList = []

    if (options.verbose == True):
        print >>sys.stderr, \
        "compileFileList: topdir, subdir, level: ", \
            topdir, ", ", subdir, ", ", level
    
    if (level > 5):
        # too deep
        print >>sys.stderr, "ERROR - recursion too deep"
        return

    if (len(subdir) > 0):
        thisDir = os.path.join(topdir, subdir)
    else:
        # top level
        thisDir = topdir
        
    contents = os.listdir(thisDir)

    for entry in contents:

        if (entry[0] == '_'):
            # ignore files starting with '_'
            continue

        entryPath = os.path.join(thisDir, entry)
        entryRelPath = os.path.join(subdir, entry)

        if (os.path.isdir(entryPath)):
            
            # recurse
            if (options.verbose == True):
                print >>sys.stderr, "  ===>> recursing to dir: ", entryPath

            compileFileList(topdir, entryRelPath, level + 1)

        elif (os.path.isfile(entryPath)):

            entryStat = os.stat(entryPath)
            fileModSecs = entryStat.st_mtime
            timeDiffSecs = nowSecs - fileModSecs

            if (options.verbose == True):
                print >>sys.stderr, "  ===>> path, modSecs, timeDiffSecs: ", \
                    entryPath, ", ", fileModSecs, ", ", timeDiffSecs

            # append to fileList

            if (fileModSecs > validStartSecs):
                if (options.verbose == True):
                    print >>sys.stderr, "  ===>> appending file: ", entryRelPath
                fileList.append(entryRelPath)

    return

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
