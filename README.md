# chunk_s

A C program that splits a binary file into `n` equal-sized chunks (1 ≤ n ≤ 8).

## Compilation

```sh
gcc -Wall -Wextra -o e code.c
```

## Usage

```
./e <file> <n>
```

- `<file>` — path to the binary file to split
- `<n>` — number of chunks (integer, 1–8)

## Output

Creates `n` files named `<file>.0`, `<file>.1`, ..., `<file>.<n-1>`, each containing a portion of the input.

## How it works

1. The input file size is obtained without moving the read cursor.
2. Each chunk gets `file_size / n` bytes.
3. If the file size is not evenly divisible by `n`, the last chunk receives the remaining `file_size % n` extra bytes.
4. Chunks are written sequentially from the start of the file.

## Example

```sh
$ echo "hello-world-123" > test.txt
$ ./e test.txt 3
$ ls test.txt*
test.txt  test.txt.0  test.txt.1  test.txt.2
$ cat test.txt.0; echo; cat test.txt.1; echo; cat test.txt.2
hello-wor
ld-1
23
```
