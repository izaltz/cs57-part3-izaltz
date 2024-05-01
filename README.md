Readme for miniC optimizer
Isabel Zaltz (github: izaltz)

April, 30, 2024 - CS57 24S


The optimizer consists of:
- part3.c
    - contains implementations of the following local optimizations:
        - constant subexpression elimination
        - dead code elimination
        - constant folding
    - contains implementation of the global optimization constant propogation
- Makefile
- Core.h (LLVM functions)

The optimizer accepts a LLVM file and produces an optimized version of that file
