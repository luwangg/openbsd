#!/usr/local/bin/perl
# A bit of an evil hack but it post processes the file ../MINFO which
# is generated by `make files` in the top directory.
# This script outputs one mega makefile that has no shell stuff or any
# funny stuff
#

$INSTALLTOP="/usr/local/ssl";
$OPTIONS="";
$ssl_version="";

open(IN,"<Makefile.ssl") || die "unable to open Makefile.ssl!\n";
while(<IN>) {
    $ssl_version=$1 if (/^VERSION=(.*)$/);
    $OPTIONS=$1 if (/^OPTIONS=(.*)$/);
    $INSTALLTOP=$1 if (/^INSTALLTOP=(.*$)/);
}
close(IN);

die "Makefile.ssl is not the toplevel Makefile!\n" if $ssl_version eq "";

$infile="MINFO";

%ops=(
	"VC-WIN32",   "Microsoft Visual C++ [4-6] - Windows NT or 9X",
	"VC-NT",   "Microsoft Visual C++ [4-6] - Windows NT ONLY",
	"VC-W31-16",  "Microsoft Visual C++ 1.52 - Windows 3.1 - 286",
	"VC-WIN16",   "Alias for VC-W31-32",
	"VC-W31-32",  "Microsoft Visual C++ 1.52 - Windows 3.1 - 386+",
	"VC-MSDOS","Microsoft Visual C++ 1.52 - MSDOS",
	"Mingw32", "GNU C++ - Windows NT or 9x",
	"Mingw32-files", "Create files with DOS copy ...",
	"BC-NT",   "Borland C++ 4.5 - Windows NT",
	"BC-W31",  "Borland C++ 4.5 - Windows 3.1 - PROBABLY NOT WORKING",
	"BC-MSDOS","Borland C++ 4.5 - MSDOS",
	"linux-elf","Linux elf",
	"ultrix-mips","DEC mips ultrix",
	"FreeBSD","FreeBSD distribution",
	"default","cc under unix",
	);

$platform="";
foreach (@ARGV)
	{
	if (!&read_options && !defined($ops{$_}))
		{
		print STDERR "unknown option - $_\n";
		print STDERR "usage: perl mk1mf.pl [options] [system]\n";
		print STDERR "\nwhere [system] can be one of the following\n";
		foreach $i (sort keys %ops)
		{ printf STDERR "\t%-10s\t%s\n",$i,$ops{$i}; }
		print STDERR <<"EOF";
and [options] can be one of
	no-md2 no-md5 no-sha no-mdc2 no-ripemd  - Skip this digest
	no-rc2 no-rc4 no-idea no-des no-bf no-cast - Skip this symetric cipher
	no-rc5
	no-rsa no-dsa no-dh			- Skip this public key cipher
	no-ssl2 no-ssl3				- Skip this version of SSL
	just-ssl				- remove all non-ssl keys/digest
	no-asm 					- No x86 asm
	nasm 					- Use NASM for x86 asm
	no-socks				- No socket code
	no-err					- No error strings
	dll/shlib				- Build shared libraries (MS)
	debug					- Debug build
	gcc					- Use Gcc (unix)
	rsaref					- Build to require RSAref

Values that can be set
TMP=tmpdir OUT=outdir SRC=srcdir BIN=binpath INC=header-outdir CC=C-compiler

-L<ex_lib_path> -l<ex_lib>			- extra library flags (unix)
-<ex_cc_flags>					- extra 'cc' flags,
						  added (MS), or replace (unix)
EOF
		exit(1);
		}
	$platform=$_;
	}
foreach (split / /, $OPTIONS)
	{
	print STDERR "unknown option - $_\n" if !&read_options;
	}

$no_mdc2=1 if ($no_des);

$no_ssl3=1 if ($no_md5 || $no_sha);
$no_ssl3=1 if ($no_rsa && $no_dh);

$no_ssl2=1 if ($no_md5 || $no_rsa);
$no_ssl2=1 if ($no_rsa);

$out_def="out";
$inc_def="outinc";
$tmp_def="tmp";

$mkdir="mkdir";

($ssl,$crypto)=("ssl","crypto");
$RSAglue="RSAglue";
$ranlib="echo ranlib";

$cc=(defined($VARS{'CC'}))?$VARS{'CC'}:'cc';
$src_dir=(defined($VARS{'SRC'}))?$VARS{'SRC'}:'.';
$bin_dir=(defined($VARS{'BIN'}))?$VARS{'BIN'}:'';

