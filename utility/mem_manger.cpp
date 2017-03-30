//
//  mem.cpp
//  Teensyduino_3_5
//
//  Created by Colin on 1/31/17.
//  Copyright (c) 2017 Colin Duffy. All rights reserved.
//

#include "mem_manger.h"
#include "Arduino.h"

uint32_t *mem_manger::pool     = NULL;
uint16_t mem_manger::pool_size = 0;

void mem_manger::init( uint32_t *p, uint16_t len ) {
    pool_size = len;
    pool = p;
    mem_block_t *ptr = ( mem_block_t * )p;
    ptr->block = pool + 128;
    ptr->length = len - 128;
    uint32_t *start = ( uint32_t * )p + 2;
    uint32_t *end = ( uint32_t * )p + ( len );
    do {
        *start++ = 0;
    } while ( start != end );
}

mem_block_t *mem_manger::alloc( uint32_t nwords, uint32_t fill_pattern ) {
    mem_block_t *start = ( mem_block_t * )pool;
    mem_block_t *end = ( mem_block_t * )pool + 32;
    uint32_t *memory = NULL;
    do {
        if ( start->length >= nwords ) {
            memory = start->block;
            start->block = start->block + nwords;
            start->length = start->length - nwords;
            if ( start->length == 0 ) {
                start->block = 0;
            }
            break;
        }
    } while ( ++start != end );
    if ( memory == NULL ) return NULL;
    start = ( mem_block_t * )pool + 32;
    end = ( mem_block_t * )pool + 64;
    do {
        if ( start->block == 0 ) {
            start->block = memory;
            start->length = nwords;
            uint32_t *bottom = start->block;
            uint32_t *top = start->block + nwords;
            do {
                *bottom = fill_pattern;
            } while (++bottom != top);
            return start;
        }
    } while ( ++start != end );
    return NULL;
}

void mem_manger::free( uint32_t* p ) {
    mem_block_t *start = ( mem_block_t * )pool + 32;
    mem_block_t *end = ( mem_block_t * )pool + 64;
    uint32_t block_size = 0;
    do {
        if ( start->block == p ) {
            block_size = start->length;
            start->length = 0;
            start->block = 0;
            break;
        }
    } while ( ++start != end );
    if ( block_size == 0 ) return;
    start = ( mem_block_t * )pool;
    end = ( mem_block_t * )pool + 32;
    do {
        if (start->block == 0) {
            start->block = p;
            start->length = block_size;
            
            uint32_t *bottom = p;
            uint32_t *top = p + block_size;
            do {
                *bottom = 0;
            } while (++bottom != top);
            return;
        }
    } while (++start != end);
}

void mem_manger::combine_free_blocks( void ) {
    mem_block_t *start = (mem_block_t*)pool;
    mem_block_t *end = (mem_block_t*)pool + 32;
    uint32_t blocks = 0;
    do {
        if (start->block != NULL) {
            mem_block_t *stmp = (mem_block_t*)pool;
            do {
                if (stmp->block + stmp->length == start->block) {
                    stmp->length += start->length;
                    start->block = 0;
                    start->length = 0;
                }
            } while (++stmp != end);
            blocks++;
        }
    } while (++start != end);
}

uint16_t mem_manger::poolSize( void ) {
    return pool_size;
}

mem_block_t *mem_manger::allocList( void ) {
    mem_block_t *p = ( mem_block_t * )pool + 32;
    return p;
}
