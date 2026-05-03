#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum
{
    MAX_GROUP_SIZE = 64, /* I cannot see much call for anything larger than eight, for uint64_t ... */
};

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <size1> <size2> ... <sizeN> < stdin\n", argv[0]);
        fprintf(stderr, "Example: cat data | %s 1 2 4\n", argv[0]);
        return EXIT_FAILURE;
    }

    int groups[argc];
    int group_count = 0;
    int reverse = 1; /* Normal case, Intel byte ordering. I will change this and recompile if I need Motorola ordering - which is unlikely. */

    for (int arg_index = 1; arg_index < argc; ++arg_index)
    {
        char *end = NULL;
        errno = 0;
        long val = strtol(argv[arg_index], &end, 0);

        if (*end != '\0' || errno != 0 || val < 1 || val > MAX_GROUP_SIZE)
        {
            fprintf(stderr, "Invalid group size '%s' (must be 1..%d)\n", argv[arg_index], MAX_GROUP_SIZE);
            return EXIT_FAILURE;
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
            char *tokens[MAX_GROUP_SIZE]; /* No need to set to zero. */
            int token_count = 0;
            while (token_count < groups[group_index])
            {
                char *token = strsep(&scan, " \t\r\n");

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

    exit(EXIT_SUCCESS);
}
