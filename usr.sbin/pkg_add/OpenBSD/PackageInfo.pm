# ex:ts=8 sw=4:
# $OpenBSD: src/usr.sbin/pkg_add/OpenBSD/PackageInfo.pm,v 1.17 2005/01/16 11:16:23 espie Exp $
#
# Copyright (c) 2003-2004 Marc Espie <espie@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;
package OpenBSD::PackageInfo;
our @ISA=qw(Exporter);
our @EXPORT=qw(installed_packages installed_info installed_name info_names is_info_name 
    lock_db unlock_db
    add_installed delete_installed is_installed borked_package CONTENTS COMMENT DESC INSTALL DEINSTALL REQUIRE 
    REQUIRED_BY REQUIRING DISPLAY UNDISPLAY MTREE_DIRS);

use OpenBSD::PackageName;
use constant {
	CONTENTS => '+CONTENTS',
	COMMENT => '+COMMENT',
	DESC => '+DESC',
	INSTALL => '+INSTALL',
	DEINSTALL => '+DEINSTALL',
	REQUIRE => '+REQUIRE',
	REQUIRED_BY => '+REQUIRED_BY',
	REQUIRING => '+REQUIRING',
	DISPLAY => '+DISPLAY',
	UNDISPLAY => '+UNDISPLAY',
	MTREE_DIRS => '+MTREE_DIRS' };

use Fcntl qw/:flock/;
my $pkg_db = $ENV{"PKG_DBDIR"} || '/var/db/pkg';

our $list;

our @info = (CONTENTS, COMMENT, DESC, REQUIRE, INSTALL, DEINSTALL, REQUIRED_BY, REQUIRING, DISPLAY, UNDISPLAY, MTREE_DIRS);

our %info = ();
for my $i (@info) {
	my $j = $i;
	$j =~ s/\+/F/;
	$info{$i} = $j;
}

sub _init_list
{
	$list = {};
	my @bad=();

	opendir(my $dir, $pkg_db) or die "Bad pkg_db: $!";
	while (my $e = readdir($dir)) {
		next if $e eq '.' or $e eq '..';
		next unless -d "$pkg_db/$e";
		if (! -r _) {
			push(@bad, $e);
			next;
		}
		if (-f "$pkg_db/$e/+CONTENTS") {
			$list->{$e} = 1;
		} else {
			print "Warning: $e is not really a package\n";
		}
	}
	close($dir);
	if (@bad > 0) {
		print "Warning: can't access information for ", join(", ", @bad), "\n";
	}
}

sub add_installed
{
	if (!defined $list) {
		_init_list();
	}
	for my $p (@_) {
		$list->{$p} = 1;
	}
}

sub delete_installed
{
	if (!defined $list) {
		_init_list();
	}
	for my $p (@_) {
		delete $list->{$p};

	}
}

sub installed_packages(;$)
{
	if (!defined $list) {
		_init_list();
	}
	if ($_[0]) {
		return grep { !/^\./ } keys %$list;
	} else {
		return keys %$list;
	}
}

sub installed_info($)
{
	my $name =  shift;

	if ($name =~ m|^\Q$pkg_db\E/?|) {
		return "$name/";
	} else {
		return "$pkg_db/$name/";
	}
}

sub installed_contents($)
{
	return installed_info(shift).CONTENTS;
}

sub borked_package($)
{
	my $pkgname = $_[0];
	unless (-e "$pkg_db/partial-$pkgname") {
		return "partial-$pkgname";
	}
	my $i = 1;

	while (-e "$pkg_db/partial-$pkgname.$i") {
		$i++;
	}
	return "partial-$pkgname.$i";
}

sub is_installed($)
{
	my $name = installed_name(shift);
	if (!defined $list) {
		installed_packages();
	}
	return defined $list->{$name};
}

sub installed_name($)
{
	my $name = shift;
	$name =~ s|/$||;
	$name =~ s|^\Q$pkg_db\E/?||;
	return $name;
}

sub info_names()
{
	return @info;
}

sub is_info_name($)
{
	my $name = shift;
	return $info{$name};
}

my $dlock;

sub lock_db($;$)
{
	my ($shared, $quiet) = @_;
	my $mode = $shared ? LOCK_SH : LOCK_EX;
	open($dlock, '<', $pkg_db) or return;
	if (flock($dlock, $mode | LOCK_NB)) {
		return;
	}
	print STDERR "Package database already locked... awaiting release\n"
		unless $quiet;
	while (!flock($dlock, $mode)) {
	}
	return;
}

sub unlock_db()
{
	if (defined $dlock) {
		flock($dlock, LOCK_UN);
		close($dlock);
	}
}

1;
