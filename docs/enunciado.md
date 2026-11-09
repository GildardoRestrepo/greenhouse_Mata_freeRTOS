---
type: referencia
created: 2026-10-06
---
# Reto: Programación en alto nivel

## Sistema embebido con STM32F407VET6, HAL de ST y FreeRTOS

> **Reto:** diseña y construye un sistema embebido funcional de tema libre que integre hardware real, programación estructurada y tareas concurrentes con FreeRTOS. La solución debe ser demostrable, estable y estar documentada técnicamente.

---
## 1. Propósito

El proyecto final integra conocimientos de microcontroladores, configuración de periféricos, desarrollo de firmware y sistemas operativos de tiempo real. Cada equipo identificará una necesidad o propondrá una experiencia interactiva, diseñará una solución y demostrará su funcionamiento en una tarjeta basada en el **STM32F407VET6**.

El proyecto es de **tema libre**. Se evaluará la idea, pero también la calidad de la implementación, la arquitectura de software, el uso real de FreeRTOS, la integración de periféricos, las pruebas y la capacidad de explicar las decisiones técnicas.

---
## 2. Elección del proyecto

Cada equipo elegirá el proyecto que quiera desarrollar. Puede proponer una idea propia o adaptar alguna de estas opciones:

- Juego de acertijos o _Escape Room_ electrónico.
- Entrenador de memoria, secuencias luminosas o reflejos.
- Invernadero inteligente o estación de monitoreo ambiental.
- Sintetizador musical o generador de sonidos.
- Sistema de seguridad o control de acceso.
- Monitor educativo de variables fisiológicas.
- Robot móvil autónomo.
- Estación de experimentación y control.
- Juego tipo arcade con sensores, puntuación y efectos de sonido.
- Sistema de automatización para un espacio doméstico o de laboratorio.

Estos ejemplos son inspiración, no diseños obligatorios. Se aceptan propuestas diferentes siempre que cumplan **todos los requerimientos obligatorios** de esta guía.

### 2.1 Definición de la propuesta

Antes de implementar el sistema, el equipo describirá:

1. **Nombre y objetivo:** qué hará el sistema y qué problema o experiencia abordará.
2. **Usuario y escenario de uso:** quién lo utilizará y en qué situación.
3. **Entradas:** qué datos o eventos físicos recibirá.
4. **Salidas:** qué respuestas físicas producirá.
5. **Estados del sistema:** modos principales de operación y transiciones entre ellos.
6. **Periféricos:** componentes seleccionados y conexiones con el STM32.
7. **Tareas FreeRTOS:** responsabilidad de cada tarea y mecanismo de comunicación entre tareas.
8. **Demostración:** prueba observable que permitirá comprobar el funcionamiento.

La propuesta debe ser viable con el hardware y el tiempo disponibles. Se puede reducir el alcance de una idea ambiciosa, pero no eliminar los requerimientos obligatorios.

---
## 3. Plataforma de desarrollo obligatoria

Todos los siguientes elementos son obligatorios:

|Elemento|Requerimiento verificable|
|---|---|
|Microcontrolador|La unidad principal de procesamiento será una tarjeta basada en el **STM32F407VET6**.|
|STM32CubeMX|Se utilizará para configurar reloj, pines y periféricos. Se entregará el archivo `.ioc`.|
|STM32CubeIDE|Se utilizará para editar, compilar, depurar y generar el firmware.|
|HAL de ST|Los periféricos del STM32 se inicializarán y operarán mediante la biblioteca HAL oficial de ST. Se permite código propio complementario.|
|FreeRTOS|El firmware ejecutará FreeRTOS en la tarjeta física. Las tareas deben tener responsabilidades diferenciadas.|
|Hardware físico|La demostración principal se ejecutará en el STM32 y con periféricos reales. Una simulación en computador no sustituye este requisito.|

Se permite código de bajo nivel complementario con justificación técnica, pero no se puede sustituir por completo la HAL ni reemplazar FreeRTOS por un bucle principal (_superloop_).

---
## 4. Requerimientos funcionales

El sistema terminado debe cumplir todos los puntos siguientes:

1. Tener un propósito identificable y un comportamiento demostrable físicamente.
2. Implementar **al menos tres estados de operación identificables**. Ejemplos: reposo, operación y alarma; menú, juego y resultado; inicialización, monitoreo y error.
3. Permitir cambios de estado durante la ejecución normal, sin reiniciar el microcontrolador para cada transición.
4. Recibir información de **al menos dos tipos distintos de entradas físicas**.
5. Producir **al menos dos tipos distintos de salidas físicas**.
6. Informar al usuario sobre el estado o resultado de la operación mediante una salida física.
7. Gestionar errores previsibles relevantes para la propuesta, como lecturas fuera de rango, pulsaciones inválidas, tiempos de espera agotados o desconexión de un sensor. Si no aplica ningún ejemplo, se debe justificar en la documentación.
8. Mantener una operación estable durante una demostración continua de **al menos cinco minutos**.

