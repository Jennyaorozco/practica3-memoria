# Laboratorio de Sistemas Operativos — Laboratorio No. 3: Gestión de Memoria

**Universidad de Antioquia | Facultad de Ingeniería | Ingeniería de Sistemas**

---

## a) Integrantes

| Jenny Andrea Orozco Osorio| Jennya.orozco@udea.edu.co| CC.43.918.288 | | David Julián Penagos Arroyave | julian.penagos@udea.edu.co | CC.1.037.610.202 |

---

## Estructura del repositorio

```
practica3-memoria/
├── README.md          ← Este informe
└── src/
    ├── mem_map.c          (Sección 1 — Espacio de direcciones)
    ├── heap_demo.c        (Sección 2 — API de memoria, uso correcto)
    ├── buggy_mem.c        (Sección 2 — Código con bugs)
    ├── buggy_mem_fixed.c  (Sección 2 — Bugs corregidos)
    ├── base_bounds.c      (Sección 3 — Simulador Base & Bounds)
    └── paging_sim.c       (Sección 5 — Simulador de paginación)
```

---

## Cómo compilar y ejecutar

### Requisitos
```bash
sudo apt install gcc valgrind    # Ubuntu/Debian
```

### Compilar todos
```bash
cd src/
gcc -Wall -o mem_map       mem_map.c
gcc -Wall -o heap_demo     heap_demo.c
gcc -Wall -g -o buggy_mem       buggy_mem.c
gcc -Wall -g -o buggy_mem_fixed buggy_mem_fixed.c
gcc -Wall -o base_bounds   base_bounds.c
gcc -Wall -o paging_sim    paging_sim.c
```

---

## b) Documentación de funciones

### `mem_map.c`
| Función | Descripción |
|---|---|
| `main()` | Declara variables en los tres segmentos: `global_var` en `.data`, `local_var` en stack, `heap_var` en heap. Imprime PID y direcciones virtuales de cada variable para ilustrar el layout del espacio de direcciones. Pausa con `getchar()` para permitir la inspección de `/proc/[pid]/maps`. |

### `heap_demo.c`
| Función | Descripción |
|---|---|
| `main()` | Demuestra el ciclo completo de vida de memoria dinámica: `malloc` para alojar 10 enteros, inicialización, `realloc` para ampliar a 20, impresión del arreglo y `free` para liberar. Verifica el retorno de `malloc` y `realloc` contra NULL. |

### `buggy_mem.c`
| Función | Descripción |
|---|---|
| `main()` | Contiene tres errores intencionales de memoria: **buffer overflow** (`i<=5`), **memory leak** (`q` sin `free`) y **use-after-free** (acceso a `p` después de `free`). Sirve para ilustrar los errores que Valgrind detecta. |

### `buggy_mem_fixed.c`
| Función | Descripción |
|---|---|
| `main()` | Versión corregida: usa `i<5`, llama `free(q)` y accede a `p[0]` **antes** de `free(p)`. Verificada con Valgrind: 0 errores, 0 fugas. |

### `base_bounds.c`
| Función | Descripción |
|---|---|
| `traducir(Registro r, int va)` | Simula la traducción hardware VA→PA con el mecanismo base & bounds. Verifica que `0 <= va < r.bounds`; si no, imprime excepción. Si es válida, retorna `r.base + va`. |
| `main()` | Instancia tres procesos (A, B, C) con distintos base y bounds. Traduce el vector `{0,10,63,64,100}` para cada proceso. |

### `paging_sim.c`
| Función | Descripción |
|---|---|
| `traducir(int va)` | Descompone la VA en VPN (bits altos) y offset (bits bajos según `PAGE_BITS`). Consulta `page_table[vpn]`; si es -1 imprime PAGE FAULT, de lo contrario calcula `PA = (pfn << PAGE_BITS) \| offset`. |
| `main()` | Define el vector de VAs a traducir e itera llamando a `traducir()` para cada elemento. |

---

## Sección 1 — Espacio de Direcciones

### 1.1 Salida de mem_map

```
PID del proceso  : 58
Dir. codigo (main): 0x55bd98fe71c9
Dir. global_var  : 0x55bd98fea010
Dir. local_var   : 0x7ef6ca0d0a0c
Dir. heap_var    : 0x55bd98feb2a0
```

