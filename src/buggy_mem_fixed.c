// buggy_mem_fixed.c -- Versión corregida
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    /* CORRECCION 1: buffer overflow -> usar < en lugar de <= */
    int *p = malloc(5 * sizeof(int));
    if (p == NULL) { perror("malloc p"); return 1; }
    for (int i = 0; i < 5; i++)   /* < correcto */
        p[i] = i;

    /* CORRECCION 2: memory leak -> llamar free(q) */
    char *q = malloc(100);
    if (q == NULL) { perror("malloc q"); free(p); return 1; }
    strcpy(q, "hola mundo");
    printf("%s\n", q);
    free(q);   /* liberar q correctamente */

    /* CORRECCION 3: use-after-free -> no acceder después de free */
    printf("p[0] = %d\n", p[0]);  /* acceso ANTES de free */
    free(p);

    return 0;
}
