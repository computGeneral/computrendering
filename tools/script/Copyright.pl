#!/usr/bin/perl -w
################################################################################
##
##  Copyright (c) 2022 - 2022 by computGeneral Lab.  All rights reserved.
##
##  The material in this file is confidential and contains trade secrets
##  of computGeneral Lab. This is proprietary information owned by
##  computGeneral Lab. No part of this work may be disclosed, reproduced,
##  copied, transmitted, or used in any way for any purpose,
##  without the express written permission of computGeneral Lab.
##
##  Auto-generated file on 1/28/2022. Do not edit it.
##
################################################################################

use strict;
use POSIX qw(ceil floor);
use Carp;
use Getopt::Long;
use Cwd;
use File::Basename;
use File::Copy;

$SIG{'INT'}  = \&interrupt;  $SIG{'QUIT'} = \&interrupt;
$SIG{'HUP' } = \&interrupt;  $SIG{'TRAP'} = \&interrupt;
$SIG{'ABRT'} = \&interrupt;  $SIG{'STOP'} = \&interrupt;

my $DIR = dirname ($0);
my $PGM = basename ($0);
&Getopt::Long::config("permute", "passthrough");

my %Opts;
if(!&GetOptions ( \%Opts, '-dir=s')){
#    &PrintUsage();
    exit(0);
}

if($Opts{'dir'}){
    if (-d $Opts{'dir'}) {
        browser_dir($Opts{'dir'});
    } else {
        exit(1);
    }
}else{
    print("[Copyright.pl][FATAL] target director '-dir=s' have to be specified");
}

sub browser_dir
{
    my $path = shift;
    opendir(DIR, $path) or die ("[Copyright.pl][FATAL] Open dir $path failed\n");
    my @files = readdir(DIR);
    closedir DIR;

    chdir($path);
    foreach my $file (@files) {
        if (-d "$path/$file") {
            unless ($file =~ m/^\.|_BUILD_|thirdParty/)
            {
                print("[Copyright.pl][INFO] Entering path:$path/$file\n");
                browser_dir("$path/$file");
                chdir("$path");
            }
        }else{
            addCopyRight($file) unless($file =~ m/sln|^\.|vcxproj|\.def|\.csv|\.md|\.xlsx|\.docx|\.props|\.xmind/);
        }
    }
}

sub addCopyRight
{
    my $file = shift;
    open FH, "$file";
    my @lines = <FH>;
    close FH;

    my $comm = "";
    if($file =~ m/\.sh|\.csh|\.bsh|\.pl|\.py|akefile|\.yml|\.yaml|\.mak|\.mk|\.txt|\.build|\.cmake|\.ini/) {
        $comm = "#";
    } elsif($file =~ m/\.bat/) {
        $comm = ":";
    } elsif($file =~ m/\.def/) { # vs project def file
        $comm = ";";
    } else{ #csv, md //if($file =~ m/\.cpp|\.hpp|\.c|\.h|\.v/) 
        $comm = "/";
    }

    my ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime time;
    $year+= 1900;
    $mon += 1;
    my $genTime = "$mon/$mday/$year";

    open FH, ">$file";
    for(my $i = 0; $i < @lines; $i++)
    {
        if($i == 0)
        {
            if($lines[0] =~ m/^#!/)
            {
                print FH $lines[0];
                for(my $j = 0; $j <80; $j++){ print FH "$comm"; }
                print FH "\n";
                print FH "$comm$comm\n";
                print FH "$comm$comm  Copyright (c) 2022 - $year by computGeneral Lab.  All rights reserved.\n";
                print FH "$comm$comm\n";
                print FH "$comm$comm  The material in this file is confidential and contains trade secrets\n";
                print FH "$comm$comm  of computGeneral Lab. This is proprietary information owned by\n";
                print FH "$comm$comm  computGeneral Lab. No part of this work may be disclosed, reproduced,\n";
                print FH "$comm$comm  copied, transmitted, or used in any way for any purpose,\n";
                print FH "$comm$comm  without the express written permission of computGeneral Lab.\n";
                print FH "$comm$comm\n";
                print FH "$comm$comm  Auto-generated file on $genTime. Do not edit it.\n";
                print FH "$comm$comm\n";
                for(my $j = 0; $j <80; $j++){ print FH "$comm"; }
                print FH "\n";
            }
            else
            {
                for(my $j = 0; $j <80; $j++){ print FH "$comm"; }
                print FH "\n";
                print FH "$comm$comm\n";
                print FH "$comm$comm  Copyright (c) 2022 - $year by computGeneral Lab.  All rights reserved.\n";
                print FH "$comm$comm\n";
                print FH "$comm$comm  The material in this file is confidential and contains trade secrets\n";
                print FH "$comm$comm  of computGeneral Lab. This is proprietary information owned by\n";
                print FH "$comm$comm  computGeneral Lab. No part of this work may be disclosed, reproduced,\n";
                print FH "$comm$comm  copied, transmitted, or used in any way for any purpose,\n";
                print FH "$comm$comm  without the express written permission of computGeneral Lab.\n";
                print FH "$comm$comm\n";
                print FH "$comm$comm  Auto-generated file on $genTime. Do not edit it.\n";
                print FH "$comm$comm\n";
                for(my $j = 0; $j <80; $j++){ print FH "$comm"; }
                print FH "\n";
                print FH $lines[0];
            }
        } else {
            print FH $lines[$i];
        }
    }
    close FH;
}

