#include "csapp.h"

int main(int argc, char **argv)
{
    int pipe1[2];
    pid_t childpid;
    char buf[MAXLINE];
    int n;

    pipe(pipe1);	                    /* tube pere vers fils */

    if ( (childpid = Fork()) == 0) {    /* fils */
        Close(pipe1[1]);

        /* le fils lit sur la sortie du tube */
        /* la boucle se termine lorsque l'on detecte
           que le pere a ferme l'entree du tube */
        while ( (n = Read(pipe1[0], buf, MAXLINE)) > 0) {
            /* ... et ecrit sur l'ecran */ 
            Write(1, buf, n);
        }   
        Close(pipe1[0]);
        exit(0);
    }
    
    /* pere */
    
    Close(pipe1[0]);
    
    while ( (n = Read(0, buf, MAXLINE)) > 0) { /* le pere lit sur le terminal */
        /* ... et ecrit dans l'entree du tube */
        Write(pipe1[1], buf, n);
    }         
    Close(pipe1[1]); /* fermeture de l'entree du tube */

    Waitpid(childpid, NULL, 0);	        /* attendre fin du fils */
    exit(0);
}
