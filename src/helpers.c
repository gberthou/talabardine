unsigned int __aeabi_uidivmod(unsigned int numerator, unsigned int denominator)
{
    unsigned int quot = 0;
    for(; numerator >= denominator; ++quot, numerator -= denominator);
    __asm__ __volatile__("mov r1, %0" :: "r"(numerator));
    return quot;
}

unsigned int __aeabi_uidiv(unsigned int numerator, unsigned int denominator)
{
    unsigned int quot = 0;
    for(; numerator >= denominator; ++quot, numerator -= denominator);
    return quot;
}

