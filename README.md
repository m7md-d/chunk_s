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
- `<n>` — number of chunks (integer, 1–8)

### Merge mode

```
./e -m <pmd_file> [start_chunk output_file]
```

- `<pmd_file>` — path to the `.pmd` metadata file created during split
- `start_chunk` — chunk index to begin merging from (default: 0)
- `output_file` — name of the reconstructed file (default: `<name>.m`)

## Output

**Split mode** creates `n` chunk files (`<file>.0`, `<file>.1`, ...) plus a metadata file (`<file>.pmd`).

**Merge mode** reconstructs the original file from the chunk files, outputting to `<name>.m` (or a custom name).

## Limitations

- Maximum file size: **1 MB**
- Chunks must not outnumber the bytes in the file.

## How it works

1. The input file size is obtained without moving the read cursor.
2. Each chunk gets `file_size / n` bytes.
3. If the file size is not evenly divisible by `n`, the last chunk receives the remaining `file_size % n` extra bytes.
4. A `.pmd` metadata file is written alongside the chunks, storing the original filename, chunk size, extra bytes, and chunk count.
5. During merge, the `.pmd` file is read, chunk integrity is verified, and the original file is reconstructed.

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
