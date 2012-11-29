#!/usr/bin/tclsh
#
# To build a single huge source file holding all of SQLite (or at
# least the core components - the test harness, shell, and TCL 
# interface are omitted.) first do
#
#      make target_source
#
# The make target above moves all of the source code files into
# a subdirectory named "tsrc".  (This script expects to find the files
# there and will not work if they are not found.)  There are a few
# generated C code files that are also added to the tsrc directory.
# For example, the "parse.c" and "parse.h" files to implement the
# the parser are derived from "parse.y" using lemon.  And the 
# "keywordhash.h" files is generated by a program named "mkkeywordhash".
#
# After the "tsrc" directory has been created and populated, run
# this script:
#
#      tclsh mksqlite3c.tcl
#
# The amalgamated SQLite code will be written into sqlite3.c
#

# Begin by reading the "sqlite3.h" header file.  Count the number of lines
# in this file and extract the version number.  That information will be
# needed in order to generate the header of the amalgamation.
#
set in [open tsrc/sqlite3.h]
set cnt 0
set VERSION ?????
while {![eof $in]} {
  set line [gets $in]
  if {$line=="" && [eof $in]} break
  incr cnt
  regexp {#define\s+SQLITE_VERSION\s+"(.*)"} $line all VERSION
}
close $in

# Open the output file and write a header comment at the beginning
# of the file.
#
set out [open sqlite3internal.h w]
set today [clock format [clock seconds] -format "%Y-%m-%d %H:%M:%S UTC" -gmt 1]
puts $out [subst \
{/******************************************************************************
** This file is an amalgamation of many private header files from SQLite
** version $VERSION. 
*/}]

# These are the header files used by SQLite.  The first time any of these 
# files are seen in a #include statement in the C code, include the complete
# text of the file in-line.  The file only needs to be included once.
#
foreach hdr {
   btree.h
   btreeInt.h
   hash.h
   hwtime.h
   keywordhash.h
   opcodes.h
   os_common.h
   os.h
   pager.h
   parse.h
   sqlite3ext.h
   sqlite3.h
   sqliteInt.h
   sqliteLimit.h
   vdbe.h
   vdbeInt.h
} {
  set available_hdr($hdr) 1
}

# 78 stars used for comment formatting.
set s78 \
{*****************************************************************************}

# Insert a comment into the code
#
proc section_comment {text} {
  global out s78
  set n [string length $text]
  set nstar [expr {60 - $n}]
  set stars [string range $s78 0 $nstar]
  puts $out "/************** $text $stars/"
}

# Read the source file named $filename and write it into the
# sqlite3.c output file.  If any #include statements are seen,
# process them approprately.
#
proc copy_file {filename} {
  global seen_hdr available_hdr out
  set tail [file tail $filename]
  section_comment "Begin file $tail"
  set in [open $filename r]
  while {![eof $in]} {
    set line [gets $in]
    if {[regexp {^#\s*include\s+["<]([^">]+)[">]} $line all hdr]} {
      if {[info exists available_hdr($hdr)]} {
        if {$available_hdr($hdr)} {
          section_comment "Include $hdr in the middle of $tail"
          copy_file tsrc/$hdr
          section_comment "Continuing where we left off in $tail"
        }
      } elseif {![info exists seen_hdr($hdr)]} {
        set seen_hdr($hdr) 1
        puts $out $line
      }
    } elseif {[regexp {^#ifdef __cplusplus} $line]} {
      puts $out "#if 0"
    } elseif {[regexp {^#line} $line]} {
      # Skip #line directives.
    } else {
      puts $out $line
    }
  }
  close $in
  section_comment "End of $tail"
}


# Process the source files.  Process files containing commonly
# used subroutines first in order to help the compiler find
# inlining opportunities.
#
foreach file {
   sqliteInt.h
   sqlite3.h
   btree.h
   hash.h
   os.h
   pager.h
   parse.h
   sqlite3ext.h
   vdbe.h
} {
  if {$available_hdr($file)} {
    copy_file tsrc/$file
  }
}

close $out