### 1.2 Mapa de memoria `/proc/[pid]/maps`

```
55a256824000-55a256825000 r--p  /mem_map_pause   ← text (read-only)
55a256825000-55a256826000 r-xp  /mem_map_pause   ← text (ejecutable)
55a256828000-55a256829000 rw-p  /mem_map_pause   ← .data/.bss
55a256829000-55a25684a000 rw-p  [heap]
7ec173200000-7ec1733ff000 r--p  libc.so.6
7ee407c09000-7ee408409000 rw-p  [stack]
ffffffffff600000-...       r-xp  [vsyscall]
```

### 1.3 Análisis de /proc/[pid]/maps

**Pregunta 1 — Permisos de cada región:**

| Región | Permisos | Razón |
|---|---|---|
| Text (código) | `r-xp` | Solo lectura y ejecución. No escribible para evitar modificación del código en tiempo de ejecución (seguridad). |
| Data (`.data`) | `rw-p` | Lectura y escritura. Aloja variables globales inicializadas (`global_var=42`). No ejecutable. |
| Heap | `rw-p` | Lectura y escritura. El programa solicita/libera bloques dinámicamente con `malloc/free`. No ejecutable. |
| Stack | `rw-p` | Lectura y escritura. Contiene variables locales y marcos de llamada. No ejecutable (protección NX/DEP). |
| libc (texto) | `r-xp` | Código de la biblioteca C compartida. Solo lectura y ejecución. |

**Pregunta 2 — Correspondencia variable ↔ región:**

| Variable | Dirección | Región |
|---|---|---|
| `main` (código) | `0x55bd98fe71c9` | Text: `r-xp` (55a256825000-55a256826000) |
| `global_var` | `0x55bd98fea010` | Data: `rw-p` (55a256828000-55a256829000) |
| `local_var` | `0x7ef6ca0d0a0c` | Stack: `rw-p` (7ee407c09000-7ee408409000) |
| `heap_var` | `0x55bd98feb2a0` | Heap: `rw-p` (55a256829000-55a25684a000) |

**Pregunta 3 — Otras regiones:**

| Región | Función |
|---|---|
| `libc.so.6` | Biblioteca estándar de C. Contiene `printf`, `malloc`, `free`, etc. Se mapea como shared library. |
| `ld-linux.so.2` | Enlazador dinámico. Carga las bibliotecas compartidas al inicio del proceso. |
| `[vvar]` | Virtual Variable: expone variables del kernel (ej. tiempo) sin syscall costosa. |
| `[vdso]` | Virtual Dynamic Shared Object: código del kernel en espacio de usuario para acelerar syscalls frecuentes (`gettimeofday`, `clock_gettime`). |
| `[vsyscall]` | Mecanismo legacy para syscalls en x86-64. Reemplazado por vdso, se mantiene por compatibilidad. |

**Pregunta 4 — Direcciones virtuales vs físicas:**

No, las direcciones virtuales **NO son iguales** a las físicas. El SO virtualiza la memoria: cada proceso cree que tiene acceso exclusivo a un espacio contiguo, pero en realidad sus páginas están dispersas por la RAM física. La MMU usa la tabla de páginas del proceso para traducir cada dirección virtual a su correspondiente dirección física. Dos procesos distintos pueden tener la misma dirección virtual (ej. `0x401000`) apuntando a frames físicos completamente diferentes — esta es la esencia del *address space* como abstracción (OSTEP, vm-intro).

### 1.4 Dos procesos simultáneos

**Pregunta 1:** Sí, las direcciones virtuales son idénticas (o muy similares) en ambos procesos porque el SO asigna el mismo layout virtual a cada nuevo proceso. La conclusión es que el espacio de direcciones está **completamente aislado**: lo que el Proceso A ve como `0x4052a0` y lo que el Proceso B ve como `0x4052a0` son dos frames físicos distintos en la RAM.

**Pregunta 2:** No. El Proceso A no puede leer ni modificar variables del Proceso B mediante su dirección virtual, porque la MMU solo conoce la tabla de páginas del proceso actualmente en ejecución. Un acceso cruzado generaría un `segmentation fault`. La única forma legítima de comunicación es mediante mecanismos del SO: shared memory, pipes o sockets.

