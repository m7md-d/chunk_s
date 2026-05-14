#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct pmd
{
    char name[256];
    long chunk_size;
    int extra_bytes;
    int num_chunks;
} pmd;

/** Get the size of an open file without altering the file position */
long get_file_size(FILE *file)
{
    long saved_pos = ftell(file), file_size;
    
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    fseek(file, saved_pos, SEEK_SET);
    return file_size;
}

/** Check the integrity of the PMD file */
int check_pmd(pmd *meta, long file_size)
{
    int extra_bytes = 0, i = 0;
    long chunk_size;
    char chunk_name[256];
    FILE *tmp;

    if (file_size > 1048576L)
    {
        fprintf(stderr, "Error: File size exceeds 1MB limit\n");
        return (4);
    }

    for (i = 0; i < meta->num_chunks; i++)
    {
        if (i == meta->num_chunks - 1)
            extra_bytes = meta->extra_bytes;

        sprintf(chunk_name, "%s.%d", meta->name, i);
        tmp = fopen(chunk_name, "rb");
        if (!tmp)
        {
            fprintf(stderr, "Error: Cannot open chunk file '%s'\n", chunk_name);
            return (3);
        }
        chunk_size = get_file_size(tmp);
        if (chunk_size != meta->chunk_size + extra_bytes)
        {
            fprintf(stderr, "Error: Chunk '%s' has unexpected size (expected %ld, got %ld)\n",
                    chunk_name, meta->chunk_size + extra_bytes, chunk_size);
            return (5);
        }
        fclose(tmp);
    }
    return (0);
}

/** Merge chunks into a single file */
int merge(int argc, char **argv)
{
    int extra_bytes = 0, i = 0, r = 0, n;
    long file_size;
    char chunk_name[256], *file_out, file_name[256];
    size_t bytes_read;
    pmd meta;
    FILE *pmd_file = fopen(argv[2], "rb"), *chunk, *tmp;

    if (!pmd_file)
    {
        fprintf(stderr, "Error: Cannot open PMD file '%s'\n", argv[2]);
        return (3);
    }

    fread(&meta, 1, sizeof(pmd), pmd_file);
    fclose(pmd_file);
    file_size = (meta.chunk_size * meta.num_chunks) + meta.extra_bytes;

    if (check_pmd(&meta, file_size))
    {
        fprintf(stderr, "Error: PMD integrity check failed\n");
        return 1;
    }

    if (argc == 3)
        sprintf(file_name, "%s.m",meta.name);
    else if (argc == 6 && !strcmp(argv[3], "-r"))
    {
        r = atoi(argv[4]);
        strcpy(file_name, argv[5]);
    }
    else
    {
        fprintf(stderr, "Usage: %s -m <pmd_file> [ -r <start_chunk> <output_file> ]\n", argv[0]);
        exit(1);
    }
    file_out = malloc(file_size);

    sprintf(chunk_name, "%s.m", meta.name);
    tmp = fopen(file_name, "wb");

    for (n = r, i = 0; i < meta.num_chunks; i++, n++)
    {
        if (n == meta.num_chunks - 1)
            extra_bytes = meta.extra_bytes;
        if (n == meta.num_chunks)
        {
            n = 0;
            extra_bytes = 0;
        }

        sprintf(chunk_name, "%s.%d", meta.name, n);
        chunk = fopen(chunk_name, "rb");
        bytes_read = fread(file_out, 1, meta.chunk_size + extra_bytes, chunk);
        fwrite(file_out, bytes_read, 1, tmp);
        fclose(chunk);
    }
    fclose(tmp);
    free(file_out);

    return (0);
}

/** Parse a file into chunks */
int pars_file(char **argv)
{
    int num_chunks, extra_bytes = 0, i;
    long chunk_size, file_size;
    FILE *src_file, *chunk_file;
    char *chunk_buffer, chunk_name[256];
    size_t bytes_read;
    pmd meta;

    num_chunks = atoi(argv[3]);
    if (num_chunks <= 0 || num_chunks > 8)
    {
        fprintf(stderr, "Error: Number of chunks must be between 1 and 8 (got %d)\n", num_chunks);
        return (2);
    }

    src_file = fopen(argv[2], "rb");
    if (!src_file)
    {
        fprintf(stderr, "Error: Cannot open source file '%s'\n", argv[2]);
        return (3);
    }

    file_size = get_file_size(src_file);

    if ((long)num_chunks > file_size)
    {
        fprintf(stderr, "Error: More chunks than bytes in file\n");
        return (4);
    }

    if (file_size > 1048576L)
    {
        fprintf(stderr, "Error: File size exceeds 1MB limit\n");
        return (4);
    }

    chunk_size = file_size / num_chunks;

    chunk_buffer = malloc(chunk_size + file_size % num_chunks);

    for (i = 0; i < num_chunks; i++)
    {
        if (i == num_chunks - 1)
            extra_bytes = file_size % num_chunks;

        bytes_read = fread(chunk_buffer, 1, chunk_size + extra_bytes, src_file);

        sprintf(chunk_name, "%s.%d", argv[2], i);
        chunk_file = fopen(chunk_name, "wb");

        fwrite(chunk_buffer, 1, bytes_read, chunk_file);

        fclose(chunk_file);
    }
    free(chunk_buffer);

    meta.chunk_size = chunk_size;
    meta.extra_bytes = extra_bytes;
    strcpy(meta.name, argv[2]);
    meta.num_chunks = num_chunks;
    
    sprintf(chunk_name, "%s.pmd", argv[2]);
    FILE *m =fopen(chunk_name, "wb");
    fwrite(&meta, 1,sizeof(pmd),m);
    fclose(m);

    return (0);
}

/** Split a binary file into n chunks (1 <= n <= 8) */
int main(int argc, char **argv)
{
    int result = 0;

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s -p <file> <num_chunks>\n", argv[0]);
        fprintf(stderr, "       %s -m <pmd_file> [start_chunk output_file]\n", argv[0]);
        return (1);
    }
    
    if (!strcmp(argv[1], "-p"))
        result = pars_file(argv);

    else if (!strcmp(argv[1], "-m"))
        result = merge(argc, argv);
    else
    {
        fprintf(stderr, "Error: Unknown option '%s' (use -p or -m)\n", argv[1]);
        result = 1;
    }

    return (result);
}
