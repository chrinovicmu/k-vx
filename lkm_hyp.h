#include "utils.h"


static inline usigned long long notrace __rdmsr1(unsigned int msr)
{
    /* low = eax = 0-31 
     * high = edx = 32-63 */  
    unsigned int high;  
    unsigned int low; 

    __asm__ __volatile__ (
        "1: rdmsr\n"
        "2:\n"
        /*switch to exception table temporarily */ 
        ".pushsection __ex_table. \"a\"\n"

        ".balign 4\n"
        ".long (1b) -. \n"
        ".long (2b) - .\n"
        ".long ex_handler_rdmsr_unsafe - .\n"
        ".popsection\n"

        : "=a" (low), "=d" (high) 
        : "c" (msr)
        : "memory"
    ); 

:wq
    return ((unsigned long long)high << 32) | low; 
}
