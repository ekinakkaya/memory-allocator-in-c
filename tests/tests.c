#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int test1() {
    int pass = -1;
    /* allocate some strings */

    char *expect1 = "hi!";
    char *expect2 = "test1";

    char *t1, *t2;
    t1 = malloc(strlen(expect1) + 1);
    strcpy(t1, expect1);

    t2 = malloc(strlen(expect2) + 1);
    strcpy(t2, expect2);

    if (strcmp(t1, expect1) == 0 && strcmp(t2, expect2) == 0 && (t1 != NULL) && (t2 != NULL))
        pass = 0;

    free(t1); free(t2);

    if (t1 == NULL && t2 == NULL)
        pass = 0;

    if (pass == 0)
        return pass;

    return -1;
}

int test2() {
    int pass = -1;

    return pass;
}

int main() {
    if (test1() == 0) printf("test1: pass\n"); else printf("test1: fail\n");
    if (test2() == 0) printf("test2: pass\n"); else printf("test1: fail\n");


    return 0;
}
