extern int add_x_y(int x, int y);

extern int print_int(int X);

void Void(void){
  int result;
  result = 0;
  print_int(result);
  while(result < 10) {
    result = result + 1;
    print_int(result);
  }

  return;
}

int main(int x, int y) {
    float z;

    z = x / y + 4 - 5.0;

    z = 2.0 * x;

    if (x > 2) {
        x = 4;
    } else {
        return 3;
    }
    
    while (x <= 5 || y >= 10) {
        add_x_y(50, 100 != 4);
    }

    return x * y % 2;
}