**Qué se entiende por tipos distintos:** dos botones equivalentes no cuentan por sí solos como dos tipos de entrada. Sí pueden contar, por ejemplo, un teclado matricial y un sensor, o un potenciómetro leído por ADC y un sensor digital I²C. Dos LED que cumplen la misma función tampoco cuentan por sí solos como dos tipos de salida; una matriz LED y un speaker, o una pantalla y un actuador, sí son tipos diferentes.

---
## 5. Requerimientos de hardware y periféricos

El proyecto incorporará como mínimo:

- **Dos tipos de entradas físicas** que proporcionen datos o eventos del entorno o del usuario.
- **Dos tipos de salidas físicas** que produzcan respuestas observables diferentes.
- **Al menos un periférico o interfaz externa** que utilice efectivamente uno de estos recursos del microcontrolador: ADC, I²C, SPI, UART, temporizador/PWM o GPIO para conectar un dispositivo externo.
### 5.1 Documentación eléctrica

Para cada módulo o periférico, documentar:

- Nombre, referencia o modelo del componente.
- Función dentro del proyecto.
- Pin o interfaz del STM32 al que se conecta.
- Alimentación y niveles lógicos requeridos.
- Protocolo o recurso utilizado (ADC, I²C, SPI, UART, GPIO o PWM, por ejemplo).
- Configuración relevante y procedimiento de prueba.

No se deben conectar motores, altavoces u otras cargas que excedan la capacidad de los pines directamente al microcontrolador. Se debe utilizar una etapa de manejo apropiada. El montaje debe respetar los límites eléctricos de todos los componentes.

---
## 6. Requerimientos obligatorios de FreeRTOS

FreeRTOS debe cumplir una función real en la arquitectura. Crear tareas sin responsabilidades diferenciadas o dejar toda la lógica en `main()` no satisface este requisito.

### 6.1 Tareas y prioridades

El firmware debe incluir:

1. **Al menos cuatro tareas de aplicación**, creadas mediante la API de FreeRTOS o la capa CMSIS-RTOS v2 configurada para FreeRTOS.
2. Una responsabilidad claramente diferenciada para cada tarea. Ejemplos: adquisición de sensores, procesamiento, interfaz, lógica de aplicación, control de actuadores o alarmas.
3. **Al menos tres niveles de prioridad distintos** entre las tareas de aplicación.
4. Una justificación para la prioridad asignada a cada tarea, relacionándola con su urgencia y las consecuencias de atenderla tarde.
5. Para cada tarea periódica, un periodo o frecuencia definido y documentado. Para cada tarea activada por eventos, identificar el evento que la despierta.
6. Un tamaño de pila (_stack_) explícito para cada tarea y una explicación breve de cómo se verificó que fuera suficiente.

Las tareas internas del sistema creadas por FreeRTOS no cuentan dentro del mínimo de cuatro tareas de aplicación.

### 6.2 Temporización y uso de CPU

- Para suspender una tarea durante un intervalo, utilizar mecanismos de FreeRTOS, como `vTaskDelay()` o `vTaskDelayUntil()`, según el comportamiento requerido.
- **No utilizar `HAL_Delay()` dentro de las tareas como mecanismo normal de temporización.** La inicialización anterior al arranque del scheduler es una excepción posible; después de arrancar la aplicación no debe utilizarse para sustituir la planificación.
- Evitar bucles de espera activa que consuman CPU sin necesidad. Cuando una tarea espere datos o un evento, debe bloquearse mediante el mecanismo apropiado.
- Acotar el trabajo de cada tarea para no impedir indefinidamente que otras tareas atiendan sus eventos.
- Explicar cómo el sistema distribuye el tiempo de CPU entre tareas listas y qué ocurre cuando una tarea de mayor prioridad pasa al estado listo.

### 6.3 Comunicación y sincronización

El proyecto utilizará **al menos un mecanismo de comunicación o sincronización de FreeRTOS con una finalidad demostrable**:

- Cola (_queue_).
- Notificación de tarea (_task notification_).
- Semáforo binario o contador.
- Mutex.

La documentación debe especificar qué tareas participan, qué dato o evento se intercambia y por qué se seleccionó ese mecanismo. Su uso debe ser funcional, no solamente declarativo.

Si varias tareas acceden a un recurso compartido —por ejemplo, una pantalla, un bus de comunicación o una estructura de datos—, se debe identificar el posible conflicto y proteger el acceso cuando sea necesario. Si se utiliza un mutex, explicar qué recurso protege. No utilizar retardos arbitrarios como sustituto de la sincronización.

### 6.4 Preempción y evidencia de planificación

La implementación debe permitir demostrar un caso real de planificación entre tareas de distinta prioridad. Como mínimo, mostrar:

1. Una tarea de prioridad baja ejecutando una función no instantánea o esperando trabajo.
2. Una tarea de prioridad superior que pasa al estado listo mientras la de menor prioridad podría seguir ejecutándose.
3. La ejecución de la tarea de mayor prioridad antes de que continúe el trabajo de menor prioridad, de acuerdo con la configuración del scheduler.
4. Una evidencia observable de la secuencia, como mensajes por UART, cambios de indicadores, eventos registrados o una medición temporal.

