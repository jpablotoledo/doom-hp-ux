# Auditoría de commits: qué cambió y por qué (da4f9db6..HEAD)

Informe commit por commit del trabajo de audio/rendimiento en el port de
Doom a HP-UX, rama `adding-music-try2`. Detalla qué tocó cada archivo, para
qué, y si ese cambio sigue siendo necesario en el binario que corre hoy.
Se omiten los commits que solo tocaron documentación, por instrucción
explícita.

**Rango**: `da4f9db61256acd2d9d918ee3ea500e820ed8275..HEAD`
**Commits en el rango**: 11 - **solo documentación (omitidos)**: 3 -
**analizados**: 8

## Resumen

- **5 commits necesarios / base del binario actual.**
- **1 commit completamente reemplazado** por el siguiente (arquitectura de
  música descartada).
- **1 pieza de código correcta pero que nunca se ejecuta** en el binario
  que realmente se distribuye.
- **2 merges sin diff propio** (bookkeeping de Git, no decisiones de
  ingeniería).

| Commit | Mensaje | Veredicto |
|---|---|---|
| `c47ad96` | implementacion usando simpleAudio sobre Alib | Fundacional |
| `a096d1a` | Cambios en i_sound mejora el rendimiento | Necesario |
| `afceff1` | Merge branch 'adding-sound' | Merge vacío |
| `3191a4e` | change FreeDooM to DooM shareware, update docs and improvements | Mixto (parcial necesario / parcial código muerto) |
| `49cd9d2` | Merge pull request #1 from jpablotoledo/improvements | Merge vacío |
| `45c3bee` | many test, im have music, but not the best | Reemplazado por completo |
| `7cb4f13` | music and doc | Fundacional |
| `ea4b68d` | Habemus music | Necesario |

Omitidos por ser solo documentación: `56a8ec5` (actualizacion
source-changes), `91c4c23` (translate documents), `423e720` (update doc).

---

## 1. `c47ad96` - implementación de sonido sobre Alib

*2011-06-30 · Veredicto: **fundacional***

**`src/Makefile`**
- `HPFLAGS` pasa de `-O +e -Aa … -DDOOM_NO_SFX` a `+O2 +Onolimit +e -Aa …`
  - **quita `-DDOOM_NO_SFX`**, la macro que literalmente desactivaba la
  compilación de todo el subsistema de sonido. Este es el cambio que
  enciende audio por primera vez en el port.
- Suma `-I/opt/audio/include` y `-L/opt/audio/lib -lAlib -lAt` para
  enlazar contra la librería de audio de HP-UX (Alib).
- Agrega `simpleAudio.o` a `OBJS` y su regla de compilación.

**`src/i_sound.c`**
- Incluye `simpleAudio.h` bajo `#ifdef __hpux`.
- `I_InitSound()`: agrega la rama HP-UX que abre el audio vía
  `openAudio()`/`openAStream()` (Alib, 16-bit lineal estéreo).
- `I_ShutdownSound()`: agrega el cierre simétrico (`closeAStream`/
  `closeAudio`).
- Extiende la rama de sincronización por timer que antes solo cubría
  `__sun` para que también cubra `__hpux`.

**`src/simpleAudio.h`** (nuevo)
- Declaraciones del wrapper de Alib provisto por el SDK de audio de
  HP-UX (no es código propio, son las firmas de la librería del sistema
  que `doom_build.sh` copia en tiempo de build).

**`doom_build.sh`**
- `SCRIPT_DIR=\`dirname $0\`` → `SCRIPT_DIR=\`cd \`dirname $0\` && pwd\``:
  resuelve la ruta absoluta en vez de relativa (necesario para que
  `at -f script now` no pierda el directorio de trabajo real).
- Agrega el paso que copia `simpleAudio.c` desde la instalación de
  HP-UX, con error explícito si no está.

**Veredicto**: necesario en su totalidad. Sin este commit no hay sonido -
es la base sobre la que corre absolutamente todo lo demás.

---

## 2. `a096d1a` - throttle de escritura y no-bloqueo

*2011-06-30 · Veredicto: **necesario***

**`src/Makefile`**
- `MAXSCREENWIDTH`/`MAXSCREENHEIGHT` de 1024×768 a 1280×800 - cambio de
  resolución de pantalla, sin relación real con el mensaje del commit
  (audio).

**`src/i_sound.c`**
- Agrega `<sys/socket.h>`, `<netinet/in.h>`, `<netinet/tcp.h>` -
  ninguno de los tres se usa en el diff de este commit ni en el resto
  del historial revisado; quedan como *dead includes* desde este punto
  en adelante.
