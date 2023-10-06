#include <cstdlib>
#include <cstdio>
#include <iostream>

int main()
{
    __asm__ volatile( "nop \n\
                        nop \n\
                        nop \n\
                        nop" : : : );
    std::cout << "print\n";                        
    return( EXIT_SUCCESS );
}