# $bin_dir.=$o causes a core dump on my sparc :-(

$NT=0;

push(@INC,"util/pl","pl");
if ($platform eq "VC-MSDOS")
	{
	$asmbits=16;
	$msdos=1;
	require 'VC-16.pl';
	}
elsif ($platform eq "VC-W31-16")
	{
	$asmbits=16;
	$msdos=1; $win16=1;
	require 'VC-16.pl';
	}
elsif (($platform eq "VC-W31-32") || ($platform eq "VC-WIN16"))
	{
	$asmbits=32;
	$msdos=1; $win16=1;
	require 'VC-16.pl';
	}
elsif (($platform eq "VC-WIN32") || ($platform eq "VC-NT"))
	{
	$NT = 1 if $platform eq "VC-NT";
	require 'VC-32.pl';
	}
elsif ($platform eq "Mingw32")
	{
	require 'Mingw32.pl';
	}
elsif ($platform eq "Mingw32-files")
	{
	require 'Mingw32f.pl';
	}
elsif ($platform eq "BC-NT")
	{
	$bc=1;
	require 'BC-32.pl';
	}
elsif ($platform eq "BC-W31")
	{
	$bc=1;
	$msdos=1; $w16=1;
	require 'BC-16.pl';
	}
elsif ($platform eq "BC-Q16")
	{
	$msdos=1; $w16=1; $shlib=0; $qw=1;
	require 'BC-16.pl';
	}
elsif ($platform eq "BC-MSDOS")
	{
	$asmbits=16;
	$msdos=1;
	require 'BC-16.pl';
	}
elsif ($platform eq "FreeBSD")
	{
	require 'unix.pl';
	$cflags='-DTERMIO -D_ANSI_SOURCE -O2 -fomit-frame-pointer';
	}
elsif ($platform eq "linux-elf")
	{
	require "unix.pl";
	require "linux.pl";
	$unix=1;
	}
elsif ($platform eq "ultrix-mips")
	{
	require "unix.pl";
	require "ultrix.pl";
	$unix=1;
	}
else
	{
	require "unix.pl";

	$unix=1;
	$cflags.=' -DTERMIO';
	}

$out_dir=(defined($VARS{'OUT'}))?$VARS{'OUT'}:$out_def.($debug?".dbg":"");
$tmp_dir=(defined($VARS{'TMP'}))?$VARS{'TMP'}:$tmp_def.($debug?".dbg":"");
$inc_dir=(defined($VARS{'INC'}))?$VARS{'INC'}:$inc_def;

$bin_dir=$bin_dir.$o unless ((substr($bin_dir,-1,1) eq $o) || ($bin_dir eq ''));

$cflags.=" -DNO_IDEA" if $no_idea;
$cflags.=" -DNO_RC2"  if $no_rc2;
$cflags.=" -DNO_RC4"  if $no_rc4;
$cflags.=" -DNO_RC5"  if $no_rc5;
$cflags.=" -DNO_MD2"  if $no_md2;
$cflags.=" -DNO_MD5"  if $no_md5;
$cflags.=" -DNO_SHA"  if $no_sha;
$cflags.=" -DNO_SHA1" if $no_sha1;
$cflags.=" -DNO_RIPEMD" if $no_rmd160;
$cflags.=" -DNO_MDC2" if $no_mdc2;
$cflags.=" -DNO_BF"  if $no_bf;
$cflags.=" -DNO_CAST" if $no_cast;
$cflags.=" -DNO_DES"  if $no_des;
$cflags.=" -DNO_RSA"  if $no_rsa;
$cflags.=" -DNO_DSA"  if $no_dsa;
$cflags.=" -DNO_DH"   if $no_dh;
$cflags.=" -DNO_SOCK" if $no_sock;
$cflags.=" -DNO_SSL2" if $no_ssl2;
$cflags.=" -DNO_SSL3" if $no_ssl3;
$cflags.=" -DNO_ERR"  if $no_err;
$cflags.=" -DRSAref"  if $rsaref ne "";

if ($unix)
	{ $cflags="$c_flags" if ($c_flags ne ""); }
else	{ $cflags="$c_flags$cflags" if ($c_flags ne ""); }

$ex_libs="$l_flags$ex_libs" if ($l_flags ne "");

if ($msdos)
	{
	$banner ="\t\@echo Make sure you have run 'perl Configure $platform' in the\n";
	$banner.="\t\@echo top level directory, if you don't have perl, you will\n";
	$banner.="\t\@echo need to probably edit crypto/bn/bn.h, check the\n";
	$banner.="\t\@echo documentation for details.\n";
	}

