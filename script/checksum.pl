#! /usr/bin/perl

use strict;
use warnings;

use Digest::SHA;

# Try to read old checksum
my $symbol   = shift @ARGV;
my $filename = shift @ARGV;

my $sha = Digest::SHA->new('1');
$sha->add($_) while (<>);

my $newCheckum = $sha->hexdigest;

open my $fd, '>', $filename;
print $fd "#define TAPEBENCHMARK_SRCSUM \"$newCheckum\"\n";
close $fd;

