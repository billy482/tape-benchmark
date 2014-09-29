#! /usr/bin/perl

use strict;
use warnings;

use IPC::Open2;

# Try to read old checksum
my $symbol   = shift @ARGV;
my $filename = shift @ARGV;

my ( $fdin, $fdout );
my $pid = open2( $fdout, $fdin, 'sha1sum', '-' ) or die "Can't exec sha1sum";

print $fdin $_ while (<>);
close $fdin;

waitpid $pid, 0;

my $line       = <$fdout>;
my $newCheckum = '';
$newCheckum = $1 if ( $line =~ /^(\w+)\s/ );

open my $fd, '>', $filename;
print $fd "#define TAPEBENCHMARK_SRCSUM \"$newCheckum\"\n";
close $fd;

