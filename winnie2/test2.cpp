// r-lyeh
// based on code by winnie_

#include "winnie.hpp"
#include <string>
#include <stdio.h>

int main () {
    void * a1 = wmalloc( 256 );
    void * a2 = wmalloc( 256 );
    void * a3 = wmalloc( 256 );

    wfree( a2, 256 );

    void * a4 = wmalloc( 256 );
    void * a5 = wmalloc( 256 );

    printf ("%p \n", a1);
    printf ("%p \n", a2);
    printf ("%p \n", a3);
    printf ("%p \n", a4);
    printf ("%p \n", a5);

    std::string *s1 = wnew<std::string>();
    std::string *s2 = wnew<std::string>();
    std::string *s3 = wnew<std::string>();
    wdelete(s3);
    wdelete(s3);
    std::string *s4 = wnew<std::string>();
    std::string *s5 = wnew<std::string>();

    printf ("%p \n", s1);
    printf ("%p \n", s2);
    printf ("%p \n", s3);
    printf ("%p \n", s4);
    printf ("%p \n", s5);

    system("pause");
}
