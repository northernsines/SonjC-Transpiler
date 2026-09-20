#include "io_utils.h"
#include <stdio.h>
#include <stdlib.h>

/*
SonjC IO Utilities
Written Sep 2026
File I/O helpers shared by the compiler driver.
*/

char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "couldn't open %s\n", path); //invalid file
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f); //get file size
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1); //allocate mem
    if (buffer == NULL)
    {
        fprintf(stderr, "malloc returned null for file content buffer allocation \n"); 
        exit(EXIT_FAILURE);
    }
    size_t bytesRead = fread(buffer, 1, size, f);

    if (bytesRead != (size_t)size)
    {
        fprintf(stderr, "failed to read %s\n", path);
        fclose(f);
        free(buffer);
        exit(EXIT_FAILURE);
    }
    buffer[size] = '\0';   // null-terminate to treat like normal C string

    fclose(f);
    return buffer;
}