/*
 * This is an ad-hoc program for working with debug data in specific format, streaming on stdin.
 * The input data format will look something like this:
 *
 *    1777806699.117235 aa bb cc dd ee ff 00 11 22 33 44 55 66 ...
 *    1777806699.117456 aa bb cc dd ee ff 00 11 22 33 44 55 66 ...
 *    ...
 *
 * Specifically, a microsecond resolution epoch time followed by hexadecimal octets.
 *
 * In my workflow I generate the data part of those lines on a target system and then
 * fire datagrams out to a host system which timestamps them and dumps the payload as
 * octets.
 *
 * Often the payloads contain multi-byte values. In my current work the target is
 * little endian.
 *
 * This program is used in a data processing pipeline. It gloms together sequences
 * of octets which a subsequent process will convert into decimal. The final output
 * is a simple block of numerical text which Octave / Matlab ingests without further
 * ado.
 *
 * The operation is easy to explain in terms of an example.
 *
 *    :; cat example
 *    1777806699.117235 aa bb cc dd ee ff 00 11 22 33 44 55 66
 *    1777806699.117456 11 22 33 44 55 66 77 88 99 10 11 12 13 14
 *    :; cat example | ./glom 1 2 4 2 2 2
 *    1777806699.117235 bbaa ffeeddcc 1100 3322 5544
 *    1777806699.117456 2211 66554433 8877 1099 1211
 *
 * In this case, the interesting part of the payload comprises
 *
 *    * An octet (1 byte)
 *    * A uint16_t (2 bytes)
 *    * A uint32_t (4 bytes)
 *    * A uint16_t (2 bytes)
 *    * A uint16_t (2 bytes)
 *    * A uint16_t (2 bytes)
 *
 * Those type widths are in the command line arguments but notice that the
 * first argument is 1, to allow the epoch to pass straight through. You will
 * note that the bytes have been re-ordered to cope with the native little-
 * endian format in the payload.
 *
 * The next pipeline stage will convert those hex values to decimal integers.
 *
 * I have public GitHub repos which can deal with structs and convert to JSON,
 * etc. but find that for most simple case, this is the sweet spot for dealing
 * with ad-hoc debug.
 *
 * I make no claims for the accuracy, safety, and style of this program but it seems
 * adequate for the task in hand.
 *
 * Build it like this:
 *
 *     gcc -Wall -Wextra --pedantic glom.c -o glom
 *
 * In this application, I doubt that optimisation will matter.
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "This gloms hex octets together in group sizes specified as values on the command line\n");
        exit(EXIT_FAILURE);
    }

    enum
    {
        MAX_TOKENS = 0x20,
    };
    int *groups = calloc(argc, sizeof(int));
    if (!groups)
    {
        fprintf(stderr, "A small calloc failed, expect big trouble\n");
        exit(EXIT_FAILURE);
    }
    int group_count = 0;
    int reverse = 1; /* Normal case, Intel byte ordering. I will change this and recompile if I need Motorola ordering - which is unlikely. */

    for (int arg_index = 1; arg_index < argc; ++arg_index)
    {
        char *end = NULL;
        errno = 0;

        unsigned long val = strtoul(argv[arg_index], &end, 10);

        if (argv[arg_index][0] == '\0' || *end != '\0' || errno != 0 || val > INT_MAX)
        {
            fprintf(stderr, "Invalid integer argument '%s'\n", argv[arg_index]);
            exit(EXIT_FAILURE);
        }

        if (val < 1 || val > MAX_TOKENS)
        {
            fprintf(stderr, "Group lengths must be integer values in the range 1 to %d\n", MAX_TOKENS);
            exit(EXIT_FAILURE);
        }
        groups[group_count++] = (int)val;
    }

    char *line = NULL;
    size_t capacity = 0;
    int line_number = 0;
    while (getline(&line, &capacity, stdin) != -1)
    {
        char *scan = line;
        ++line_number;

        for (int group_index = 0; group_index < group_count; ++group_index)
        {
            char *tokens[MAX_TOKENS]; /* No need to set to zero. */
            int token_count = 0;
            while (token_count < groups[group_index])
            {
                char *token = strsep(&scan, " \r\n");

                if (!token)
                {
                    fprintf(stderr, "Insufficient tokens on line %d\n", line_number);
                    exit(EXIT_FAILURE);
                }
                else if (*token == '\0')
                {
                    continue; /* Runs of spaces are simply treated as a single space. */
                }
                else
                {
                    tokens[token_count++] = token;
                }
            }

            for (int index = 0; index < token_count; ++index)
            {
                printf("%s", tokens[reverse ? token_count - index - 1 : index]);
            }

            if (group_index + 1 < group_count) /* A single space between groups, no space after last group. */
            {
                printf(" ");
            }
        }
        printf("\n");
    }

    free(line);
    free(groups);
    exit(EXIT_SUCCESS);
}
