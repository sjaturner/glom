# Introduction

This is an ad-hoc program for working with debug data in specific format,
streaming on stdin.  The input data format will look something like this:

    1777806699.117235 aa bb cc dd ee ff 00 11 22 33 44 55 66 ...
    1777806699.117456 aa bb cc dd ee ff 00 11 22 33 44 55 66 ...
    ...

Specifically, a microsecond resolution epoch time followed by hexadecimal
octets.

In my workflow I generate the data part of those lines on a target system
and then fire datagrams out to a host system which timestamps them and
dumps the payload as octets.

Often the payloads contain multi-byte values. In my current work the
target is little endian.

This program is used in a data processing pipeline. It gloms together
sequences of octets which a subsequent process will convert into
decimal. The final output is a simple block of numerical text which
Octave / Matlab ingests without further ado.

# An example

The operation is easy to explain in terms of an example.

    :; cat example
    1777806699.117235 aa bb cc dd ee ff 00 11 22 33 44 55 66
    1777806699.117456 11 22 33 44 55 66 77 88 99 10 11 12 13 14
    :; cat example | ./glom 1 2 4 2 2 2
    1777806699.117235 bbaa ffeeddcc 1100 3322 5544
    1777806699.117456 2211 66554433 8877 1099 1211

In this case, the interesting part of the payload comprises

    An octet   (1 byte)
    A uint16_t (2 bytes)
    A uint32_t (4 bytes)
    A uint16_t (2 bytes)
    A uint16_t (2 bytes)
    A uint16_t (2 bytes)

Those type widths are in the command line arguments but notice that the
first argument is 1, to allow the epoch to pass straight through. You
will note that the bytes have been re-ordered to cope with the native
little- endian format in the payload.

The next pipeline stage will convert those hex values to decimal integers.

Here is a simple example of that conversion, using GAWK:

    :; cat example | ./glom 1 2 4 2 2 2 | gawk '{printf("%s ", $1); for (x = 2; x <= NF; ++x) printf("%d ", strtonum("0x" $x));printf("\n");}'
    1777806699.117235 48042 4293844428 4352 13090 21828
    1777806699.117456 8721 1716864051 34935 4249 4625

I have public GitHub repos which can deal with structs and convert to
JSON, etc. but find that for most simple case, this is the sweet spot
for dealing with ad-hoc debug.

I make no claims for the accuracy, safety, and style of this program
but it seems adequate for the task in hand.

Build it like this:

    gcc -Wall -Wextra --pedantic -g glom.c -o glom

Or just type:

    make glom

In this application, I doubt that optimisation will matter.
