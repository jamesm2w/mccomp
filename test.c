// MiniC program to compute the value of pi
extern void print_float(float x);

extern void print_int(int x);

float pi() {

  bool flag;
  float PI;
  int i;

  flag = true;
  PI = 3.0;
  i = 2;

  print_float(PI);
  print_int(flag);
  print_int(i);

  while(i < 100) {
    
    if(flag) {
      PI = PI + (4.0 / (i*(i+1)*(i+2)));
      //flag = false;
    }
    else {
      PI = PI - (4.0 / (i*(i+1)*(i+2)));
      //flag = true;
    }
    flag = !flag;
    i = i+2;

    print_float(PI);
    print_int(flag);
   print_int(i);
  }

  return PI;
}