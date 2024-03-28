#ifndef SYSTEM_CONFIG_H_
#define SYSTEM_CONFIG_H_

#include <stdint.h>
#include <cstdlib>
#include <stdlib.h>

typedef uint16_t TableID;
typedef uint64_t IndexID;

typedef char* TupleData;

typedef uint64_t IndexKey;

#define ENBALE_BPTREE_BUFFER true

#endif