#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct pmd
{
    char name[256];
    long chunk_size;
    int extra_bytes;
    int num_chunks;
    unsigned short decoy_mask;
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

/** Generate a decoy mask for the specified number of slots and decoys */
unsigned short gen_decoy_mask(int slots, int num_decoys)
{
    unsigned short decoy_mask = 0, mask = (1 << slots)-1;
    int cnt = 0, bit;

    while (cnt < num_decoys)
    {
        bit = rand() % slots;
        if (!((decoy_mask >> bit) & 1))
        {
            decoy_mask |= (1 << bit);
            cnt++;
        }
    }
    return (decoy_mask & mask);
}

/** Generate a decoy chunk */
void gen_decoy_chunk(char **argv, char *chunk_buffer, long chunk_size, char *chunk_name, int idx)
{
    FILE *chunk_file;
    unsigned int state;
    size_t i;

    sprintf(chunk_name, "%s.%d", argv[2], idx);
    chunk_file = fopen(chunk_name, "wb");
    if (!chunk_file)
    {
        fprintf(stderr, "Error: Cannot create decoy chunk '%s'\n", chunk_name);
        exit(1);
    }

    state = (unsigned int)rand();

    for (i = 0; i < chunk_size; i++)
    {
        state = state * 1103515245u + 12345u * idx;
        chunk_buffer[i] = (unsigned char)state >> 16;
    }

    fwrite(chunk_buffer, 1, chunk_size, chunk_file);

    fclose(chunk_file);
}

/** Check the integrity of the PMD file */
int check_pmd(pmd *meta, long file_size)
{
    int extra_bytes = 0, i = 0;
    long chunk_size;
    char chunk_name[256];
    FILE *chunk_fp;

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
        chunk_fp = fopen(chunk_name, "rb");
        if (!chunk_fp)
        {
            fprintf(stderr, "Error: Cannot open chunk file '%s'\n", chunk_name);
            return (3);
        }
        chunk_size = get_file_size(chunk_fp);
        if (chunk_size != meta->chunk_size + extra_bytes)
        {
            fprintf(stderr, "Error: Chunk '%s' has unexpected size (expected %ld, got %ld)\n",
                    chunk_name, meta->chunk_size + extra_bytes, chunk_size);
            fclose(chunk_fp);
            return (5);
        }
        fclose(chunk_fp);
    }
    return (0);
}

/** Merge chunks into a single file */
int merge(int argc, char **argv)
{
    int extra_bytes = 0, i = 0, start_chunk = 0, idx, num_decoys = 0, real_cnt;
    long file_size;
    char chunk_name[256], *file_out, file_name[256], inc_decoy = 0;
    size_t bytes_read;
    pmd meta;
    FILE *pmd_file = fopen(argv[2], "rb"), *chunk, *out_file;

    if (!pmd_file)
    {
        fprintf(stderr, "Error: Cannot open PMD file '%s'\n", argv[2]);
        return (3);
    }

    if (fread(&meta, 1, sizeof(pmd), pmd_file) != sizeof(pmd))
    {
        fprintf(stderr, "Error: Failed to read PMD metadata from '%s'\n", argv[2]);
        fclose(pmd_file);
        return (3);
    }
    fclose(pmd_file);

    if (argc == 3)
        sprintf(file_name, "%s.m",meta.name);
    else if (argc == 6 && !strcmp(argv[3], "-r"))
    {
        start_chunk = atoi(argv[4]);
        strcpy(file_name, argv[5]);
    }
    else if (argc == 5 && !strcmp(argv[3], "-R"))
    {
        strcpy(file_name, argv[4]);
        inc_decoy = 1;
    }
    else
    {
        fprintf(stderr, "Usage: %s -m <pmd_file> [ -r <start_chunk> <output_file> ]\n", argv[0]);
        exit(1);
    }
    for (i = 0; i < 16; i++)
        if ((meta.decoy_mask >> i) & 0b1)
            num_decoys++;

    file_size = (meta.chunk_size * (meta.num_chunks + num_decoys)) + meta.extra_bytes;

    if (check_pmd(&meta, file_size))
    {
        fprintf(stderr, "Error: PMD integrity check failed\n");
        return 1;
    }

    file_out = malloc(file_size);
    if (!file_out)
    {
        fprintf(stderr, "Error: Memory allocation failed (%ld bytes)\n", file_size);
        return (1);
    }

    sprintf(chunk_name, "%s.m", meta.name);
    out_file = fopen(file_name, "wb");
    if (!out_file)
    {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", file_name);
        free(file_out);
        return (3);
    }

    for (real_cnt = 0, idx = start_chunk, i = 0; i < meta.num_chunks + num_decoys; i++, idx++)
    {
        if (((meta.decoy_mask >> i) & 0b1) && !inc_decoy)
            continue;

        if (real_cnt == meta.num_chunks - 1)
            extra_bytes = meta.extra_bytes;
        if (!((meta.decoy_mask >> i) & 0b1))
            real_cnt++;
        if ((meta.decoy_mask >> i) & 0b1)
            extra_bytes = 0;
        if (idx == meta.num_chunks + num_decoys)
        {
            idx = 0;
            extra_bytes = 0;
        }

        sprintf(chunk_name, "%s.%d", meta.name, idx);
        chunk = fopen(chunk_name, "rb");
        if (!chunk)
        {
            fprintf(stderr, "Error: Cannot open chunk '%s'\n", chunk_name);
            fclose(out_file);
            free(file_out);
            return (3);
        }
        bytes_read = fread(file_out, 1, meta.chunk_size + extra_bytes, chunk);
        fwrite(file_out, bytes_read, 1, out_file);
        fclose(chunk);
    }
    fclose(out_file);
    free(file_out);

    return (0);
}