- Implementa el **throttle de escritura por reloj de pared** en
  `I_SubmitSound()`: mide tiempo real transcurrido y solo escribe a
  Aserver si hay menos de 2 buffers en cola - corrige que
  `I_SubmitSound` se llamaba 35×/seg mientras la tasa real de
  reproducción era 11025 Hz, desbordando el buffer de Aserver. Este
  mecanismo sigue vivo hoy, intacto en su forma original.
- Marca `audio_fd` como no bloqueante (`O_NONBLOCK`).

**Veredicto**: necesario - el throttle es la razón por la que el audio
no se degrada progresivamente con el tiempo. Único punto flojo: tres
`#include` que nunca se usaron, sin costo funcional pero sin propósito.

---

## 3. `afceff1` - merge sin contenido propio

*2011-06-30 · Veredicto: **merge vacío***

Fusiona los dos commits anteriores (`c47ad96` + `a096d1a`) de vuelta a la
rama principal. `git show afceff1 -- src/` no produce ningún diff propio
- es un merge limpio, sin resolución de conflictos ni cambios
adicionales.

**Veredicto**: bookkeeping de historial, no un cambio de código
independiente.

---

## 4. `3191a4e` - WAD real y una optimización que nunca corre

*2026-07-06 · Veredicto: **mixto** (build script necesario / i_video.c código muerto)*

**`doom_build.sh`**
- Cambia la búsqueda del WAD de `freedoom1.wad` a `doom1.wad` (shareware
  real que el proyecto distribuye) - corrección operativa directa.
- Reescribe el log con una función `log()` que imprime a consola y
  archivo a la vez; cambia de redirección silenciosa a `tee` (el
  progreso del build es visible en vivo, no solo al terminar).
- Agrega verificación explícita de que `hpdiy8` existe como señal de
  éxito, en vez de confiar únicamente en el código de salida de `make`.

**`shareware/doom1.wad`**
- Agrega el WAD binario real (4.196.020 bytes) - necesario para que el
  juego tenga contenido que cargar.

**`src/i_video.c`**
- Optimiza `Expand4()` (duplicación de píxeles para modos de pantalla
  ampliados): elimina una variable temporal redundante y reemplaza 4
  escrituras manuales repetidas por fila por un único cálculo +
  `memcpy()` para replicar las 3 filas de salida restantes.
- **Este código nunca se ejecuta en el binario que se distribuye.**
  `Expand4()` está condicionado a `#if (LD_PIXEL_DEPTH == 4)`, y esa
  macro solo se define en el target `hp16` del Makefile
  (`SPECIALFLAGS='-DLD_PIXEL_DEPTH=4'`). `doom_build.sh` compila
  exclusivamente con `make hp8`, que nunca define `LD_PIXEL_DEPTH` - el
  preprocesador la trata como 0, y toda esta rama queda fuera del
  binario final.

**Veredicto**: los cambios a `doom_build.sh` y el WAD real son
necesarios y siguen vigentes. La optimización de `i_video.c` es correcta
en aislamiento pero **código muerto** para cómo el proyecto realmente se
compila y despliega.

---

## 5. `49cd9d2` - segundo merge sin contenido propio

*2026-07-06 · Veredicto: **merge vacío***

`git show 49cd9d2 -- src/i_video.c doom_build.sh` no produce diff -
idéntico caso que `afceff1`, trae `3191a4e` de una rama de PR sin
cambios adicionales.

**Veredicto**: bookkeeping de historial únicamente.

---

## 6. `45c3bee` - la arquitectura de música que se abandonó

*2026-07-10 · Veredicto: **reemplazado por completo un commit después***

**`src/Makefile`**
- Agrega `-DMUSSERV="/opt/doom-hpux/musserver-hpux"`, la variable
  `HPMUSFLAGS` y el target `musserver_hp`, que compila
  `musserver_hpux.c` como **ejecutable separado**.

**`src/i_sound.c`** (+269 líneas)
- Arquitectura `fork()` + dos pipes: Doom lanza `musserver-hpux` como
  proceso hijo y le habla por un pipe de comandos, recibiendo PCM de
  vuelta por otro.
- Un ring buffer de 4096 *shorts* para desacoplar la lectura del pipe
  del throttle de escritura a Aserver.
- Instrumentación de diagnóstico: contador de disparos de SFX logueado
  a `logfile`, estadísticas de `musmix`, y un volcado crudo temporal a
  `/tmp/aserver_capture.raw` - todo explícitamente diagnóstico, no
  pensado para quedarse.
