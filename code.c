#include <stdio.h>
#include <stdlib.h>

/** Get the size of an open file without altering the file position */
long get_file_size(FILE *file)
{
    long saved_pos = ftell(file), file_size;
    
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, saved_pos, SEEK_SET);
    return file_size;
}

/** Split a binary file into n chunks (1 <= n <= 8) */
int main(int argc, char **argv)
{
    int num_chunks, extra_bytes = 0, i;
    long chunk_size, file_size;
    FILE *src_file, *chunk_file;
    char *chunk_buffer, chunk_name[256];
    size_t bytes_read;

    if (argc != 3) return (1);
    num_chunks = atoi(argv[2]);
    if (num_chunks <= 0 || num_chunks > 8) return (1);

    src_file = fopen(argv[1], "rb");
    if (!src_file) return (1);

    file_size = get_file_size(src_file);

    if ((long)num_chunks > file_size || file_size > 1048576L) /* 1048576 == 1MB */
        return (1);

    chunk_size = file_size / num_chunks;

    chunk_buffer = malloc(chunk_size + file_size % num_chunks);

    for (i = 0; i < num_chunks; i++)
    {
        if (i == num_chunks - 1)
            extra_bytes = file_size % num_chunks;

        bytes_read = fread(chunk_buffer, 1, chunk_size + extra_bytes, src_file);

        sprintf(chunk_name, "%s.%d", argv[1], i);
        chunk_file = fopen(chunk_name, "wb");

        fwrite(chunk_buffer, 1, bytes_read, chunk_file);

        fclose(chunk_file);
    }
    free(chunk_buffer);

    return (0);
}
