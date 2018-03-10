#!/usr/bin/env python

# ========================================================================== #
#
# Catalog the data on the archive disks
#
# Prints catalog to stdout
#
# ========================================================================== #

import os
from os.path import join, getsize
import sys
import subprocess
from optparse import OptionParser

def main():

    global options
    global debug
    global driveList

    # parse the command line

    usage = "usage: %prog [options]: prints catalog to stdout"
    parser = OptionParser(usage)
    parser.add_option('--debug',
                      dest='debug', default='False',
                      action="store_true",
                      help='Set debugging on')
    
    (options, args) = parser.parse_args()

    if (options.debug == True):
        print >>sys.stderr, "Options:"
        print >>sys.stderr, "  Debug: ", options.debug
        
    # compile the list of target drives

    compileDriveList()

    if (options.debug == True):
        print >>sys.stderr, "  Archive drive list: ", driveList
        
    # catalog each drive

    for drive in driveList:
        doCatalog(drive)

    exit(0)

########################################################################
# Get the list of drives

def compileDriveList():

    global driveList

    pipe = subprocess.Popen('df', shell=True,
                            stdout=subprocess.PIPE).stdout
    lines = pipe.readlines()
    driveList = []
    for line in lines:
        if (line.split()[0].find('/dev') >= 0):
            partition = line.split()[5]
            if (partition.find('DOW') >= 0):
                driveList.append(partition)

    driveList.sort()

########################################################################
# Perform catalog for this drive

def doCatalog(drive):

    # create dir list with nbytes used by files
    
    totGig = 0
    catalog = {}
    for root, dirs, files in os.walk(drive):
        nbytes = sum(getsize(join(root, name)) for name in files)
        if (int(nbytes) > 0
            and root.find('CVS') < 0
            and root.find('field_data') > 0):
            ngig = nbytes / 1.073731e9
            fieldPos = root.find('field_data')
            projPos = fieldPos + 11
            dir = root[projPos:]
            catalog[dir] = ngig;
            totGig += ngig

    # get sorted list of days

    days = []
    for key in sorted(catalog.iterkeys()):
        dir = key
        daystr = dir[-8:]
        if (daystr.isdigit()):
            if (days.count(daystr) < 1):
                days.append(daystr)

    days.sort()

    # get gbytes per day

    daysizes = {}
    for day in days:
        daysum = 0
        for key in sorted(catalog.iterkeys()):
            if (key.find(day) > 0):
                ngig = catalog[key]
                daysum += ngig
        daysizes[day] = daysum
    
    print "=============================================================== "
    print "Catalog for drive " + drive + ":"
    print "  %50s %10s" % ("Dir", "Gbytes")
    print "  %50s %10s" % ("---", "------")

    for key in sorted(catalog.iterkeys()):
        dir = key
        ngig = catalog[key]
        print "  %50s %10.3f" % (dir, ngig)

    print "  %50s %10s" % ("", "--------")

    for key in sorted(daysizes.iterkeys()):
        day = key
        ngig = daysizes[key]
        label = "Subtotal for " + day + " (GBytes):"
        print "  %50s %10.3f" % (label, ngig)

    print "  %50s %10s" % ("", "--------")
    print "  %50s %10.3f" % ("Total (GBytes):", totGig)
    print "=============================================================== "


########################################################################
# Run - entry point

if __name__ == "__main__":
   main()
