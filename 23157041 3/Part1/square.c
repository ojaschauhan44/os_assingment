#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include<math.h>
double square(double x) {
    return x * x;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("UNABLE TO EXECUTE\n");
        return 1;
    }

    long long int x = atof(argv[argc - 1]);
    for (int i = 0; i < argc - 1; i++) {
        char *operation = argv[i];
        if (strcmp(operation, "square") == 0 || strcmp(operation, "./square")==0) {
            x = x * x;
        } else if (strcmp(operation, "double") == 0) {
            x = 2 * x;
        } else if (strcmp(operation, "root") == 0) {
            x = round(sqrt(x));
        } else {
            printf("UNABLE TO EXECUTE\n");
            return 1;
        }
    }
    if(x>0){
    printf("%llu\n", x);
    }else{
    printf("UNABLE TO EXECUTE\n");
    }
    return 0;

}
