//
//  mem.h
//  Teensyduino_3_5
//
//  Created by Colin on 1/31/17.
//  Copyright (c) 2017 Colin Duffy. All rights reserved.
//

#ifndef __mem_manager__
#define __mem_manager__

#include "Arduino.h"
class mem_manager;

#define AllocateMemoryPool(len) ({                                                      \
    static DMAMEM uint32_t mem_pool[( 256 + ( len - 1 ) - ( ( len - 1 ) % 128 )) + 512];\
    mem_manager::init( mem_pool, ( 256 + ( len - 1 ) - ( ( len - 1 ) % 128 )) + 512 );  \
})

struct mem_block_t {
    uint32_t *block;
    uint32_t length;
};

class mem_manager {
public:
    mem_manager( void ) { }
    static void init( uint32_t *p, uint16_t len );
    mem_block_t *alloc( uint32_t nwords, uint32_t fill_pattern );
    void free( uint32_t* p );
    void combine_free_blocks( void );
    uint16_t poolSize( void );
    mem_block_t *allocList( void );
    static uint32_t *pool;
private:
    
    static uint16_t pool_size;
};
#endif /* defined(__mem_manager__) */
