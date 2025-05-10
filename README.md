# Filmvert
Film inversion software


Requirements: LibRaw, OpenMP (libomp on Brew for macOS)
Compile LibRaw from scratch:
```
autoreconf --intsall
```
Building for macOS using the XCode Toolchain:
```
./configure --enable-openmp OPENMP_CFLAGS="-Xpreprocessor -fopenmp" OPENMP_CXXFLAGS="-Xpreprocessor -fopenmp" OPENMP_LIBS="-L/opt/homebrew/opt/libomp/lib -lomp"
```
Copy built libraw_r.a (thread support) to LibRaw/lib
Copy "libraw" header to LibRaw