# have to do this to allow $(CC) under unix
$link="$bin_dir$link" if ($link !~ /^\$/);

$INSTALLTOP =~ s|/|$o|g;

$defs= <<"EOF";
# This makefile has been automatically generated from the OpenSSL distribution.
# This single makefile will build the complete OpenSSL distribution and
# by default leave the 'intertesting' output files in .${o}out and the stuff
# that needs deleting in .${o}tmp.
# The file was generated by running 'make makefile.one', which
# does a 'make files', which writes all the environment variables from all
# the makefiles to the file call MINFO.  This file is used by
# util${o}mk1mf.pl to generate makefile.one.
# The 'makefile per directory' system suites me when developing this
# library and also so I can 'distribute' indervidual library sections.
# The one monster makefile better suits building in non-unix
# environments.

INSTALLTOP=$INSTALLTOP

# Set your compiler options
PLATFORM=$platform
CC=$bin_dir${cc}
CFLAG=$cflags
APP_CFLAG=$app_cflag
LIB_CFLAG=$lib_cflag
SHLIB_CFLAG=$shl_cflag
APP_EX_OBJ=$app_ex_obj
SHLIB_EX_OBJ=$shlib_ex_obj
# add extra libraries to this define, for solaris -lsocket -lnsl would
# be added
EX_LIBS=$ex_libs

# The OpenSSL directory
SRC_D=$src_dir

LINK=$link
LFLAGS=$lflags

BN_ASM_OBJ=$bn_asm_obj
BN_ASM_SRC=$bn_asm_src
DES_ENC_OBJ=$des_enc_obj
DES_ENC_SRC=$des_enc_src
BF_ENC_OBJ=$bf_enc_obj
BF_ENC_SRC=$bf_enc_src
CAST_ENC_OBJ=$cast_enc_obj
CAST_ENC_SRC=$cast_enc_src
RC4_ENC_OBJ=$rc4_enc_obj
RC4_ENC_SRC=$rc4_enc_src
RC5_ENC_OBJ=$rc5_enc_obj
RC5_ENC_SRC=$rc5_enc_src
MD5_ASM_OBJ=$md5_asm_obj
MD5_ASM_SRC=$md5_asm_src
SHA1_ASM_OBJ=$sha1_asm_obj
SHA1_ASM_SRC=$sha1_asm_src
RMD160_ASM_OBJ=$rmd160_asm_obj
RMD160_ASM_SRC=$rmd160_asm_src

# The output directory for everything intersting
OUT_D=$out_dir
# The output directory for all the temporary muck
TMP_D=$tmp_dir
# The output directory for the header files
INC_D=$inc_dir
INCO_D=$inc_dir${o}openssl

CP=$cp
RM=$rm
RANLIB=$ranlib
MKDIR=$mkdir
MKLIB=$bin_dir$mklib
MLFLAGS=$mlflags
ASM=$bin_dir$asm

######################################################
# You should not need to touch anything below this point
######################################################

E_EXE=openssl
SSL=$ssl
CRYPTO=$crypto
RSAGLUE=$RSAglue

# BIN_D  - Binary output directory
# TEST_D - Binary test file output directory
# LIB_D  - library output directory
# Note: if you change these point to different directories then uncomment out
# the lines around the 'NB' comment below.
# 
BIN_D=\$(OUT_D)
TEST_D=\$(OUT_D)
LIB_D=\$(OUT_D)

# INCL_D - local library directory
# OBJ_D  - temp object file directory
OBJ_D=\$(TMP_D)
INCL_D=\$(TMP_D)

O_SSL=     \$(LIB_D)$o$plib\$(SSL)$shlibp
O_CRYPTO=  \$(LIB_D)$o$plib\$(CRYPTO)$shlibp
O_RSAGLUE= \$(LIB_D)$o$plib\$(RSAGLUE)$libp
SO_SSL=    $plib\$(SSL)$so_shlibp
SO_CRYPTO= $plib\$(CRYPTO)$so_shlibp
L_SSL=     \$(LIB_D)$o$plib\$(SSL)$libp
L_CRYPTO=  \$(LIB_D)$o$plib\$(CRYPTO)$libp

L_LIBS= \$(L_SSL) \$(L_CRYPTO)
#L_LIBS= \$(O_SSL) \$(O_RSAGLUE) -lrsaref \$(O_CRYPTO)

######################################################
# Don't touch anything below this point
######################################################