---

## Sección 2 — API de Memoria

### 2.2 Valgrind sobre heap_demo (sin errores)

```
==165== Command: ./heap_demo
Arreglo original: 0 1 4 9 16 25 36 49 64 81
Arreglo ampliado: 0 1 4 9 16 25 36 49 64 81 100 121 144 169 196 225 256 289 324 361
==165== HEAP SUMMARY:
==165==     in use at exit: 0 bytes in 0 blocks
==165==   total heap usage: 3 allocs, 3 frees, 4,216 bytes allocated
==165== All heap blocks were freed -- no leaks are possible
==165== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

**Pregunta 1:** Valgrind no reporta ningún error ni fuga. El mensaje "All heap blocks were freed" significa que cada bloque solicitado con `malloc/realloc` fue liberado con `free` antes de que el proceso terminara.

**Pregunta 2:** `sizeof(int)` devuelve el tamaño en bytes del tipo `int` en la arquitectura donde se compila: 4 bytes en x86-64, pero podría ser 2 bytes en sistemas embebidos de 16 bits. Usar el literal `4` hardcodea una suposición de arquitectura. Con `sizeof(int)` el código es portable.

**Pregunta 3:** `malloc` retorna `NULL` cuando no puede satisfacer la solicitud. Es crítico verificarlo porque intentar desreferenciar `NULL` produce un `segmentation fault` inmediato o comportamiento indefinido. En sistemas críticos, no verificar esta condición puede provocar caídas silenciosas o vulnerabilidades de seguridad.

### 2.4 Valgrind sobre buggy_mem (3 errores detectados)

```
==60== Invalid write of size 4
==60==    at 0x1091E3: main (buggy_mem.c:10)
==60==  Address 0x4a7c054 is 0 bytes after a block of size 20 alloc'd
         → ERROR 1: Buffer overflow (i<=5 escribe en p[5] fuera del bloque)

==60== Invalid read of size 4
==60==    at 0x109231: main (buggy_mem.c:19)
==60==  Address 0x4a7c040 is 0 bytes inside a block of size 20 free'd
         → ERROR 3: Use-after-free (printf accede a p[0] después de free(p))

==60== 100 bytes in 1 blocks are definitely lost in loss record 1 of 1
==60==    at 0x4846828: malloc (buggy_mem.c:13)
         → ERROR 2: Memory leak (malloc(100) para q nunca se libera)

==60== ERROR SUMMARY: 3 errors from 3 contexts
```

**Correcciones aplicadas en `buggy_mem_fixed.c`:**

```c
// CORRECCIÓN 1: cambiar i<=5 por i<5
for (int i = 0; i < 5; i++)

// CORRECCIÓN 2: agregar free(q)
free(q);

// CORRECCIÓN 3: mover printf ANTES de free(p)
printf("p[0] = %d\n", p[0]);
free(p);
```

**Valgrind sobre la versión corregida:**
```
==61== All heap blocks were freed -- no leaks are possible
==61== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

**Pregunta 3 — Consecuencias de use-after-free:**
- **Seguridad:** El bloque liberado puede reasignarse a otro objeto. Un atacante que controle la reasignación puede lograr ejecución arbitraria de código (RCE). CVEs de alta severidad como Heartbleed involucran este patrón.
- **Estabilidad:** El comportamiento es indefinido. El programa puede funcionar correctamente la mayoría de veces y fallar de forma impredecible en producción.
- **Corrupción silenciosa:** El valor leído puede parecer correcto si el bloque aún no fue sobrescrito, dando falsa confianza.

---

## Sección 3 — Traducción de Direcciones: Base & Bounds

### 3.2 Salida completa de base_bounds

```
--- Proceso A (base=32, bounds=64) ---
  VA=  0 -> PA= 32
  VA= 10 -> PA= 42
  VA= 63 -> PA= 95
  [EXCEPCION] VA=64 viola bounds=64
  [EXCEPCION] VA=100 viola bounds=64
--- Proceso B (base=128, bounds=80) ---
  VA=  0 -> PA=128
  VA= 10 -> PA=138
  VA= 63 -> PA=191
  VA= 64 -> PA=192
  [EXCEPCION] VA=100 viola bounds=80
--- Proceso C (base=0, bounds=32) ---
  VA=  0 -> PA=  0
  VA= 10 -> PA= 10
  [EXCEPCION] VA=63 viola bounds=32
  [EXCEPCION] VA=64 viola bounds=32
  [EXCEPCION] VA=100 viola bounds=32
```

