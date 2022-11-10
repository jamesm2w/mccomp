#include <iostream>
#include <cstdio>

// clang++ driver.cpp addition.ll -o add

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

extern "C" DLLEXPORT void print_float(float x) {
  printf("%f\n", x);
}

extern "C" DLLEXPORT void print_int(int x) {
  printf("%d\n", x);
}

extern "C" {
  float pi ();
}

int main() {
    printf("%f\n\n", pi());
}