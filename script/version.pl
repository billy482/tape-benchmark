#! /usr/bin/perl

use strict;
use warnings;

my ($version) = qx/git describe/;
chomp $version;

my ($branch) = grep { s/^\* (?:.*\/)?(\w+)/$1/ } qx/git branch/;
chomp $branch;

print (($branch eq 'master') ? $version : "$version-$branch\n");

