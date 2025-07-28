#include <stdio.h>
#include "parser.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <source_file>\n", argv[0]);
        return 1;
    }

    FILE *sourceCode = fopen(argv[1], "r");
    if (sourceCode == NULL) {
        printf("Error file: %s\n", argv[1]);
        return 1;
    }

    lexicalAnalyzer(sourceCode);
    fclose(sourceCode);

    return 0;
}
