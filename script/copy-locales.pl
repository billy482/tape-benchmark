#! /usr/bin/perl

use strict;
use warnings;

use File::Path 'make_path';

my ( $dest_dir, @files ) = @ARGV;

exit unless @files;

foreach my $file (@files) {
    if ( $file =~ m{/([^./]*)\.([^.]*).po$} ) {
        make_path( "$dest_dir/$2.UTF-8/LC_MESSAGES",
            { errno => \my $error } );

        my $new_file = "$dest_dir/$2.UTF-8/LC_MESSAGES/$1.po";

        open( my $fd_in, '<', $file ) or die "Can't open $file because $!";
        open( my $fd_out, '>', $new_file )
            or die "Can't open $new_file because $!";

        print {$fd_out} $_ while (<$fd_in>);

        close $fd_in;
        close $fd_out;
    }
}