La explicación debe distinguir entre una tarea **lista**, una **ejecutándose** y una **bloqueada**. Una tarea bloqueada por un retardo o por la espera de un evento no consume CPU mientras permanece bloqueada.

---
## 7. Entregables

### 7.1 Repositorio de código

Entregar un repositorio organizado que incluya:

- Proyecto completo de STM32CubeIDE.
- Archivo de configuración `.ioc` de STM32CubeMX.
- Código fuente propio y archivos necesarios para compilar.
- Configuración de FreeRTOS utilizada.
- Instrucciones para abrir, compilar y ejecutar el proyecto.
- Historial de cambios del equipo durante el desarrollo.

El proyecto debe abrirse en STM32CubeIDE sin depender de rutas absolutas ni archivos privados de un computador específico. Si se requiere una biblioteca externa, incluirla apropiadamente o indicar su origen y versión.

### 7.2 Documento técnico

Debe incluir:

1. Nombre, objetivo y descripción del sistema.
2. Requerimientos funcionales y alcance.
3. Diagrama de bloques de hardware.
4. Tabla de conexiones entre el STM32 y cada periférico.
5. Diagrama de estados de la aplicación.
6. Tabla de tareas FreeRTOS con nombre, función, prioridad, tamaño de pila, periodo o evento de activación y recursos compartidos.
7. Diagrama o explicación de la comunicación y sincronización entre tareas.
8. Justificación del uso de ADC, GPIO, I²C, SPI, UART, PWM u otros recursos seleccionados.
9. Procedimiento y resultados de las pruebas.
10. Limitaciones conocidas y posibles mejoras.

### 7.3 Demostración funcional

La demostración debe realizarse en hardware real y mostrar el funcionamiento normal, los cambios entre estados, la interacción con entradas y salidas, la ejecución de las tareas y la evidencia de planificación descrita en la sección 6.4.

El equipo debe explicar brevemente las decisiones de arquitectura y al menos un problema técnico encontrado y resuelto durante el desarrollo.

---
## 8. Rúbrica de evaluación

La calificación se asignará sobre **100 puntos**. En cada criterio se valorará la evidencia observable en el hardware, el código fuente, la documentación y las pruebas.

|Criterio|Puntos|Qué se evalúa|
|---|---|---|
|1. Objetivo, solución y funcionalidad|10|Claridad del objetivo; solución funcional; estados de operación; pertinencia de entradas y salidas; calidad de la experiencia o utilidad lograda.|
|2. Integración de hardware y periféricos|15|Uso correcto del STM32F407VET6, HAL y recursos configurados; conexiones seguras; funcionamiento de entradas y salidas; configuración documentada de periféricos.|
|3. Arquitectura de tareas FreeRTOS|20|Cuatro o más tareas reales; responsabilidades diferenciadas; tres o más niveles de prioridad; periodos/eventos definidos; configuración y justificación de pilas y prioridades.|
|4. Planificación, preempción y sincronización|15|Demostración de planificación entre prioridades; uso correcto y justificado de una cola, notificación, semáforo o mutex; gestión de recursos compartidos; explicación de estados de tarea.|
|5. Temporización, memoria y estabilidad|10|Uso adecuado de retardos de FreeRTOS; ausencia de esperas activas innecesarias; estabilidad durante cinco minutos; tratamiento de errores relevantes; dimensionamiento razonable de pilas.|
|6. Calidad y organización del código|10|Modularidad, legibilidad, nombres, manejo de errores, separación entre aplicación y periféricos, y mantenimiento del código generado por CubeMX.|
|7. Pruebas y evidencias|10|Pruebas reproducibles; resultados esperados y observados; evidencia de entradas, salidas, estados, comunicación entre tareas, preempción y estabilidad.|
|8. Documentación y demostración|10|Repositorio completo y reproducible; diagramas y tablas requeridas; explicación de decisiones técnicas; demostración física comprensible.|
|**Total**|**100**||

---
## 9. Lista de verificación antes de entregar

- [ ] Elegimos y describimos un proyecto de tema libre.
- [ ] El firmware se ejecuta en una tarjeta STM32F407VET6.
- [ ] El proyecto utiliza STM32CubeMX, STM32CubeIDE, HAL de ST y FreeRTOS.
- [ ] Tenemos al menos tres estados de operación.
- [ ] Integramos al menos dos tipos distintos de entradas y dos tipos distintos de salidas físicas.
- [ ] Implementamos al menos cuatro tareas de aplicación y tres niveles de prioridad.
- [ ] Documentamos periodo o evento, prioridad y tamaño de pila de cada tarea.
- [ ] Utilizamos funcionalmente al menos una cola, notificación, semáforo o mutex.
- [ ] Demostramos la planificación entre tareas de distinta prioridad.
- [ ] El sistema funciona durante al menos cinco minutos y probamos errores relevantes.
- [ ] Entregamos el repositorio, el archivo `.ioc`, el documento técnico y las evidencias de prueba.

---
## Enlaces
[[reto_4_os|Reto 4 OS]]