INC=-I\$(INC_D) -I\$(INCL_D)
APP_CFLAGS=\$(INC) \$(CFLAG) \$(APP_CFLAG)
LIB_CFLAGS=\$(INC) \$(CFLAG) \$(LIB_CFLAG)
SHLIB_CFLAGS=\$(INC) \$(CFLAG) \$(LIB_CFLAG) \$(SHLIB_CFLAG)
LIBS_DEP=\$(O_CRYPTO) \$(O_RSAGLUE) \$(O_SSL)

#############################################
EOF

$rules=<<"EOF";
all: banner \$(TMP_D) \$(BIN_D) \$(TEST_D) \$(LIB_D) \$(INCO_D) headers lib exe

banner:
$banner

\$(TMP_D):
	\$(MKDIR) \$(TMP_D)
# NB: uncomment out these lines if BIN_D, TEST_D and LIB_D are different
#\$(BIN_D):
#	\$(MKDIR) \$(BIN_D)
#
#\$(TEST_D):
#	\$(MKDIR) \$(TEST_D)

\$(LIB_D):
	\$(MKDIR) \$(LIB_D)

\$(INCO_D): \$(INC_D)
	\$(MKDIR) \$(INCO_D)

\$(INC_D):
	\$(MKDIR) \$(INC_D)

headers: \$(HEADER) \$(EXHEADER)

lib: \$(LIBS_DEP)

exe: \$(T_EXE) \$(BIN_D)$o\$(E_EXE)$exep

install:
	\$(MKDIR) \$(INSTALLTOP)
	\$(MKDIR) \$(INSTALLTOP)${o}bin
	\$(MKDIR) \$(INSTALLTOP)${o}include
	\$(MKDIR) \$(INSTALLTOP)${o}include${o}openssl
	\$(MKDIR) \$(INSTALLTOP)${o}lib
	\$(CP) \$(INCO_D)${o}*.\[ch\] \$(INSTALLTOP)${o}include${o}openssl
	\$(CP) \$(BIN_D)$o\$(E_EXE)$exep \$(INSTALLTOP)${o}bin
	\$(CP) \$(O_SSL) \$(INSTALLTOP)${o}lib
	\$(CP) \$(O_CRYPTO) \$(INSTALLTOP)${o}lib

clean:
	\$(RM) \$(TMP_D)$o*.*

vclean:
	\$(RM) \$(TMP_D)$o*.*
	\$(RM) \$(OUT_D)$o*.*

EOF
    
