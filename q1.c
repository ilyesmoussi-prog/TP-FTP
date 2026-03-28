#include "csapp.h"

#define INPUT_FILE  "./input1.bin"
#define SLEEP_TIME  1

/* ATTENTION : ne pas modifier cette procedure */
void sender(int fdout)
{
    int fdin;
    int n;
    char buf[MAXLINE];
    int s;
        
    fdin = Open(INPUT_FILE, O_RDONLY, 0);
    
    srand(22508);  // initialisation du generateur de nombres pseudo-aleatoires 
    
    s = (rand()%9) + 1; // tirage pseudo-aleatoire
                        // d'une valeur entiere entre 1 et 9
                        // on peut modifier la sequence de tirages en jouant
                        // sur la valeur passee a srand
    while ( (n = Read(fdin, buf, s)) > 0) {
        Write(fdout, buf, n);
        sleep(SLEEP_TIME);
        s = (rand()%9) + 1;
    }
    Close(fdin);
    Close(fdout);
}


// 
// Tant qu'il y a des donnees disponibles dans le tube,
// lire une valeur (de type unsigned long long => 8 octets)
// et ecrire sur l'ecran la representation textuelle correspondante
// sur une nouvelle ligne (en utilisant printf - le format pour
// le type unsigned long long est %llu)
//
void receiver(int fdin)
{

    rio_t rio;
    unsigned long long val;
    ssize_t n;
 
    rio_readinitb(&rio, fdin);
 
    /* Lire 8 octets à la fois (taille d'un unsigned long long).
       rio_readnb accumule les octets même si le sender envoie
       des morceaux de taille variable (1 à 9 octets). */
    while ((n = rio_readnb(&rio, &val, sizeof(unsigned long long))) > 0) {
        printf("%llu\n", val);
    }
    
    Close(fdin);
}

int main(int argc, char **argv)
{
    int pipe1[2];
    pid_t childpid;

    pipe(pipe1);    /* tube pere vers fils */

    if ( (childpid = Fork()) == 0) {    /* fils */
        
        Close(pipe1[1]);
        /* le fils lit sur la sortie du tube */
        receiver(pipe1[0]);
        exit(0);
    }
    
    /* pere */
    
    Close(pipe1[0]);    
    sender(pipe1[1]);

    Waitpid(childpid, NULL, 0);	        /* attendre fin fils */
    exit(0);
}