/** Parse a file into chunks */
int pars_file(int argc, char **argv)
{
    int num_chunks, extra_bytes = 0, i, num_decoys = 0, real_cnt;
    long chunk_size, file_size;
    FILE *src_file, *chunk_file;
    char *chunk_buffer, chunk_name[256];
    size_t bytes_read;
    pmd meta;
    unsigned short decoy_mask;

    num_chunks = atoi(argv[3]);
    if (num_chunks <= 0 || num_chunks > 8)
    {
        fprintf(stderr, "Error: Number of chunks must be between 1 and 8 (got %d)\n", num_chunks);
        return (2);
    }

    if (argc == 6 && !strcmp(argv[4], "-R"))
        num_decoys = atoi(argv[5]);

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
        fclose(src_file);
        return (4);
    }

    if (file_size > 1048576L)
    {
        fprintf(stderr, "Error: File size exceeds 1MB limit\n");
        fclose(src_file);
        return (4);
    }

    chunk_size = file_size / num_chunks;

    chunk_buffer = malloc(chunk_size + file_size % num_chunks);
    if (!chunk_buffer)
    {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(src_file);
        return (1);
    }

    decoy_mask = gen_decoy_mask(num_chunks + num_decoys, num_decoys);

    for (real_cnt = 0, i = 0; i < num_chunks + num_decoys; i++)
    {
        if (real_cnt == num_chunks - 1)
            extra_bytes = file_size % num_chunks;
        if ((decoy_mask >> i) & 0b1)
        {
            gen_decoy_chunk(argv, chunk_buffer, chunk_size, chunk_name, i);
            continue;
        }

        bytes_read = fread(chunk_buffer, 1, chunk_size + extra_bytes, src_file);

        sprintf(chunk_name, "%s.%d", argv[2], i);
        chunk_file = fopen(chunk_name, "wb");
        if (!chunk_file)
        {
            fprintf(stderr, "Error: Cannot create chunk '%s'\n", chunk_name);
            free(chunk_buffer);
            fclose(src_file);
            return (3);
        }

        fwrite(chunk_buffer, 1, bytes_read, chunk_file);

        fclose(chunk_file);
        real_cnt++;
    }
    free(chunk_buffer);

    meta.chunk_size = chunk_size;
    meta.extra_bytes = extra_bytes;
    strcpy(meta.name, argv[2]);
    meta.num_chunks = num_chunks;
    meta.decoy_mask = decoy_mask;

    printf("%s\n", meta.name);

    sprintf(chunk_name, "%s.pmd", argv[2]);
    FILE *meta_fp = fopen(chunk_name, "wb");
    if (!meta_fp)
    {
        fprintf(stderr, "Error: Cannot create metadata file '%s'\n", chunk_name);
        fclose(src_file);
        return (3);
    }
    fwrite(&meta, 1, sizeof(pmd), meta_fp);
    fclose(meta_fp);
    fclose(src_file);

    return (0);
}

/** Split a binary file into n chunks (1 <= n <= 8) */
int main(int argc, char **argv)
{
    int result = 0;

    srand(time(NULL));

    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s -p <file> <num_chunks>\n", argv[0]);
        fprintf(stderr, "       %s -m <pmd_file> [start_chunk output_file]\n", argv[0]);
        return (1);
    }
    
    if (!strcmp(argv[1], "-p"))
        result = pars_file(argc, argv);

    else if (!strcmp(argv[1], "-m"))
        result = merge(argc, argv);
    else
    {
        fprintf(stderr, "Error: Unknown option '%s' (use -p or -m)\n", argv[1]);
        result = 1;
    }

    return (result);
}
