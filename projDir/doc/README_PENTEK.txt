README for PENTEK DOW operation      2013/06/09
===============================

1. Power on
-----------

Turn power on to:

  * STALO
  * Digital receiver
  * HIQ windows computer
  * DRX linux computer
  * MGEN linux computer

2. Start mgen processes
-----------------------

Log in:

  user: titan5
  password: titan4cswr

Start the system:

  double-click on green StartAll icon on desktop

After 30 secs or so, type

  pcheck

to make sure all processes come up.

To see the current processes, open a terminal and make it wide.
Type:

  ppm (for print_procmap)

Change to a new desktop.

Click on the icons in the top bar:

  HawkEye high
  HawkEye low

3. Start drx processes
-----------------------

Log in:

  user: titan5
  password: titan4cswr

Start the system:

  double-click on green StartAll icon on desktop

After 30 secs or so, type

  pcheck

to make sure all processes come up.

To see the current processes, open a terminal and make it wide.
Type:

  ppm (for print_procmap)

Bring up the GUI:

  Change to a new desktop.
  Bring up the GUI by clicking the GUI icon in the top bar.

Bring up HawkEye:

  Change to a new desktop.
  Bring up HawkEye by clicking on the icons in the top bar:

    HawkEye high
    HawkEye low

To see the burst pulse:

  Change to a new desktop.
  Click on ascope high and low icons in the top bar.
    Select 'channel 2'
    Select 'along range'
    Select 'auto scale'

4. Starting HIQ
---------------

  * start antenna controller (Antenna Controller icon)
  * start angle export (Swiss army knife icon)

There is an RTF Readme file - see the HIQ-Readme icon

5. Data locations
-----------------

The data is stored on mgen.

DORADE moments:

  projDir/data/dorade/moments/high/yyyymmdd
  projDir/data/dorade/moments/low/yyyymmdd
  projDir/data/dorade/moments/combined/yyyymmdd

NetCDF moments:

  projDir/data/cfradial/moments/high/yyyymmdd
  projDir/data/cfradial/moments/low/yyyymmdd
  projDir/data/cfradial/moments/combined/yyyymmdd

Time series:

  projDir/data/time_series/save/high/yyyymmdd
  projDir/data/time_series/save/low/yyyymmdd

Monitoring:

  projDir/data/spdb/monitor/high/yyyymmdd*
  projDir/data/spdb/monitor/low/yyyymmdd*

To see the current data, open a terminal and make it wide.
Type:

  pdm (for PrintDataMap)

6. Data archival to USB
-----------------------

If a properly prepared USB drive is plugged into the USB3 connection,
the data will be copied from the RAID to the USB drive.

Type:

  df -h

to see the drive is mounted and how full it is. You will see something like:

 (mgen7) data 14 % df -h
  Filesystem            Size  Used Avail Use% Mounted on
  /dev/sda3              49G   12G   34G  27% /
  tmpfs                  32G  228K   32G   1% /dev/shm
  /dev/sda1             485M   64M  397M  14% /boot
  /dev/sda2              97G  2.3G   89G   3% /home
  /dev/sda5             5.3T  1.3T  3.7T  26% /data
  /dev/sdb1             1.8T  151G  1.6T   9% /media/DOW7_001

The drives should be labelled DOW7_XXX, where XXX is the serial number.

7. Cleaning data off the RAID
-----------------------------

You will need to keep the RAID empty enough to take new data.

Use 'df -h' to see how much space there is on the RAID (/data).

To clean a day of data from the disk, run:

  clean_raid_data yyyymmdd

You will be prompted to make sure you actually want to go ahead.

8. Viewing the radial data
--------------------------

Use solo3 to view the dorade data.

We can set up CIDD to view the netcdf data.

9. Shutting down
----------------

On drx and mgen, use menus:

  System -> ShutDown

On HIQ:

  Stop antenna and stow
  Start->shutdown

