#include <stdio.h>
#include <string.h>

int main() {
    char dosyaismi[100]; 
    char cumle[100]; 

 
    printf("Dosya ismini girin: ");
    fgets(dosyaismi, 100, stdin);

  
    size_t len = strlen(dosyaismi);
    if (len > 0 && dosyaismi[len - 1] == '\n') {
        dosyaismi[len - 1] = '\0';
    }

    
    printf("Bir cümle girin: ");
    fgets(cumle, 1000, stdin);

    
    FILE *file = fopen(dosyaismi, "w");
   
}
