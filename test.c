extern int add_x_y(int x, int y);

void main(int x, int y) {
    float z;

    z = x / y + 4 - 5.0;

    if (x == 2) {
        x = 4;
    } else {
        return 3;
    }

    if (!false) {
        y = -y / 5;
    }

    while (x <= 5 || y >= 10) {
        add_x_y(50, 100 != 4);
    }

    {
        bool a;
        x = 6;
    }

    return x * y % 2;
}