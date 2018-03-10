#! /bin/bash
# script to enter a heading and append it to a file
#
# (c) Advanced Radar Corp, 2009   All rights reserved.
# M. Dixon May 2009
# F. Hage April 2010
#   This script is the UI for updating the Heading and Location of Mobile Radars.
#    How this works:
#      1. A custom version of ntpd using a GPS reference is run which leaves location info in /var/log/location.
#	  - Started from /etc/rc.local - Must make sure the /dev/gps0 link exists.
#      2. The user enters a heading and is is added to the location in /var/log/positions
#      3. TsTcp2Fmq reads this file and applies the offset to the time series data.
#--------------------------------------------------------------------

# set the path

export PATH=.:/bin:/usr/bin:/sbin:/usr/sbin:/usr/bin/X11:/usr/local/bin:/usr/local/sbin

#######################################################
# get run time

year=`date +'%Y'`
month=`date +'%m'`
day=`date +'%d'`
hour=`date +'%H'`
min=`date +'%M'`
sec=`date +'%S'`
datestr=${year}${month}${day}.${hour}${min}${sec}

#--------------------------------------------------------------------

echo
echo "**********************************************"
echo "  ARC - ADVANCED RADAR CORPORATION"
echo "  Boulder, CO, USA"
echo "  Runtime: $year/$month/$day $hour:$min:$sec"
echo "**********************************************"
echo "  UTILITY TO ENTER MOBILE PLATFORM HEADING:"
echo "**********************************************"
echo

while [ 1 ]
do
  read -ep "Enter heading (decimal degrees): " heading
  if [ ${#heading} -gt 0 ]; then
      loc=`cat /var/log/location`
      datestr=`date -u "+%Y%m%d.%H%M%S"`
      echo "$heading	$loc	$datestr" >> /var/log/positions
      echo "Heading, Loc: $heading $loc m" 
  fi
done