my $platform_cpp_symbol = "MK1MF_PLATFORM_$platform";
$platform_cpp_symbol =~ s/-/_/g;
if (open(IN,"crypto/buildinf.h"))
	{
	# Remove entry for this platform in existing file buildinf.h.

	my $old_buildinf_h = "";
	while (<IN>)
		{
		if (/^\#ifdef $platform_cpp_symbol$/)
			{
			while (<IN>) { last if (/^\#endif/); }
			}
		else
			{
			$old_buildinf_h .= $_;
			}
		}
	close(IN);

	open(OUT,">crypto/buildinf.h") || die "Can't open buildinf.h";
	print OUT $old_buildinf_h;
	close(OUT);
	}

open (OUT,">>crypto/buildinf.h") || die "Can't open buildinf.h";
printf OUT <<EOF;
#ifdef $platform_cpp_symbol
  /* auto-generated/updated by util/mk1mf.pl for crypto/cversion.c */
  #define CFLAGS "$cc $cflags"
  #define PLATFORM "$platform"
EOF
printf OUT "  #define DATE \"%s\"\n", scalar gmtime();
printf OUT "#endif\n";
close(OUT);

#############################################
# We parse in input file and 'store' info for later printing.
open(IN,"<$infile") || die "unable to open $infile:$!\n";
$_=<IN>;
for (;;)
	{
	chop;

	($key,$val)=/^([^=]+)=(.*)/;
	if ($key eq "RELATIVE_DIRECTORY")
		{
		if ($lib ne "")
			{
			$uc=$lib;
			$uc =~ s/^lib(.*)\.a/$1/;
			$uc =~ tr/a-z/A-Z/;
			$lib_nam{$uc}=$uc;
			$lib_obj{$uc}.=$libobj." ";
			}
		last if ($val eq "FINISHED");
		$lib="";
		$libobj="";
		$dir=$val;
		}

	if ($key eq "TEST")
		{ $test.=&var_add($dir,$val); }

	if (($key eq "PROGS") || ($key eq "E_OBJ"))
		{ $e_exe.=&var_add($dir,$val); }

	if ($key eq "LIB")
		{
		$lib=$val;
		$lib =~ s/^.*\/([^\/]+)$/$1/;
		}

	if ($key eq "EXHEADER")
		{ $exheader.=&var_add($dir,$val); }

	if ($key eq "HEADER")
		{ $header.=&var_add($dir,$val); }

	if ($key eq "LIBOBJ")
		{ $libobj=&var_add($dir,$val); }

	if (!($_=<IN>))
		{ $_="RELATIVE_DIRECTORY=FINISHED\n"; }
	}
close(IN);

# Strip of trailing ' '
foreach (keys %lib_obj) { $lib_obj{$_}=&clean_up_ws($lib_obj{$_}); }
$test=&clean_up_ws($test);
$e_exe=&clean_up_ws($e_exe);
$exheader=&clean_up_ws($exheader);
$header=&clean_up_ws($header);

# First we strip the exheaders from the headers list
foreach (split(/\s+/,$exheader)){ $h{$_}=1; }
foreach (split(/\s+/,$header))	{ $h.=$_." " unless $h{$_}; }
chop($h); $header=$h;

$defs.=&do_defs("HEADER",$header,"\$(INCL_D)",".h");
$rules.=&do_copy_rule("\$(INCL_D)",$header,".h");

$defs.=&do_defs("EXHEADER",$exheader,"\$(INCO_D)",".h");
$rules.=&do_copy_rule("\$(INCO_D)",$exheader,".h");

$defs.=&do_defs("T_OBJ",$test,"\$(OBJ_D)",$obj);
$rules.=&do_compile_rule("\$(OBJ_D)",$test,"\$(APP_CFLAGS)");

$defs.=&do_defs("E_OBJ",$e_exe,"\$(OBJ_D)",$obj);
$rules.=&do_compile_rule("\$(OBJ_D)",$e_exe,'-DMONOLITH $(APP_CFLAGS)');

foreach (values %lib_nam)
	{
	$lib_obj=$lib_obj{$_};
	local($slib)=$shlib;

	$slib=0 if ($_ eq "RSAGLUE");

	if (($_ eq "SSL") && $no_ssl2 && $no_ssl3)
		{
		$rules.="\$(O_SSL):\n\n"; 
		next;
		}

	if (($_ eq "RSAGLUE") && $no_rsa)
		{
		$rules.="\$(O_RSAGLUE):\n\n"; 
		next;
		}

	if (($bn_asm_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s\S*\/bn_asm\S*/ \$(BN_ASM_OBJ)/;
		$rules.=&do_asm_rule($bn_asm_obj,$bn_asm_src);
		}
	if (($des_enc_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s\S*des_enc\S*/ \$(DES_ENC_OBJ)/;
		$lib_obj =~ s/\s\S*\/fcrypt_b\S*\s*/ /;
		$rules.=&do_asm_rule($des_enc_obj,$des_enc_src);
		}
	if (($bf_enc_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s\S*\/bf_enc\S*/ \$(BF_ENC_OBJ)/;
		$rules.=&do_asm_rule($bf_enc_obj,$bf_enc_src);
		}
	if (($cast_enc_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/(\s\S*\/c_enc\S*)/ \$(CAST_ENC_OBJ)/;
		$rules.=&do_asm_rule($cast_enc_obj,$cast_enc_src);
		}
	if (($rc4_enc_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s\S*\/rc4_enc\S*/ \$(RC4_ENC_OBJ)/;
		$rules.=&do_asm_rule($rc4_enc_obj,$rc4_enc_src);
		}
	if (($rc5_enc_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s\S*\/rc5_enc\S*/ \$(RC5_ENC_OBJ)/;
		$rules.=&do_asm_rule($rc5_enc_obj,$rc5_enc_src);
		}
	if (($md5_asm_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s(\S*\/md5_dgst\S*)/ $1 \$(MD5_ASM_OBJ)/;
		$rules.=&do_asm_rule($md5_asm_obj,$md5_asm_src);
		}
	if (($sha1_asm_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s(\S*\/sha1dgst\S*)/ $1 \$(SHA1_ASM_OBJ)/;
		$rules.=&do_asm_rule($sha1_asm_obj,$sha1_asm_src);
		}
	if (($rmd160_asm_obj ne "") && ($_ eq "CRYPTO"))
		{
		$lib_obj =~ s/\s(\S*\/rmd_dgst\S*)/ $1 \$(RMD160_ASM_OBJ)/;
		$rules.=&do_asm_rule($rmd160_asm_obj,$rmd160_asm_src);
		}
	$defs.=&do_defs(${_}."OBJ",$lib_obj,"\$(OBJ_D)",$obj);
	$lib=($slib)?" \$(SHLIB_CFLAGS)":" \$(LIB_CFLAGS)";
	$rules.=&do_compile_rule("\$(OBJ_D)",$lib_obj{$_},$lib);
	}

$defs.=&do_defs("T_EXE",$test,"\$(TEST_D)",$exep);
foreach (split(/\s+/,$test))
	{
	$t=&bname($_);
	$tt="\$(OBJ_D)${o}$t${obj}";
	$rules.=&do_link_rule("\$(TEST_D)$o$t$exep",$tt,"\$(LIBS_DEP)","\$(L_LIBS) \$(EX_LIBS)");
	}

$rules.= &do_lib_rule("\$(SSLOBJ)","\$(O_SSL)",$ssl,$shlib,"\$(SO_SSL)");
$rules.= &do_lib_rule("\$(RSAGLUEOBJ)","\$(O_RSAGLUE)",$RSAglue,0,"")
	unless $no_rsa;
$rules.= &do_lib_rule("\$(CRYPTOOBJ)","\$(O_CRYPTO)",$crypto,$shlib,"\$(SO_CRYPTO)");

$rules.=&do_link_rule("\$(BIN_D)$o\$(E_EXE)$exep","\$(E_OBJ)","\$(LIBS_DEP)","\$(L_LIBS) \$(EX_LIBS)");

print $defs;
print "###################################################################\n";
print $rules;

###############################################
# strip off any trailing .[och] and append the relative directory
# also remembering to do nothing if we are in one of the dropped
# directories
sub var_add
	{
	local($dir,$val)=@_;
	local(@a,$_,$ret);

	return("") if $no_idea && $dir =~ /\/idea/;
	return("") if $no_rc2  && $dir =~ /\/rc2/;
	return("") if $no_rc4  && $dir =~ /\/rc4/;
	return("") if $no_rc5  && $dir =~ /\/rc5/;
	return("") if $no_rsa  && $dir =~ /\/rsa/;
	return("") if $no_rsa  && $dir =~ /^rsaref/;
	return("") if $no_dsa  && $dir =~ /\/dsa/;
	return("") if $no_dh   && $dir =~ /\/dh/;
	if ($no_des && $dir =~ /\/des/)
		{
		if ($val =~ /read_pwd/)
			{ return("$dir/read_pwd "); }
		else
			{ return(""); }
		}
	return("") if $no_mdc2 && $dir =~ /\/mdc2/;
	return("") if $no_sock && $dir =~ /\/proxy/;
	return("") if $no_bf   && $dir =~ /\/bf/;
	return("") if $no_cast && $dir =~ /\/cast/;

	$val =~ s/^\s*(.*)\s*$/$1/;
	@a=split(/\s+/,$val);
	grep(s/\.[och]$//,@a);

	@a=grep(!/^e_.*_3d$/,@a) if $no_des;
	@a=grep(!/^e_.*_d$/,@a) if $no_des;
	@a=grep(!/^e_.*_i$/,@a) if $no_idea;
	@a=grep(!/^e_.*_r2$/,@a) if $no_rc2;
	@a=grep(!/^e_.*_r5$/,@a) if $no_rc5;
	@a=grep(!/^e_.*_bf$/,@a) if $no_bf;
	@a=grep(!/^e_.*_c$/,@a) if $no_cast;
	@a=grep(!/^e_rc4$/,@a) if $no_rc4;

	@a=grep(!/(^s2_)|(^s23_)/,@a) if $no_ssl2;
	@a=grep(!/(^s3_)|(^s23_)/,@a) if $no_ssl3;

	@a=grep(!/(_sock$)|(_acpt$)|(_conn$)|(^pxy_)/,@a) if $no_sock;

	@a=grep(!/(^md2)|(_md2$)/,@a) if $no_md2;
	@a=grep(!/(^md5)|(_md5$)/,@a) if $no_md5;
	@a=grep(!/(rmd)|(ripemd)/,@a) if $no_rmd160;

	@a=grep(!/(^d2i_r_)|(^i2d_r_)/,@a) if $no_rsa;
	@a=grep(!/(^p_open$)|(^p_seal$)/,@a) if $no_rsa;
	@a=grep(!/(^pem_seal$)/,@a) if $no_rsa;

	@a=grep(!/(m_dss$)|(m_dss1$)/,@a) if $no_dsa;
	@a=grep(!/(^d2i_s_)|(^i2d_s_)|(_dsap$)/,@a) if $no_dsa;

	@a=grep(!/^n_pkey$/,@a) if $no_rsa || $no_rc4;

	@a=grep(!/_dhp$/,@a) if $no_dh;

	@a=grep(!/(^sha[^1])|(_sha$)|(m_dss$)/,@a) if $no_sha;
	@a=grep(!/(^sha1)|(_sha1$)|(m_dss1$)/,@a) if $no_sha1;
	@a=grep(!/_mdc2$/,@a) if $no_mdc2;

	@a=grep(!/(^rsa$)|(^genrsa$)/,@a) if $no_rsa;
	@a=grep(!/(^dsa$)|(^gendsa$)|(^dsaparam$)/,@a) if $no_dsa;
	@a=grep(!/^gendsa$/,@a) if $no_sha1;
	@a=grep(!/(^dh$)|(^gendh$)/,@a) if $no_dh;

	@a=grep(!/(^dh)|(_sha1$)|(m_dss1$)/,@a) if $no_sha1;

	grep($_="$dir/$_",@a);
	@a=grep(!/(^|\/)s_/,@a) if $no_sock;
	@a=grep(!/(^|\/)bio_sock/,@a) if $no_sock;
	$ret=join(' ',@a)." ";
	return($ret);
	}

# change things so that each 'token' is only separated by one space
sub clean_up_ws
	{
	local($w)=@_;

	$w =~ s/^\s*(.*)\s*$/$1/;
	$w =~ s/\s+/ /g;
	return($w);
	}

sub do_defs
	{
	local($var,$files,$location,$postfix)=@_;
	local($_,$ret,$pf);
	local(*OUT,$tmp,$t);

	$files =~ s/\//$o/g if $o ne '/';
	$ret="$var="; 
	$n=1;
	$Vars{$var}.="";
	foreach (split(/ /,$files))
		{
		$orig=$_;
		$_=&bname($_) unless /^\$/;
		if ($n++ == 2)
			{
			$n=0;
			$ret.="\\\n\t";
			}
		if (($_ =~ /bss_file/) && ($postfix eq ".h"))
			{ $pf=".c"; }
		else	{ $pf=$postfix; }
		if ($_ =~ /BN_ASM/)	{ $t="$_ "; }
		elsif ($_ =~ /DES_ENC/)	{ $t="$_ "; }
		elsif ($_ =~ /BF_ENC/)	{ $t="$_ "; }
		elsif ($_ =~ /CAST_ENC/){ $t="$_ "; }
		elsif ($_ =~ /RC4_ENC/)	{ $t="$_ "; }
		elsif ($_ =~ /RC5_ENC/)	{ $t="$_ "; }
		elsif ($_ =~ /MD5_ASM/)	{ $t="$_ "; }
		elsif ($_ =~ /SHA1_ASM/){ $t="$_ "; }
		elsif ($_ =~ /RMD160_ASM/){ $t="$_ "; }
		else	{ $t="$location${o}$_$pf "; }

		$Vars{$var}.="$t ";
		$ret.=$t;
		}
	chop($ret);
	$ret.="\n\n";
	return($ret);
	}

# return the name with the leading path removed
sub bname
	{
	local($ret)=@_;
	$ret =~ s/^.*[\\\/]([^\\\/]+)$/$1/;
	return($ret);
	}


##############################################################
# do a rule for each file that says 'compile' to new direcory
# compile the files in '$files' into $to
sub do_compile_rule
	{
	local($to,$files,$ex)=@_;
	local($ret,$_,$n);
	
	$files =~ s/\//$o/g if $o ne '/';
	foreach (split(/\s+/,$files))
		{
		$n=&bname($_);
		$ret.=&cc_compile_target("$to${o}$n$obj","${_}.c",$ex)
		}
	return($ret);
	}

##############################################################
# do a rule for each file that says 'compile' to new direcory
sub cc_compile_target
	{
	local($target,$source,$ex_flags)=@_;
	local($ret);
	
	$ex_flags.=" -DMK1MF_BUILD -D$platform_cpp_symbol" if ($source =~ /cversion/);
	$target =~ s/\//$o/g if $o ne "/";
	$source =~ s/\//$o/g if $o ne "/";
	$ret ="$target: \$(SRC_D)$o$source\n\t";
	$ret.="\$(CC) ${ofile}$target $ex_flags -c \$(SRC_D)$o$source\n\n";
	return($ret);
	}

##############################################################
sub do_asm_rule
	{
	local($target,$src)=@_;
	local($ret,@s,@t,$i);

	$target =~ s/\//$o/g if $o ne "/";
	$src =~ s/\//$o/g if $o ne "/";

	@s=split(/\s+/,$src);
	@t=split(/\s+/,$target);

	for ($i=0; $i<=$#s; $i++)
		{
		$ret.="$t[$i]: $s[$i]\n";
		$ret.="\t\$(ASM) $afile$t[$i] \$(SRC_D)$o$s[$i]\n\n";
		}
	return($ret);
	}

sub do_shlib_rule
	{
	local($n,$def)=@_;
	local($ret,$nn);
	local($t);

	($nn=$n) =~ tr/a-z/A-Z/;
	$ret.="$n.dll: \$(${nn}OBJ)\n";
	if ($vc && $w32)
		{
		$ret.="\t\$(MKSHLIB) $efile$n.dll $def @<<\n  \$(${nn}OBJ_F)\n<<\n";
		}
	$ret.="\n";
	return($ret);
	}

# do a rule for each file that says 'copy' to new direcory on change
sub do_copy_rule
	{
	local($to,$files,$p)=@_;
	local($ret,$_,$n,$pp);
	
	$files =~ s/\//$o/g if $o ne '/';
	foreach (split(/\s+/,$files))
		{
		$n=&bname($_);
		if ($n =~ /bss_file/)
			{ $pp=".c"; }
		else	{ $pp=$p; }
		$ret.="$to${o}$n$pp: \$(SRC_D)$o$_$pp\n\t\$(CP) \$(SRC_D)$o$_$pp $to${o}$n$pp\n\n";
		}
	return($ret);
	}

sub read_options
	{
	if    (/^no-rc2$/)	{ $no_rc2=1; }
	elsif (/^no-rc4$/)	{ $no_rc4=1; }
	elsif (/^no-rc5$/)	{ $no_rc5=1; }
	elsif (/^no-idea$/)	{ $no_idea=1; }
	elsif (/^no-des$/)	{ $no_des=1; }
	elsif (/^no-bf$/)	{ $no_bf=1; }
	elsif (/^no-cast$/)	{ $no_cast=1; }
	elsif (/^no-md2$/)  	{ $no_md2=1; }
	elsif (/^no-md5$/)	{ $no_md5=1; }
	elsif (/^no-sha$/)	{ $no_sha=1; }
	elsif (/^no-sha1$/)	{ $no_sha1=1; }
	elsif (/^no-ripemd$/)	{ $no_ripemd=1; }
	elsif (/^no-mdc2$/)	{ $no_mdc2=1; }
	elsif (/^no-patents$/)	{ $no_rc2=$no_rc4=$no_rc5=$no_idea=$no_rsa=1; }
	elsif (/^no-rsa$/)	{ $no_rsa=1; }
	elsif (/^no-dsa$/)	{ $no_dsa=1; }
	elsif (/^no-dh$/)	{ $no_dh=1; }
	elsif (/^no-hmac$/)	{ $no_hmac=1; }
	elsif (/^no-asm$/)	{ $no_asm=1; }
	elsif (/^nasm$/)	{ $nasm=1; }
	elsif (/^no-ssl2$/)	{ $no_ssl2=1; }
	elsif (/^no-ssl3$/)	{ $no_ssl3=1; }
	elsif (/^no-err$/)	{ $no_err=1; }
	elsif (/^no-sock$/)	{ $no_sock=1; }

	elsif (/^just-ssl$/)	{ $no_rc2=$no_idea=$no_des=$no_bf=$no_cast=1;
				  $no_md2=$no_sha=$no_mdc2=$no_dsa=$no_dh=1;
				  $no_ssl2=$no_err=$no_rmd160=$no_rc5=1; }

	elsif (/^rsaref$/)	{ $rsaref=1; }
	elsif (/^gcc$/)		{ $gcc=1; }
	elsif (/^debug$/)	{ $debug=1; }
	elsif (/^shlib$/)	{ $shlib=1; }
	elsif (/^dll$/)		{ $shlib=1; }
	elsif (/^([^=]*)=(.*)$/){ $VARS{$1}=$2; }
	elsif (/^-[lL].*$/)	{ $l_flags.="$_ "; }
	elsif ((!/^-help/) && (!/^-h/) && (!/^-\?/) && /^-.*$/)
		{ $c_flags.="$_ "; }
	else { return(0); }
	return(1);
	}
