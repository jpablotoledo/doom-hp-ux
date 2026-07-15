# Investigación: optimizaciones de CPU/compilador para HP-UX PA-RISC

Informe de arquitectura sobre posibles mejoras de rendimiento a nivel de
instrucciones de procesador y flags de compilación, para el HP Visualize
B2000 (PA-RISC 2.0, chip 9000/785). Escrito originalmente como documento
de análisis puro (sin implementar nada todavía); las secciones 1-7 abajo
son ese análisis inicial, sin editar. Ver el resumen de resultados justo
debajo para lo que realmente se probó después.

## Resultados (post-análisis)

De las recomendaciones de este documento, esto es lo que efectivamente
se probó en el hardware real, en una sesión posterior:

- **`+O3 +DA2.0 +DS2.0 +Ofastaccess`**: aplicado y confirmado - mejora
  real de rendimiento jugando en vivo ("va mucho mejor, hay muy pocas
  trabas"). Es el estado actual de `HPFLAGS` en `src/Makefile`.
- **PBO (`+I`/`+P`)**: probado hasta el punto de compilar un binario
  instrumentado; descartado antes de completar el ciclo por
  costo/beneficio - el usuario juzgó que el esfuerzo (instrumentar,
  jugar para generar el perfil, recompilar) no ameritaba la ganancia
  esperada frente a lo ya logrado con `+O3`.
- **`rtprio`: NO USAR sin extremo cuidado.** Se probó con una prioridad
  de tiempo real "moderada" (80) y **causó un colgado total del sistema**
  (pantalla, mouse y teclado sin respuesta; solo el ping ICMP seguía
  funcionando, confirmando inanición de CPU en vez de caída de red o
  kernel panic). Requirió reiniciar físicamente la máquina. La
  suposición de que 80 era "moderado" en un CPU de un solo núcleo con un
  bucle de render sin límite de frecuencia resultó incorrecta. **No se
  volvió a intentar.** Si se retoma esta vía en el futuro, empezar con
  una prioridad mucho más baja y tener acceso físico a la máquina (no
  solo telnet) antes de probar.
- **MAX-2 a mano (ensamblador inline)**: no se llegó a intentar - quedó
  como última opción dado el esfuerzo, y `+O3` solo ya dio una mejora
  suficiente para el objetivo de la sesión.
- Los seis bugs de audio corregidos en la misma sesión (ver
  [`docs/05-investigacion-musica.md`](05-investigacion-musica.md),
  sección 10) son independientes de este documento - son bugs de
  síntesis, no de rendimiento de CPU.

---

*A partir de acá, el documento original sin editar:*

## 1. Punto de partida (confirmado)

- **CPU**: PA-RISC 2.0, designación de modelo `9000/785` (familia B/C-class
  de estaciones HP Visualize; el B2000 específicamente lleva un PA-8500 en
  la configuración más común de esa línea - **la revisión exacta del chip
  (PA-8500 vs PA-8600) y la cantidad de procesadores quedan pendientes de
  confirmar en vivo**, ver sección 6).
- **Compilador**: HP C Compiler A.11.01.00 (`/usr/bin/cc`), sin GCC
  instalado.
- **Flags actuales** (`src/Makefile`, `HPFLAGS`):
  ```
  +O2 +Onolimit +e -Aa -D__BIG_ENDIAN__ -D_HPUX_SOURCE
  ```
  `+O2` es un nivel de optimización intermedio; `-Aa` es modo ANSI C89
  estricto (sin extensiones); no hay flags de arquitectura destino
  (`+DA`/`+DS`), ni optimización interprocedural, ni optimización guiada
  por perfil.

## 2. Extensiones SIMD del procesador: MAX-2

La familia PA-8500/PA-8600 (procesadores típicos de las estaciones
`9000/785`) implementa **MAX-2** (Multimedia Acceleration eXtensions,
versión 2), un conjunto de instrucciones SIMD de enteros que opera sobre
los registros de 64 bits existentes tratándolos como 2×32 bits o 4×16 bits
empaquetados. Permite hacer en una instrucción lo que normalmente toma 2-4
(suma/resta saturada, shift, "average" con redondeo, permutación de
mitades de registro).

**Por qué es relevante acá específicamente:**

- **Mezcla de audio** (`I_SubmitSound()` en `i_sound.c`, la fórmula
  "virtual-analog" que combina SFX y música muestra por muestra) es
  exactamente el tipo de operación que MAX-2 acelera: sumas/multiplicaciones
  de enteros de 16 bits con recorte (clamp) a rango, repetidas miles de
  veces por segundo sobre un buffer.
- **Renderizado de columnas de textura** (`r_draw.c`, el "column drawer" de
  Doom que aplica la tabla de iluminación pixel por pixel) también es un
  bucle estrecho sobre bytes/words que en arquitecturas con SIMD equivalente
  (MMX en x86 de la época) se beneficiaba mucho de vectorización manual.

**Estado real de soporte en el compilador de esta máquina:** HP cc
A.11.01.00 es de **1998** (ver `docs/02-machine-setup.md`, parche
`PHCO_95167` de octubre de 1998). El soporte de **autovectorización**
automática hacia MAX-2 en el compilador de HP llegó más adelante (versiones
posteriores del compilador, ya entrado los 2000, y más completo con
`aCC`/compiladores C++ y flags específicos de vectorización). Es **poco
probable que esta versión concreta del compilador vectorice
automáticamente** código C simple hacia MAX-2 solo con flags de
optimización. Las dos vías reales para aprovechar MAX-2 con este
compilador son:

1. **Ensamblador inline** (`asm()` con la sintaxis de HP cc) para las
   instrucciones MAX-2 concretas (`HADD,SS`, `HSUB,SS`, `HAVG`, `HSHL`,
   `HSHR`, etc.) en los puntos calientes ya identificados (mezcla de
   audio, column drawer). Esto es código no portable y específico de PA-RISC,
   pero el proyecto ya tiene precedente de código específico de plataforma
   (todo `i_sound.c`/`hp_music.c` bajo `#ifdef __hpux`).
2. Verificar si `/opt/langtools` (herramientas de desarrollo HP, visible en
   el PATH de búsqueda de `swlist` de la sección 6) trae una versión más
   nueva del compilador o un ensamblador/optimizador separado con mejor
   soporte - **pendiente de verificar en vivo**.

**Riesgo/esfuerzo:** medio-alto. Requiere escribir o adaptar ensamblador
PA-RISC a mano, sin poder probar en otra máquina (el desarrollo de este
proyecto es cross-compilado/editado en Linux y solo se prueba en el B2000
real). El beneficio esperado es real pero acotado a los dos puntos
calientes mencionados - no es una mejora general del motor.

## 3. Flags de optimización del compilador no utilizados

HP cc tiene varios niveles y sub-flags de optimización más allá de `+O2`
que el proyecto no usa hoy:

| Flag | Qué hace | Aplicable acá |
|---|---|---|
| `+O3` | Optimización agresiva a nivel de módulo (más que `+O2`: mejor scheduling de instrucciones, más inlining) | Sí, candidato directo - probar primero, es el cambio de menor riesgo de toda esta lista |
| `+O4` | Optimización interprocedural (todo el programa, requiere linkear con `+O4` también) | Posible, pero cambia el flujo de build (todos los `.o` deben compilarse y linkearse con `+O4` coherentemente) - más invasivo |
| `+DA2.0` | Genera código específico para el set de instrucciones PA-RISC 2.0 (en vez del código genérico/portable que usa por defecto) | Sí - el binario ya solo corre en esta máquina, no hay razón para no fijar la arquitectura destino |
| `+DS785` (o el modelo específico de CPU) | Instruction scheduling afinado al pipeline del chip exacto | Sí, una vez confirmado el modelo exacto de CPU (sección 6) |
| `+Ofastaccess` | Asume que accesos a datos globales/estáticos no necesitan indirección de 32 bits completa (relevante para el enorme número de globals/statics de este código base, ej. `mixbuffer`, la zona de Doom, etc.) | Sí, candidato - bajo riesgo |
| PBO (`+I` instrumentar, luego `+P` recompilar con el perfil) | Recompila usando datos reales de qué ramas/bucles se ejecutan más, mejorando layout de código y predicción de saltos | Interesante pero de mayor esfuerzo - necesita una sesión de "instrumentar, jugar un rato, recompilar" en la máquina real |
| `+Onolimit` | Ya está en uso - quita el límite de tamaño de función optimizable | - |

**Recomendación de orden de prueba** (de menor a mayor riesgo/esfuerzo):
1. `+O3` solo - cambio de una palabra en el Makefile, riesgo bajo, medir
   FPS/CPU antes/después con el mismo protocolo (`vmstat`/`sar` ya usado en
   `docs/05-investigacion-musica.md`).
2. Agregar `+DA2.0 +DS<modelo>` una vez confirmado el modelo exacto de CPU.
3. `+Ofastaccess`.
4. PBO, si los anteriores no alcanzan - es el que más esfuerzo de sesión
   lleva (requiere jugar con el binario instrumentado para generar el
   perfil).
5. `+O4` interprocedural - el de mayor riesgo de romper el build (hay
   partes del proyecto, como `fastlz`, que se compilan como biblioteca
   separada con sus propios flags; `+O4` interprocedural entre módulos
   compilados con flags distintos puede no ser seguro).

**Precaución conocida:** ya hay un antecedente documentado en este mismo
repo (`docs/04-agregar-sonido.md`, problema P3) de que subir la optimización
de `-O` a `+O2 +Onolimit` cambió el comportamiento de desborde de arrays
del renderer de sprites, exponiendo un bug de límites que no aparecía con
optimización más baja. Subir a `+O3`/`+O4` puede exponer bugs similares
(UB latente que el optimizador anterior no explotaba) - probar con
cuidado, un flag a la vez, y jugar lo suficiente como para pasar por zonas
con muchos enemigos/sprites antes de dar por bueno un cambio.

## 4. Prioridad de proceso / scheduling real-time (a nivel de HP-UX, no de CPU)

Esto no es una instrucción de procesador, pero es un lever de rendimiento
específico de HP-UX que no se probó a fondo en la sesión anterior (solo se
probó `nice` estándar, con resultados limitados - ver
`docs/05-investigacion-musica.md`, sección de timer SIGALRM).

HP-UX (a diferencia de Linux) tiene una **clase de scheduling de tiempo
real de verdad** separada de `nice`: el comando `rtprio` y la familia de
llamadas `rtsched()`/`sched_setscheduler()` con las clases `RTPRIO`/`RTSCHED`.
A diferencia de `nice` (que solo ajusta la prioridad dentro de la clase de
scheduling normal por tiempo compartido, con el kernel todavía decidiendo
cuotas), una prioridad de tiempo real verdadera le da al proceso
**precedencia de scheduling garantizada** sobre cualquier proceso de la
clase normal, incluyendo los demonios de monitoreo de hardware
identificados en `docs/05-investigacion-musica.md` sección 8 (aunque esos
resultaron no ser la causa de las trabas, sí compiten por CPU en la clase
normal).

**Candidato concreto a probar:** lanzar `doom-hpux` con
`rtprio <prioridad> doom-hpux ...` (requiere privilegios de root, que ya se
tienen en esta máquina) en vez de `nice`, y repetir el mismo protocolo de
medición (`vmstat`/`sar` durante una partida con `-warp 1 1`) para comparar
contra la línea base ya documentada. Esto ataca directamente el problema
real ya diagnosticado (el bucle `while(1)` de `D_DoomLoop()` sin límite de
frecuencia, que necesita CPU consistente para no acumular jitter), de una
forma que `nice` no puede garantizar.

**Riesgo:** bajo-medio. Real-time scheduling mal usado puede monopolizar la
CPU y volver el sistema no interactivo si el proceso entra en un bucle
infinito sin ceder CPU - pero dado que ya sabemos que el motor sí cede CPU
naturalmente (llamadas bloqueantes a X11, `select()`, etc.), el riesgo
práctico es bajo. Probar primero con una prioridad de tiempo real moderada,
no la máxima.

## 5. Otras vías de menor prioridad, mencionadas por completitud

- **Modo de 64 bits (`+DD64`)**: PA-RISC 2.0 soporta modo wide (64-bit),
  pero migrar todo el proyecto a 64-bit es un cambio grande (punteros,
  tipos, ABI) con beneficio incierto para este código (Doom no está
  limitado por espacio de direcciones ni por ancho de registro en sus
  cálculos actuales) - no se recomienda perseguir esto.
- **`Onolimit` variantes / `+Oaggressive`**: en compiladores HP más
  modernos existe `+Oaggressive`, pero no está confirmado si A.11.01.00 lo
  soporta - pendiente de verificar (`cc -help` o `man cc` en la máquina).
- **Alineación de datos / `#pragma pack`**: dado que el motor ya maneja
  cuidadosamente el big-endian y structs empaquetadas (`i_sound.c`,
  `w_wad.c`), no se identificó una ganancia clara adicional en esta área.

## 6. Verificaciones pendientes en la máquina real

Esta sesión perdió conectividad con el B2000 antes de poder confirmar estos
puntos en vivo - quedan para la próxima vez que haya acceso:

1. **Modelo exacto de CPU**: `model` ya confirmó `9000/785/B2000`, pero
   falta el stepping exacto del chip (PA-8500 vs PA-8600) - relevante para
   elegir el flag `+DS<modelo>` correcto. Se puede obtener con
   `/usr/sbin/print_manifest` o revisando `/opt/langtools`/`echo | cc -V`.
2. **Cantidad de CPUs**: no confirmado si el B2000 en esta configuración es
   mono o dual-procesador (`getconf NPROCESSORS_ONLN` o `ioscan -fnC processor`).
   Si hay más de un procesador, hay una vía completamente distinta y de
   mayor impacto: mover el timer de audio (`I_HPAudioTick`) o el propio
   render a un proceso/hilo separado con afinidad al segundo CPU - pero el
   motor actual es single-threaded, así que esto sería un cambio grande.
3. **Flags soportados por `cc` A.11.01.00 exactamente**: correr
   `cc -help` o revisar el manual en línea (`man cc`) en la máquina para
   confirmar qué de la lista de la sección 3 aplica a esta versión
   específica del compilador (los flags de HP cc cambiaron bastante entre
   versiones a lo largo de los 90s-2000s).
4. **Disponibilidad de `rtprio`**: confirmar que el comando existe y que
   el kernel de esta instalación tiene el subsistema de tiempo real
   habilitado (`rtprio -l` o similar).

## 7. Recomendación de orden de trabajo (cuando se retome)

1. Confirmar los puntos de la sección 6 (rápido, solo inspección).
2. Probar `+O3` solo - medir con el mismo protocolo `vmstat`/`sar` +
   `-warp 1 1` ya establecido, comparando contra la línea base documentada
   en `docs/05-investigacion-musica.md`.
3. Probar `rtprio` en el binario - es independiente de los cambios de
   compilador y se puede probar en paralelo/primero, ya que no requiere
   recompilar.
4. Si hay margen todavía, sumar `+DA2.0 +DS<modelo>` y `+Ofastaccess`.
5. MAX-2 a mano (sección 2) queda como última opción, dado el esfuerzo y
   que ataca puntos calientes específicos en vez del rendimiento general.
