# chunk_s

A C program that splits a binary file into `n` equal-sized chunks (1 ≤ n ≤ 8) and can merge them back together.

## Compilation

```sh
gcc -Wall -Wextra -o e code.c
```

## Usage

### Split mode

```
./e -p <file> <n>
```

- `<file>` — path to the binary file to split
- `<n>` — number of real chunks (integer, 1–8)
- `-R <n_fake>` — add `<n_fake>` decoy chunks for plausible deniability

### Merge mode

```
./e -m <pmd_file> [start_chunk output_file]
./e -m <pmd_file> -R <output_file>
```

- `<pmd_file>` — path to the `.pmd` metadata file created during split
- `start_chunk` — chunk index to begin merging from (default: 0)
- `output_file` — name of the reconstructed file (default: `<name>.m`)
- `-R` — include random/decoy chunks in the output (skipped by default)

## Output

**Split mode** creates `n` real chunk files (`<file>.0`, `<file>.1`, ...) plus a metadata file (`<file>.pmd`). If `-R <n_fake>` is given, it also creates `<n_fake>` decoy chunks with pseudo-random content.

**Merge mode** reconstructs the original file from the chunk files, skipping decoy chunks unless `-R` is passed. Outputs to `<name>.m` (or a custom name).

## Limitations

- Maximum file size: **1 MB**
- Chunks must not outnumber the bytes in the file.

## How it works

1. The input file size is obtained without moving the read cursor.
2. Each chunk gets `file_size / n` bytes.
3. If the file size is not evenly divisible by `n`, the last chunk receives the remaining `file_size % n` extra bytes.
4. When `-R <n_fake>` is used, additional decoy chunks with pseudo-random content are inserted at random positions among the real chunks. The metadata file stores a bitmask tracking which chunks are decoys.
5. During merge, decoy chunks are skipped unless `-R` is passed to include them.
6. The `.pmd` file is read, chunk integrity is verified, and the original file is reconstructed.

## Examples

### Split

```sh
$ echo "hello-world-123" > test.txt
$ ./e -p test.txt 3
$ ls test.txt*
test.txt  test.txt.0  test.txt.1  test.txt.2  test.txt.pmd
$ cat test.txt.0; echo; cat test.txt.1; echo; cat test.txt.2
hello-wor
ld-1
23
```

### Merge

```sh
$ ./e -m test.txt.pmd
$ cat test.txt.m
hello-world-123
```

### Merge with custom offset and output

```sh
$ ./e -m test.txt.pmd 1 test_recovered.txt
$ cat test_recovered.txt
ld-123
```

### Split with decoy chunks

```sh
$ ./e -p test.txt 3 -R 2
$ ls test.txt.*
test.txt.0  test.txt.1  test.txt.2  test.txt.3  test.txt.4
$ ls *.pmd
test.txt.pmd
```

### Merge skipping decoys (default)

```sh
$ ./e -m test.txt.pmd
$ cat test.txt.m
hello-world-123
```

### Merge including random chunks

```sh
$ ./e -m test.txt.pmd -R test_recovered.txt
$ cat test_recovered.txt
hello-world-123
```