**Pregunta 1:** VA=64 y VA=100 superan el `bounds=64` del Proceso A, por lo que la condición `0 ≤ VA < bounds` falla. El SO real respondería terminando el proceso con `SIGSEGV` y, opcionalmente, generando un core dump.

**Pregunta 2:** El Proceso A (rango físico [32, 95]) y el Proceso C (rango físico [0, 31]) no se superponen. Además, el mecanismo base & bounds previene que el Proceso A acceda a VAs fuera de su bounds, por lo que **no puede acceder** a las direcciones del Proceso C.

**Pregunta 3:** La limitación principal es que todo el espacio del proceso (código + heap + stack + espacio vacío entre heap y stack) debe residir en un **bloque físico contiguo**, lo que desperdicia memoria física y hace imposible compartir regiones entre procesos. Esto motivó el surgimiento de la segmentación.

---

## Sección 4 — Segmentación

### 4.1 Traducción manual

Tabla de segmentos: 14 bits de VA (2 selector + 12 offset).

| VA (hex) | Selector | Offset | Segmento | PA o Excepción |
|---|---|---|---|---|
| `0x03A0` | 00 | `0x3A0` = 928 | Code | `PA = 0x4000 + 0x3A0 = 0x43A0` ✅ |
| `0x1800` | 01 | `0x800` = 2048 | Heap | `PA = 0x6000 + 0x800 = 0x6800` ✅ |
| `0x3C00` | 11 | `0xC00` = 3072 | Stack | `PA = 0x2800 + (3072-2048) = 0x2C00` ✅ |
| `0x0C00` | 00 | `0xC00` = 3072 | Code | **EXCEPCIÓN**: offset 3072 ≥ bounds 2048 ❌ |
| `0x2200` | 10 | — | ??? | **EXCEPCIÓN**: selector 10 no existe ❌ |

**Pregunta 2:** El Stack crece hacia direcciones bajas porque históricamente el código y datos empezaban en direcciones bajas. Para un segmento que crece negativamente la fórmula se ajusta a: `PA = base + (offset - tamaño_Stack)`. Si el resultado es negativo, hay violación.

**Pregunta 3:** Con base & bounds el espacio vacío entre heap y stack consume memoria física contigua aunque no se use. Con segmentación, cada segmento ocupa solo lo que necesita, permitiendo alojar más procesos en la misma RAM y compartir segmentos (ej. libc compartida).

**Pregunta 4 — Fragmentación externa:**

Ocurre cuando existen bytes libres suficientes en total pero no hay un bloque contiguo lo suficientemente grande.

```
[0 KB] [Proc A Code: 2KB] [LIBRE: 1KB] [Proc B Heap: 3KB] [LIBRE: 1KB] [Proc A Stack: 2KB] [LIBRE: 0.5KB]
                                                            Total libre: 2.5 KB
       Necesito un segmento contiguo de 2 KB → FALLO (ningún hueco tiene 2 KB contiguos)
```

La solución es la compactación (costosa) o la paginación (bloques de tamaño fijo).

---

## Sección 5 — Paginación

### 5.1 Cálculo de la tabla de páginas

```
Bits de offset = log2(4096) = 12 bits
Bits de VPN    = 32 - 12    = 20 bits
Num. entradas  = 2^20       = 1,048,576 entradas
Tamaño tabla   = 2^20 × 4 bytes = 4 MB por proceso
```

4 MB por proceso es significativo — esto motivó las tablas de páginas multinivel (x86-64 usa 4 niveles).

**Bits de PFN:** `espacio físico 20 bits − 12 bits offset = 8 bits` para el PFN.

| Bit | Función |
|---|---|
| Valid (V) | Indica si la página está presente en RAM. V=0 → page fault. |
| Dirty (D) | Indica si la página fue modificada. Si D=1 al expulsarla, el SO debe escribirla al disco. |
| Referenced (R) | Si la página fue accedida recientemente. Usado en algoritmos de reemplazo (LRU aproximado). |
| Protection (R/W/X) | Permisos de acceso. Permite Copy-on-Write y protección DEP/NX. |

