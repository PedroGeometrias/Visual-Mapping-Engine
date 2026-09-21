# Visual Mapping Engine

The browser UI is now the application entry point. The C++ panorama pipeline is compiled to WebAssembly with Emscripten.

## Requirements

- Emscripten SDK (`em++` available in your shell)
- Eigen 3
- Python 3, only for the local static server

STB is already kept in `external/stb`.

## Build

```sh
make
```

If Eigen is somewhere else:

```sh
make EIGEN_INCLUDE=/path/to/eigen3
```

The Emscripten command itself lives in `scripts/build_wasm.sh`, so the Makefile stays small.

## Run the UI

```sh
make serve
```

Then open `http://localhost:8000`.

Do not open `web/index.html` directly with `file://`; browsers normally block WebAssembly loading from there.

## Tests

```sh
make test
```

## Layout

- `src/panorama/pipeline.cpp`: application-level panorama pipeline
- `src/wasm.cpp`: thin C interface exposed to JavaScript
- `web/`: HTML/CSS/JavaScript UI
- `scripts/build_wasm.sh`: Emscripten build command
