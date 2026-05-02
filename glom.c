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
    int line_count = 0;
    while (getline(&line, &capacity, stdin) != -1)
    {
        char *scan = line;
        ++line_count;

        for (int group_index = 0; group_index < group_count; ++group_index)
        {
            char *tokens[MAX_TOKENS]; /* No need to set to zero. */
            int token_count = 0;
            while (token_count < groups[group_index])
            {
                char *token = strsep(&scan, " \r\n");

                if (!token)
                {
                    fprintf(stderr, "Insufficient tokens on line %d\n", line_count);
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