- Reescribe el parseo del header MUS de un cast a struct empaquetada a
  acceso por bytes explícito - corrección de portabilidad real (el
  comportamiento de structs empaquetadas de HP cc es específico del
  compilador). **Esta pieza sí sobrevive**, reutilizada tal cual en el
  commit siguiente.

**`src/musserver_hpux.c`, `src/opl2.c`, `src/opl2.h`, `src/opl2test_nuked.c`**
- `musserver_hpux.c` (1117 líneas): el sintetizador OPL2/parser MUS que
  corre como el proceso externo de este commit. No forma parte del
  binario de Doom - es un ejecutable aparte.
- `opl2.c`/`opl2.h` (Nuked-OPL2-Lite, de terceros) + `opl2test_nuked.c`:
  herramienta de referencia/diagnóstico standalone, nunca enlazada a
  `doomengine`.

**Por qué se abandonó**: el proceso separado -aunque su cómputo propio
es barato- introdujo suficiente latencia de scheduling/IPC en esta
máquina de un solo núcleo como para seguir causando cortes audibles bajo
carga, sin importar cuánto se ajustara el tamaño del ring buffer. El
propio mensaje del commit ya lo marca como no definitivo ("not the
best"), y un commit después (`7cb4f13`) se reemplaza enteramente por un
sintetizador en el mismo proceso.

**Qué sobrevivió igual**: el fix de parseo del header MUS por bytes
(portado tal cual). `opl2.c`/`opl2test_nuked.c` siguieron ganándose su
lugar como herramienta de referencia - es exactamente lo que se usó
esta sesión para validar cada uno de los bugs del "chirrido" contra un
emulador cycle-accurate. `musserver_hpux.c` quedó como código muerto en
el árbol: su target de build se eliminó en el commit siguiente y el
archivo no se compila ni se referencia desde ningún lado hoy.

**Veredicto**: ninguna línea de la arquitectura fork/pipe/ring-buffer en
`i_sound.c` sigue en `HEAD` - fue código de exploración necesario para
*aprender* que ese camino no funcionaba, pero no aporta nada al binario
actual salvo el fix de parseo MUS y las herramientas de referencia.

---

## 7. `7cb4f13` - el sintetizador en proceso: la base actual

*2026-07-10 · Veredicto: **fundacional***

**`src/Makefile`**
- Elimina el target `musserver_hp`, el flag `-DMUSSERV` y
  `HPMUSFLAGS` (deshace el cableado de build de `45c3bee`). Agrega
  `hp_music.o` a `OBJS`.

**`src/d_main.c`**
- Reemplaza las llamadas directas `I_UpdateSound()` + `I_SubmitSound()`
  en `D_DoomLoop()` por una única llamada a `I_HPAudioTick()` - punto
  de entrada compartido con el timer `SIGALRM`.

**`src/i_sound.c`** (+461/-220 líneas)
- Elimina por completo el bloque fork/pipe/ring-buffer de `45c3bee`.
- Reescribe la API de música (`I_InitMusic`, `I_PlaySong`,
  `I_RegisterSong`, etc.) para llamar directamente a `HPMusic_*`.
- Agrega `I_HPStartAudioTimer`/`I_HPStopAudioTimer`: timer real
  `SIGALRM`/`setitimer` que alimenta audio a ritmo fijo, independiente
  de cuánto tarde el frame de render actual - con un guard de
  reentrancia `sig_atomic_t` (sin locks, seguro dentro de un manejador
  de señal).

**`src/hp_music.c` / `src/hp_music.h`** (nuevo, 509 líneas)
- Sintetizador OPL2/GENMIDI + parser MUS en el mismo proceso - sin
  `fork()`, sin pipe, sin segundo proceso que el sistema operativo tenga
  que planificar. Esta es la arquitectura que sigue corriendo hoy.

**Veredicto**: el commit que realmente resolvió el problema de música
con trabas - reemplaza la arquitectura fallida por una que elimina la
causa raíz (latencia entre procesos) en vez de seguir ajustando sus
síntomas.

---

## 8. `ea4b68d` - rendimiento de CPU y los seis fixes del "chirrido"

*2026-07-12 · Veredicto: **necesario***

**`src/Makefile`**
- `HPFLAGS`: `+O2` → `+O3 +DA2.0 +DS2.0 +Ofastaccess` - flags de
  optimización de compilador, confirmados con mejora real jugando en el
  hardware ("va mucho mejor, hay muy pocas trabas").

**`doom_build.sh` + `launchers/*.sh`**
- Agrega 3 scripts de lanzamiento adicionales (`-nomusic`, `-nomusic
  -nosound`, `-nosound`) junto al `doom.sh` por defecto.

**`src/i_sound.c`**
- `I_HPAudioTick()` ahora retorna de inmediato si `SoundDisabled` - con
  `-nosound` se salta todo el trabajo de mezcla de audio, no solo se
  silencia la salida.

**`src/i_system.c`**
- `kb_used` (tamaño de la zona interna `Z_Malloc` de Doom): 32MB →
  128MB. Investigado como posible causa de una traba periódica ("disco
  duro trabaja"); esa hipótesis se descartó después en la misma sesión
  (la traba persiste incluso sin música y sin este cambio siendo la
  variable). Se mantiene por ser una mejora segura y de bajo riesgo, no
  porque resolviera lo que se pensaba que iba a resolver.

**`src/hp_music.c`** (+155 líneas) - los seis fixes de esta sesión
- **Mapeo de percusión MUS**: `128 + (note & 0x3F)` con clamp al último
  instrumento → `128 + (note - 35)` con rango válido [35,81], verificado
  contra Chocolate Doom.
- **Fórmula de feedback OPL2**: duplicar la última muestra → sumar las
  dos últimas, verificado contra Nuked-OPL2.
- **Clamp de frecuencia anti-aliasing** en `note_phase_step()`: evita
  que instrumentos de percusión con nota fija superen Nyquist (11025/2
  Hz) y generen "fold-back" - causa real confirmada del chirrido en el
  hi-hat (instrumento 139).
- **`rate_to_inc_release()`**: limita la velocidad máxima de caída de
  decay/release a un mínimo de 48 muestras (~4.3ms), evitando el corte
  de golpe de una envolvente lineal en una sola muestra.
- **Filtro DC-block** de un polo en `opl_mix_sample()`: remueve el
  sesgo DC intencional de varias formas de onda OPL2 (medio seno, seno
  rectificado, cuarto de seno), que de otro modo generaba un salto de
  nivel audible en cada nota nueva.
- **Divisor de mezcla fijo (÷4)** en vez de dividir por el número de
  canales activos en cada instante - la causa dominante real del
  chirrido residual: cada nota nueva diluía de golpe el volumen de las
  que ya sonaban.

**`.claude/settings.json`**
- Configuración de herramientas de Claude Code - no es código del
  juego.

**Veredicto**: necesario en su totalidad para el estado actual
(rendimiento + los seis bugs de audio), con la única salvedad de que
`kb_used` se mantiene por prudencia, no porque haya sido la causa de
nada que se le atribuyera originalmente.

---

## Síntesis

1. **Nada de lo que corre hoy es sobrante.** La cadena c47ad96 →
   a096d1a → 3191a4e (build script) → 7cb4f13 → ea4b68d es una
   progresión lineal donde cada eslabón depende del anterior y sigue
   vigente en `HEAD`.
2. **Un commit completo (`45c3bee`) se escribió, se probó y se
   descartó en el mismo ciclo de trabajo.** Eso no lo hace
   "innecesario" en el sentido de descuido - fue la forma de descubrir
   empíricamente que la arquitectura de proceso separado no era viable
   en este hardware, y dos de sus subproductos (el fix de parseo MUS,
   las herramientas `opl2.c`/`opl2test_nuked.c`) siguen aportando valor
   hoy. Pero `musserver_hpux.c` (1117 líneas) es peso muerto en el
   árbol: no se compila, no se referencia, y podría eliminarse sin
   ningún efecto.
3. **La optimización de `i_video.c` en `3191a4e` nunca se ejecuta** en
   el binario que `doom_build.sh` realmente construye y despliega
   (target `hp8`, sin `LD_PIXEL_DEPTH` definido). Es la única pieza de
   código "productivo" en todo el rango que se puede llamar
   genuinamente innecesaria para el resultado final, más allá de ser
   correcta en sí misma.
4. **Dos merges (`afceff1`, `49cd9d2`) no aportan ni restan código** -
   son artefactos normales de historial de Git, no decisiones de
   ingeniería a evaluar.

En conjunto: el volumen de cambios se explica más por *aprendizaje
iterativo en hardware real sin poder simular localmente* (cada
arquitectura de audio se validó jugando en la máquina física) que por
cambios superfluos. El único candidato real a "no debería haberse
tocado" es `musserver_hpux.c`, que puede borrarse del repo con
confianza si se quiere reducir superficie.
