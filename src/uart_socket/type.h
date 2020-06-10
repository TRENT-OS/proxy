
#pragma once

#include <stddef.h>

/*
    This is taken from Linux kernel code.

    It can be used to:

    a) handle a "type gap" when using a framework requiring callbacks
       Suppose module Y is using framework X. X is requiring Y to provide a callback but X is
       passing to this callback only a context pointer of type X. If Y has to use more than
       one frameworks like this the callback has difficulties finding its Y context.

    b) mimic a derivation hierarchy with base classes and interfaces in ANSI C.
*/

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define PARAM(x, y) (y)

        