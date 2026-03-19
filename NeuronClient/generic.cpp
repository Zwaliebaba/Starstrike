#include "pch.h"
#include "generic.h"


void IpToString(struct in_addr in, char *newip)
{
        snprintf ( newip, 16, "%u.%u.%u.%u", in.S_un.S_un_b.s_b1,
                                        in.S_un.S_un_b.s_b2,
                                        in.S_un.S_un_b.s_b3,
                                        in.S_un.S_un_b.s_b4 );
}
