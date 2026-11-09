/* count.c - prints N lines with printf; strace counts how many write() calls that costs.
 * Build: gcc -Wall -Wextra -o count count.c
 * Run:   strace -c -e trace=write ./count 20             (to the terminal)
 *        strace -c -e trace=write ./count 20 > out.txt   (to a file)
 */
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : 20;
    for (int i = 1; i <= n; i++)
        printf("line %d\n", i);       /* printf is a library function, not a system call */
    return 0;
}