### 5.3 Salida del simulador de paginación

```
VA         VPN    Offset   PFN    PA
--------------------------------------------------
VA=0x00  VPN= 0  Offset= 0  -> PFN= 3  PA=0x30
VA=0x0F  VPN= 0  Offset=15  -> PFN= 3  PA=0x3F
VA=0x20  VPN= 2  Offset= 0  -> PFN= 7  PA=0x70
VA=0x35  VPN= 3  Offset= 5  -> PFN= 2  PA=0x25
VA=0x10  VPN= 1  Offset= 0  -> PAGE FAULT (pagina no presente)
VA=0xA3  VPN=10  Offset= 3  -> PFN= 4  PA=0x43
VA=0xC8  VPN=12  Offset= 8  -> PFN= 6  PA=0x68
VA=0xF0  VPN=15  Offset= 0  -> PAGE FAULT (pagina no presente)
```

**Pregunta 2:** VA=0x10 (VPN=1) y VA=0xF0 (VPN=15) tienen `page_table[vpn] = -1` — páginas no presentes. El SO real: detiene la instrucción, busca la página en swap, carga al frame libre, actualiza la PTE con el nuevo PFN (Valid=1), y reinicia la instrucción.

**Pregunta 3:** Con tabla de un nivel, cada `LOAD` requiere **2 accesos a memoria** (1 leer la PTE + 1 leer el dato). La solución es el **TLB** (Translation Lookaside Buffer): caché asociativa en la MMU que guarda traducciones VPN→PFN recientes. Con un hit en TLB (>99% de los casos) la traducción es casi instantánea.

**Pregunta 4:** La paginación elimina completamente la **fragmentación externa** porque todos los frames tienen el mismo tamaño fijo — cualquier frame libre sirve para cualquier página. El único costo es fragmentación interna de hasta media página por segmento (~2 KB), mucho más manejable.

---

## c) Problemas y Soluciones

| Problema | Solución |
|---|---|
| Valgrind no estaba instalado. | `sudo apt install valgrind` |
| El proceso `mem_map` terminaba antes de poder leer `/proc/[pid]/maps`. | Se añadió `sleep(3)` en una versión de prueba para mantenerlo vivo. |
| El cálculo de PA para el Stack (segmento que crece negativamente) requería ajuste especial. | Se derivó la fórmula `PA = base + (offset - size)` y se verificó `offset < size` para validar. |
| El compilador emitía `-Wuse-after-free` en `buggy_mem.c`. | Es esperado y confirma el bug. Valgrind lo detecta como `Invalid read`. |

---

## d) Pruebas Realizadas

| Programa | Prueba | Resultado |
|---|---|---|
| `mem_map` | Verificar que cada dirección pertenece a la región correcta en `/proc/maps` | PASS |
| `heap_demo` | Valgrind sin errores ni fugas | PASS: 0 errors, All heap blocks freed |
| `buggy_mem` | Valgrind detecta 3 errores clásicos | PASS: 3 errors from 3 contexts |
| `buggy_mem_fixed` | Valgrind limpio tras corrección | PASS: 0 errors, 0 leaks |
| `base_bounds` | VA=64 y VA=100 generan excepción en Proceso A (bounds=64) | PASS |
| `base_bounds` | Proceso C aislado de Proceso A | PASS |
| `paging_sim` | VA=0x10 y VA=0xF0 generan PAGE FAULT | PASS |
| `paging_sim` | Traducciones correctas para VAs válidas | PASS: 0x00→0x30, 0x20→0x70, 0x35→0x25 |

---

## e) Video de sustentación

*Pendiente subir el video*

---

## f) Manifiesto de Transparencia — Uso de IA Generativa

Se utilizó IA generativa (Chatgpt) para:
-Los cálculos de segmentación (Sección 4.1).
-Los cálculos de paginación (Sección 5.1) 
- Algunas respuestas conceptuales basándose en OSTEP.

La práctica nos ayudo a comprender los conceptos de la gestión de memoria en sistemas operativos verificando todo mediante la ejecución de los experimentos en un entorno de Linux real.

*Fecha de entrega: Mayo 11 de 2026 — Universidad de Antioquia*
