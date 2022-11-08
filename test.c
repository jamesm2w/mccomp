extern int add_x_y(int x, int y);
extern int print_int(int X);
extern float print_float(float X);

float glob;

int test (int a, int b) {
    int z;

    print_float(glob);
    if (z == 0) {
      return b;
    }

    print_int(a);
    print_int(b);
    a = 2 * b;
    print_int(a);
    print_int(b);
    z = a + b * 3.0;
    glob = glob + 1;
    print_int(a);
    print_int(b);
    
    print_float(glob);
    return a;
}

int test2 (int a) {
  return print_int(a);
}