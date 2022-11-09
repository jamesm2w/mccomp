extern int add_x_y(int x, int y);
extern int print_int(int X);
extern float print_float(float X);

float glob;

int test (int a, int b) {

  print_int(a);

  {
    a = a + 1;
    b = b - 1;
  }

  print_int(b);

  return 10;
}