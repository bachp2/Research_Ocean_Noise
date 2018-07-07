#include <stdint.h>
#include <stdio.h>
#define S 100000000
int main() {
    double *arr = (double *)malloc(S * sizeof(double));
    for(size_t i = 0; i < S; ++i){
        arr[i] = i+2;
    }
    printf("%llu\n",arr[S-1]);
    printf("SUCCESS!");
    free(arr);
    return 0;
}