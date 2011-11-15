#! /usr/bin/perl

use strict;
use warnings;

my ($branch) = grep { s/^\* (?:.*\/)?(\w+)/$1/ } qx/git branch/;
chomp $branch;

my ($version) = qx/git describe/;
chomp $version;

print "$version-$branch\n";

