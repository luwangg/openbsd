# $OpenBSD: src/lib/libcurses/tinfo/MKnames.awk,v 1.2 2001/01/22 18:01:50 millert Exp $
# $From: MKnames.awk,v 1.11 2000/12/09 23:46:13 tom Exp $
BEGIN		{
			print  "/* This file was generated by MKnames.awk */" > "namehdr"
			print  ""				> "namehdr"
			print  "#include <curses.priv.h>"	> "namehdr"
			print  ""				> "namehdr"
			print  "#define IT NCURSES_CONST char * const"	> "namehdr"
			print  ""				> "namehdr"
			print  "#if BROKEN_LINKER"		> "namehdr"
			print  "#include <term.h>"		> "namehdr"
			print  "#define DCL(it) static IT data##it[]" > "namehdr"
			print  "#else"				> "namehdr"
			print  "#define DCL(it) NCURSES_EXPORT_VAR(IT) it[]"	> "namehdr"
			print  "#endif"				> "namehdr"
			print  ""				> "namehdr"
			print  "/*"				> "boolnames"
			print  " *	names.c - Arrays of capability names and codes"  > "boolnames"
			print  " *"				> "boolnames"
			print  " */"				> "boolnames"
			print  ""				> "boolnames"
			print  "DCL(boolnames)  = {"		> "boolnames"
			print  "DCL(boolfnames) = {"		> "boolfnames"
			print  "DCL(boolcodes)  = {"		> "boolcodes"
			print  "DCL(numnames)   = {"		> "numnames"
			print  "DCL(numfnames)  = {"		> "numfnames"
			print  "DCL(numcodes)   = {"		> "numcodes"
			print  "DCL(strnames)   = {"		> "strnames"
			print  "DCL(strfnames)  = {"		> "strfnames"
			print  "DCL(strcodes)   = {"		> "strcodes"
		}

$1 ~ /^#/		{next;}

$1 == "SKIPWARN"	{next;}

$3 == "bool"	{
			printf "\t\t\"%s\",\n", $2 > "boolnames"
			printf "\t\t\"%s\",\n", $1 > "boolfnames"
			printf "\t\t\"%s\",\n", $4 > "boolcodes"
		}

$3 == "num"	{
			printf "\t\t\"%s\",\n", $2 > "numnames"
			printf "\t\t\"%s\",\n", $1 > "numfnames"
			printf "\t\t\"%s\",\n", $4 > "numcodes"
		}

$3 == "str"	{
			printf "\t\t\"%s\",\n", $2 > "strnames"
			printf "\t\t\"%s\",\n", $1 > "strfnames"
			printf "\t\t\"%s\",\n", $4 > "strcodes"
		}

END		{
			print  "\t\t(NCURSES_CONST char *)0," > "boolnames"
			print  "};" > "boolnames"
			print  "" > "boolnames"
			print  "\t\t(NCURSES_CONST char *)0," > "boolfnames"
			print  "};" > "boolfnames"
			print  "" > "boolfnames"
			print  "\t\t(NCURSES_CONST char *)0," > "boolcodes"
			print  "};" > "boolcodes"
			print  "" > "boolcodes"
			print  "\t\t(NCURSES_CONST char *)0," > "numnames"
			print  "};" > "numnames"
			print  "" > "numnames"
			print  "\t\t(NCURSES_CONST char *)0," > "numfnames"
			print  "};" > "numfnames"
			print  "" > "numfnames"
			print  "\t\t(NCURSES_CONST char *)0," > "numcodes"
			print  "};" > "numcodes"
			print  "" > "numcodes"
			print  "\t\t(NCURSES_CONST char *)0," > "strnames"
			print  "};" > "strnames"
			print  "" > "strnames"
			print  "\t\t(NCURSES_CONST char *)0," > "strfnames"
			print  "};" > "strfnames"
			print  "" > "strfnames"
			print  "\t\t(NCURSES_CONST char *)0," > "strcodes"
			print  "};"				> "strcodes"
			print  ""				> "strcodes"
			print  "#if BROKEN_LINKER"		> "nameftr"
			print  "#define FIX(it) NCURSES_IMPEXP IT * NCURSES_API _nc_##it(void) { return data##it; }" > "nameftr"
			print  "FIX(boolnames)"			> "nameftr"
			print  "FIX(boolfnames)"		> "nameftr"
			print  "FIX(numnames)"			> "nameftr"
			print  "FIX(numfnames)"			> "nameftr"
			print  "FIX(strnames)"			> "nameftr"
			print  "FIX(strfnames)"			> "nameftr"
			print  "#endif /* BROKEN_LINKER */"	> "nameftr"
			print  ""				> "codeftr"
			print  "#if BROKEN_LINKER"		> "codeftr"
			print  "#define FIX(it) NCURSES_IMPEXP IT * NCURSES_API _nc_##it(void) { return data##it; }" > "codeftr"
			print  "FIX(boolcodes)"			> "codeftr"
			print  "FIX(numcodes)"			> "codeftr"
			print  "FIX(strcodes)"			> "codeftr"
			print  "#endif /* BROKEN_LINKER */"	> "codeftr"
		}
