#include <stdio.h>

int get1(int arr[]);

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    printf("The first value of argumentd is %d\n", arr[1]);
    int first = get1(arr);
    printf("The first value of formal value is %d\n", arr[1]);

    return 0;
}

int get1(int arr[]) {
    arr[1] = -9;
    return arr[0];
}
