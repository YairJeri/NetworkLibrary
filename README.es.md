# Motor de Red (C++17)

Motor de red asíncrono y multihilo desarrollado en C++17 moderno utilizando Asio (standalone o Boost.Asio). Diseñado para arquitecturas cliente-servidor de baja latencia con un modelo de transporte híbrido: **TCP** para sincronización confiable, autenticación y ciclo de vida de sesiones; y **UDP** para intercambio rápido de datagramas en tiempo real.

*Read this in [English](README.md).*

## Características

- **Arquitectura de Transporte Híbrido:**
  - **TCP:** Manejo del ciclo de vida de conexión, framing con payloads de tamaño dinámico y gestión de desconexiones.
  - **UDP:** Datagramas de bajo overhead con handshake inicial para asociar el endpoint UDP a la sesión TCP activa del cliente.
- **Serialización Binaria:**
  - Operadores de flujo (`<<`, `>>`) para tipos POD estándar y cadenas con prefijo de longitud (`std::string`).
  - Validación de límites de memoria frente a *buffer underflows* y paquetes malformados.
- **Estructuras de Datos Thread-Safe:**
  - `ThreadSafeQueue`: Cola concurrente FIFO sincronizada con `std::condition_variable`.
  - `ThreadSafeMap`: Contenedor asociativo protegido por mutex para indexar sesiones de clientes por ID.
- **Entrada/Salida Asíncrona:** Basado en worker threads con `asio::io_context` para no bloquear los hilos principales.
- **Sin Dependencias Pesadas:** Detección automática y compatibilidad tanto con `asio` standalone como con `boost::asio`.

## Flujo de Conexión

```
 [ Cliente ]                                      [ Servidor ]
     |                                                |
     | ----- 1. Conexión TCP (Handshake / Auth) ----> | (Asigna ID de sesión)
     | <---- 2. Aceptación TCP + ID de Cliente ------- |
     |                                                |
     | ----- 3. Handshake UDP (Paquete Auth) -------> | (Asocia Endpoint UDP a Sesión)
     |                                                |
     | <=============== Tráfico Híbrido =============> |
     |      TCP: Eventos confiables / Comandos        |
     |      UDP: Datagramas en tiempo real (Inputs)   |
```

## Compilación y Pruebas

### Requisitos
- CMake 3.14+
- Compilador con soporte C++17 (`g++` 9+, `clang++` 10+)
- `asio` (standalone) o `boost`

### Compilar

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Genera cuatro ejecutables en `build/`:
- `server`: Servidor de ejemplo escuchando en TCP (puerto 60000) y UDP (puerto 60001).
- `client`: Cliente interactivo con entrada de terminal sin bloqueo.
- `unit_tests`: Suite de pruebas automatizadas para colas, mapas y serialización de paquetes.
- `benchmark`: Herramienta de medición de latencia y throughput.

### Ejecutar Tests Unitarios

```bash
ctest --test-dir build --output-on-failure
# O directamente:
./build/unit_tests
```

### Ejecutar Benchmarks

```bash
./build/benchmark
```

**Métricas representativas (AMD Ryzen / Linux):**
- **Throughput de Serialización:** ~29M paquetes/seg
- **Latencia de Deserialización:**
  - `p50`: 20 ns
  - `p95`: 22 ns
  - `p99`: 28 ns
- **Throughput en ThreadSafeQueue:** ~4.2M operaciones/seg (4 hilos productores, 1 consumidor)

## Estructura del Proyecto

```
.
├── CMakeLists.txt
├── README.md                   # Documentación en inglés
├── README.es.md                # Documentación en español
├── include/
│   ├── network.h               # Header principal / Umbrella header
│   ├── network_core.h          # Detección de Asio y macros de plataforma
│   ├── network_connection.h    # Wrapper asíncrono de conexión TCP
│   ├── network_session.h       # Entidad de sesión (conexión TCP + endpoint UDP)
│   ├── network_package.h       # Paquetes binarios y operadores de serialización
│   ├── network_server.h        # Interfaz de servidor (acceptor TCP + listener UDP)
│   ├── network_client.h        # Interfaz de cliente
│   ├── network_tsqueue.h       # Cola concurrente thread-safe
│   └── network_tsmap.h         # Mapa concurrente thread-safe
├── src/
│   ├── server.cpp              # Aplicación servidor de ejemplo
│   └── client.cpp              # Aplicación cliente de ejemplo
├── tests/
│   └── test_network.cpp        # Suite de tests unitarios
└── benchmarks/
    └── benchmark.cpp           # Herramienta de benchmarking
```
