/***************************************************************
* StreamDevice Support                                         *
*                                                              *
* (C) 1999 Dirk Zimoch (zimoch@delta.uni-dortmund.de)          *
* (C) 2005 Dirk Zimoch (dirk.zimoch@psi.ch)                    *
*                                                              *
* This is the binary format converter of StreamDevice.         *
* Please refer to the HTML files in ../doc/ for a detailed     *
* documentation.                                               *
*                                                              *
* If you do any changes in this file, you are not allowed to   *
* redistribute it any more. If there is a bug or a missing     *
* feature, send me an email and/or your patch. If I accept     *
* your changes, they will go to the next release.              *
*                                                              *
* DISCLAIMER: If this software breaks something or harms       *
* someone, it's your problem.                                  *
*                                                              *
***************************************************************/

#include <ctype.h>
#include <limits.h>
#include "StreamFormatConverter.h"
#include "StreamError.h"

// Binary ASCII Converter %b and %B

class BinaryConverter : public StreamFormatConverter
{
    int parse(const StreamFormat&, StreamBuffer&, const char*&, bool);
    bool printLong(const StreamFormat&, StreamBuffer&, long);
    long scanLong(const StreamFormat&, const char*, long&);
};

int BinaryConverter::
parse(const StreamFormat& fmt, StreamBuffer& info,
    const char*& source, bool)
{
    if (fmt.conv == 'b')
    {
        // default characters 0 and 1 for %b
        info.append("01");
        return unsigned_format;
    }
    
    // user defined characters for %B (next 2 in source)
    if (*source)
    {
        if (*source == esc) source++;
        info.append(*source++);
        if (*source)
        {
            if (*source == esc) source++;
            info.append(*source++);
            return unsigned_format;
        }
    }
    error("Missing characters after %%B format conversion\n");
    return false;
}

bool BinaryConverter::
printLong(const StreamFormat& fmt, StreamBuffer& output, long value)
{
    int prec = fmt.prec;
    if (prec == -1)
    {
        // Find number of significant bits is nothing is specified.
        unsigned long x = (unsigned long) value;
        prec = 32;
#if (LONG_BIT > 32)
        if (x > 0xFFFFFFFF)  { prec = 64;  x >>=32; }
#endif 
        if (x <= 0x0000FFFF) { prec -= 16; x <<=16; }
        if (x <= 0x00FFFFFF) { prec -= 8; x <<=8; }
        if (x <= 0x0FFFFFFF) { prec -= 4; x <<=4; }
        if (x <= 0x3FFFFFFF) { prec -= 2; x <<=2; }
        if (x <= 0x7FFFFFFF) { prec -= 1; }
    }
    unsigned long width = prec;
    if (fmt.width > width) width = fmt.width;
    char zero = fmt.info[0];
    char one = fmt.info[1];
    char fill = (fmt.flags & zero_flag) ? zero : ' ';
    if (fmt.flags & alt_flag)
    {
        // little endian (least significant bit first)
        if (!(fmt.flags & left_flag))
        {
            // pad left
            while (width > (unsigned int)prec)
            {
                output.append(' ');
                width--;
            }
        }
        while (prec--)
        {
            output.append((value & 1) ? one : zero);
            value >>= 1;
            width--;
        }
        while (width--)
        {
            // pad right
            output.append(fill);
        }
    }
    else
    {
        // big endian (most significant bit first)
        if (!(fmt.flags & left_flag))
        {
            // pad left
            while (width > (unsigned int)prec)
            {
                output.append(fill);
                width--;
            }
        }
        while (prec--)
        {
            output.append((value & (1L << prec)) ? one : zero);
            width--;
        }
        while (width--)
        {
            // pad right
            output.append(' ');
        }
    }
    return true;
}

long BinaryConverter::
scanLong(const StreamFormat& fmt, const char* input, long& value)
{
    long val = 0;
    long width = fmt.width;
    if (width == 0) width = -1;
    long length = 0;
    char zero = fmt.info[0];
    char one = fmt.info[1];
    if (!isspace(zero) && !isspace(one))
        while (isspace(input[length])) length++; // skip whitespaces
    if (input[length] != zero && input[length] != one) return -1;
    if (fmt.flags & alt_flag)
    {
        // little endian (least significan bit first)
        long mask = 1;
        while (width-- && (input[length] == zero || input[length] == one))
        {
            if (input[length++] == one) val |= mask;
            mask <<= 1;
        }
    }
    else
    {
        // big endian (most significan bit first)
        while (width-- && (input[length] == zero || input[length] == one))
        {
            val <<= 1;
            if (input[length++] == one) val |= 1;
        }
    }
    value = val;
    return length;
}

RegisterConverter (BinaryConverter, "bB");
