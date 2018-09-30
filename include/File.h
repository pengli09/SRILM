/*
 * File.h
 *	File I/O utilities for LM
 *
 * Copyright (c) 1995, SRI International.  All Rights Reserved.
 *
 * @(#)$Header: /home/srilm/devel/misc/src/RCS/File.h,v 1.5 1999/01/22 07:11:42 stolcke Exp $
 *
 */

#ifndef _File_h_
#define _File_h_

#include <stdio.h>
#include <iostream.h>

const unsigned int maxLineLength = 10000;		// no longer used
const unsigned int maxWordsPerLine = 10000;

char *const wordSeparators = " \t\n";

typedef FILE * FILEptr;

/*
 * A File object is a wrapper around a stdio FILE pointer.  If presently
 * provides two kinds of convenience.
 *
 * - constructors and destructors manage opening and closing of the stream.
 *   The stream is checked for errors on closing, and the default behavior
 *   is to exit() with an error message if a problem was found.
 * - the getline() method strips comments and keeps track of input line
 *   numbers for error reporting.
 * 
 * File object can be cast to (FILE *) to perform most of the standard
 * stdio operations in a seamless way.
 */
class File
{
public:
    File(const char *name, const char *mode, int exitOnError = 1);
    File(FILE *fp = 0, int exitOnError = 1);
    ~File();

    char *getline();
    int close();
    int error() { return (fp == 0) || ferror(fp); };

    operator FILEptr() { return fp; };
    ostream &position(ostream &stream = cerr);

    const char *name;
    unsigned int lineno;
    int exitOnError;

private:
    FILE *fp;
    char *buffer;
    unsigned bufLen;
};

#endif /* _File_h_ */
