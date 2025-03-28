#!/usr/bin/env python

# ========================================================================== #
#
# Configure the host for a given role
#
# ========================================================================== #

import os
import sys
from optparse import OptionParser
import datetime
import subprocess
import string

def main():

    global options

    homeDir = os.environ['HOME']
    projDir = os.path.join(homeDir, 'projDir')
    controlDir = os.path.join(projDir, 'control')
    defaultGitDir = os.path.join(homeDir, "git/titan-cswr")

    # parse the command line

    usage = "usage: %prog [options]"
    parser = OptionParser(usage)
    parser.add_option('--debug',
                      dest='debug', default=True,
                      action="store_true",
                      help='Set debugging on')
    parser.add_option('--verbose',
                      dest='verbose', default=False,
                      action="store_true",
                      help='Set verbose debugging on')
    parser.add_option('--gitDir',
                      dest='gitDir', default=defaultGitDir,
                      help='Path of main directory in git')
    parser.add_option('--dataDir',
                      dest='dataDir', default='/data/dow',
                      help='Path of installed data dir')
    (options, args) = parser.parse_args()
    
    if (options.verbose):
        options.debug = True

    # compute paths

    gitProjDir = os.path.join(options.gitDir, 'projDir')
    gitSystemDir = os.path.join(gitProjDir, 'system')
    
    # debug print

    if (options.debug):
        print("Running script: ", os.path.basename(__file__), file=sys.stderr)
        print("", file=sys.stderr)
        print("  Options:", file=sys.stderr)
        print("    Debug: ", options.debug, file=sys.stderr)
        print("    Verbose: ", options.verbose, file=sys.stderr)
        print("    homeDir: ", homeDir, file=sys.stderr)
        print("    projDir: ", projDir, file=sys.stderr)
        print("    controlDir: ", controlDir, file=sys.stderr)
        print("    gitDir: ", options.gitDir, file=sys.stderr)
        print("    gitProjDir: ", gitProjDir, file=sys.stderr)
        print("    gitSystemDir: ", gitSystemDir, file=sys.stderr)

    # read current host type if previously set

    prevHostType = 'mgen6'
    hostTypePath = os.path.join(homeDir, '.host_type')
    if (os.path.exists(hostTypePath)):
        hostTypeFile = open(hostTypePath, 'r')
        prevHostType = hostTypeFile.read()
        prevHostType = prevHostType.strip(string.whitespace)
    if (options.debug):
        print("    prevHostType: ", prevHostType, file=sys.stderr)

    # get the host type interactively

    hostTypes = [ 'mgen6', 'mgen7', 'mgen8', 'mgenr', 'mgenc', 'mgenc2',
                  'drx6', 'drx7', 'drx8', 'drxr', 'drxc', 'drxc2' ]

    print("")
    print("Choose host type from the following list")
    print("       or hit enter to use host type shown:")
    for hostType in hostTypes:
        print("     ", hostType)
    hostType = input('    ............. (' + prevHostType + ')? ')
    if (len(hostType) < 4):
        hostType = prevHostType
    else:
        typeIsValid = False
        for htype in hostTypes:
            if (hostType == htype):
                typeIsValid = True
        if (typeIsValid != True):
            print("ERROR - invalid host type: ", hostType, file=sys.stderr)
            sys.exit(1)

    gitProjDir = os.path.join(options.gitDir, "projDir")

    # save the host type to ~/.host_type

    hostTypeFile = open(hostTypePath, "w")
    hostTypeFile.write(hostType + '\n')
    hostTypeFile.close()

    # banner

    print(" ")
    print("*****************************************************************")
    print()
    print("  configure DOW")
    print()
    print("  runtime: " + str(datetime.datetime.now()))
    print()
    print("  host type: ", hostType)
    print()
    print("*****************************************************************")
    print(" ")

    # make links to the dotfiles in git projDir
    
    os.chdir(homeDir)
    for rootName in ['cshrc', 'bashrc', 'emacs', 'Xdefaults' ]:
        dotName = '.' + rootName
        removeSymlink(homeDir, dotName)
        sourceDir = os.path.join(gitSystemDir, 'dotfiles')
        sourcePath = os.path.join(sourceDir, rootName)
        cmd = "ln -s " + sourcePath + " " + dotName
        runCommand(cmd)

    # make link to projDir

    removeSymlink(homeDir, 'projDir')
    os.chdir(homeDir)
    cmd = "ln -s " + gitProjDir
    runCommand(cmd)
    
    # make link to proc_list, crontab and data_list

    os.chdir(controlDir)

    removeSymlink(controlDir, "proc_list")
    cmd = "ln -s proc_list." + hostType + " proc_list"
    runCommand(cmd)

    removeSymlink(controlDir, "crontab")
    cmd = "ln -s crontab." + hostType + " crontab"
    runCommand(cmd)

    removeSymlink(controlDir, "data_list")
    cmd = "ln -s data_list." + hostType + " data_list"
    runCommand(cmd)

    # make link to radar_info in systems/params

    radar_name = {}
    radar_name['drx6'] = 'dow6'
    radar_name['drx7'] = 'dow7'
    radar_name['drx8'] = 'dow8'
    radar_name['drxr'] = 'dowr'
    radar_name['drxc'] = 'cow1'
    radar_name['drxc2'] = 'cow2'
    radar_name['mgen6'] = 'dow6'
    radar_name['mgen7'] = 'dow7'
    radar_name['mgen8'] = 'dow8'
    radar_name['mgenr'] = 'dowr'
    radar_name['mgenc'] = 'cow1'
    radar_name['mgenc2'] = 'cow2'
    
    sParamsDir = os.path.join(projDir, 'system/params')
    os.chdir(sParamsDir)
    removeSymlink(sParamsDir, "radar_info")
    cmd = "ln -s radar_info." + radar_name[hostType] + " radar_info"
    runCommand(cmd)

    ############################################
    # data dir - specific to the host type
    # populate installed data dir /data/spol
    
    dataDirsPath = os.path.join(options.gitDir, 'data_dirs')
    dataSubDir = "data." + hostType
    templateDataDir = os.path.join(dataDirsPath, dataSubDir)
    installDataDir = os.path.join(options.dataDir, dataSubDir)

    if (options.debug):
        print("Install data dir: ", installDataDir, file=sys.stderr)

    # create symlink to data

    os.chdir(projDir)
    removeSymlink(projDir, "data")
    if (os.path.exists('data') == False):
        cmd = "ln -s " + installDataDir + " data"
        runCommand(cmd)

    # create symlink to logs

    cmd = "mkdir -p " + installDataDir + " data/logs"
    runCommand(cmd)

    os.chdir(projDir)
    removeSymlink(projDir, "logs")
    if (os.path.exists('logs') == False):
        cmd = "ln -s data/logs"
        runCommand(cmd)

    # rync template dir into data dir

    os.chdir(templateDataDir)
    cmd = "rsync -av * " + installDataDir
    runCommand(cmd)

    # create symlinks for parameter files
    # from data tree back into the template

    os.chdir(projDir)
    debugStr = ""
    if (options.debug):
        debugStr = " --debug"
    cmd = "./system/scripts/createParamLinks.py --templateDir " + \
          templateDataDir + " --installDir " + installDataDir + debugStr
    runCommand(cmd)
    
    # done

    sys.exit(0)
    
########################################################################
# Remove a symbolic link
# Exit on error

def removeSymlink(dir, linkName):

    os.chdir(dir)

    # remove if exists
    if (os.path.islink(linkName)):
        os.unlink(linkName)
        return

    if (os.path.exists(linkName)):
        # link name exists but is not a link
        print("ERROR - trying to remove symbolic link", file=sys.stderr)
        print("  dir: ", dir, file=sys.stderr)
        print("  linkName: ", linkName, file=sys.stderr)
        print("This is NOT A LINK", file=sys.stderr)
        sys.exit(1)

########################################################################
# Run a command in a shell, wait for it to complete

def runCommand(cmd):

    if (options.debug == True):
        print("running cmd:",cmd, file=sys.stderr)

    try:
        retcode = subprocess.call(cmd, shell=True)
        if retcode < 0:
            print("Child was terminated by signal: ", -retcode, file=sys.stderr)
        else:
            if (options.verbose == True):
                print("Child returned code: ", retcode, file=sys.stderr)
    except OSError as e:
        print("Execution failed:", e, file=sys.stderr)

########################################################################
# Run - entry point

if __name__ == "__main__":
   main()
