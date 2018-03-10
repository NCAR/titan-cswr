#!/usr/bin/perl
#
# Put quicklook images to FTP
#

# system Perl modules

use Net::FTP;
use Time::Local;
use Getopt::Long;
 
#
# RAP Perl modules
#

use Env qw(RAP_LIB_DIR);
use lib "$RAP_LIB_DIR/perl5/";
use Procmap;
use Toolsa;

$debug = 1;                              # Ordinary debugging flag.
$verbose = 1;                            # Verbose debugging flag.
$proj_dir = $ENV{'PROJ_DIR'};

#------------------------- Parse command line ----------------------------

($prog = $0) =~ s%.*/%%;                 # Determine program basename.
&GetOptions("debug!" => \$debug,
	    "verbose!" => \$verbose,
	    "input_file_path=s" => \$input_file_path,
	    "file_modify_time=s" => \$file_modify_time);

if ($debug) {
    print STDERR "Running put_quicklook_to_ftp:\n";
    print STDERR "  input_file_path: $input_file_path\n";
    print STDERR "  file_modify_time: $file_modify_time\n";
}

#------------------------- initialize ----------------------------

$| = 1;                           # Unbuffer standard output.
umask 002;                        # Set file permissions.
$ftp_timeout = 60;                # Max time for single file transfer

#----------------- send files to EOL catalog ------------------------------

$ftp_host = "vortex2.org"; 
$user = "jwurman";       # The remote username.
$passwd = "cswr+binet";      # The remote password.
$targetdir = "/V2-20130303/pub/${RADAR_NAME}"; # The remote target data directory.

&send_file($ftp_host, $user, $passwd, $targetdir, $input_file_path);

#--------------------- remove tmp file ----------------------------------------

unlink $tmp_path; 

exit 0;

# --------------------------------------------------------------------
# send_file function: Send file to remote host
# Arg: host

sub send_file {

  local($ftp_host, $user, $passwd, $targetdir, $filepath) = @_;
  local($ftp);
  
  if ($debug) {
    print STDERR "Sending: ftp_host: $ftp_host\n";
    print STDERR "         user: $user\n";
    print STDERR "         passwd: $passwd\n";
    print STDERR "         targetdir: $targetdir\n";
    print STDERR "         file path: $filepath\n";
  }

  # get the day dir from the path

  @parts = split(/\//, $filepath);
  $daydir = @parts[-2];
  $ftpdir = $targetdir . "/" . $daydir;

  if ($debug) {
      print STDERR "         parts: @parts\n";
      print STDERR "         daydir: $daydir\n";
      print STDERR "         ftpdir: $ftpdir\n";
  }
  
  $ftp = Net::FTP->new($ftp_host, Passive=>true, Timeout=>$ftp_timeout);
  if (!$ftp) {
    print STDERR "ftp-new failure\n";
    return;
  }
  if ($ftp->login($user, $passwd) == 0) {
    print STDERR "ftp-login failure, user: $user, password: $pass\n";
    $ftp->quit;
    return;
  }
  
  $ftp->mkdir($ftpdir,"true");
  
  if ($ftp->cwd($ftpdir) == 0) {
    print STDERR "ftp-cwd failure\n";
    $ftp->quit;
    return;
  }
  
  $ftp->binary;
  
  if ($ftp->put($filepath) != 0) {
    print STDERR "ftp-put failure\n";
    $ftp->quit;
    return;
  }
  
  $ftp->quit;
  
  if ($debug) {
    print STDERR "-->> success\n";
  }
  
}

