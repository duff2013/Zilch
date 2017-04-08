//
//  mem.cpp
//  Teensyduino_3_5
//
//  Created by Colin on 1/31/17.
//  Copyright (c) 2017 Colin Duffy. All rights reserved.
//

#include "mem_manager.h"

uint32_t *mem_manager::pool     = NULL;
uint16_t mem_manager::pool_size = 0;
// --------------------------------------------------------------------------------------------
void mem_manager::init( uint32_t *p, uint16_t len ) {
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
    uint32_t *freelist = pool + 127;
    *freelist = 1;
    //uint32_t *freelistMax = pool + 126;
    //*freelistMax = ptr->length | 1 << 16;
}
// --------------------------------------------------------------------------------------------
mem_block_t *mem_manager::alloc( uint32_t nwords, uint32_t fill_pattern ) {
    
    /*uint32_t *freelistMax   = pool + 126;
    uint16_t free_block_len = *freelistMax & 0xFFFF;
    uint16_t free_block_idx = ( *freelistMax & 0xFFFF0000 ) >> 16;
    
    if ( free_block_len < nwords ) {
        uint32_t list = *( pool + 127 );
        while( list ) {
            int n = __builtin_ctz( list );
            list &= ~( 1 << n );
            mem_block_t *p = ( mem_block_t * )pool + n;
            if ( p->length >= nwords ) {
                free_block_len = p->length;
                free_block_idx = 1 << n;
                *freelistMax = p->length | 1 << (n + 16);
                break;
            }
        }
    }*/
    
    //free_block_len -= nwords;
    //*freelistMax = free_block_len | (free_block_idx << 16);
    
    
    mem_block_t *start = ( mem_block_t * )pool;
    mem_block_t *end = ( mem_block_t * )pool + 32;
    uint32_t *memory = NULL;
    
    uint32_t list = *( pool + 127 );
    while( list ) {
        int n = __builtin_ctz( list );
        list &= ~( 1 << n );
        mem_block_t *p = ( mem_block_t * )pool + n;
        if ( p->length >= nwords ) {
            memory = p->block;
            p->block = p->block + nwords;
            p->length = p->length - nwords;
            if ( p->length == 0 ) {
                p->block = 0;
                uint32_t *freelist = pool + 127;
                *freelist &= ~( 1 << n );
            }
            break;
        }
    }
    
    /*uint32_t *freelistMax   = pool + 126;
    list = *( pool + 127 );
    uint32_t mysize = 0;
    while ( list ) {
        int n = __builtin_ctz( list );
        list &= ~( 1 << n );
        mem_block_t *p = ( mem_block_t * )pool + n;
        if ( p->length > mysize ) {
            *freelistMax = p->length | 1 << ( n + 16 );
        }
    }*/
    
    /*volatile int idx = 0;
    do {
        if ( start->length >= nwords ) {
            memory = start->block;
            start->block = start->block + nwords;
            start->length = start->length - nwords;
            if ( start->length == 0 ) {
                uint32_t *freelist = pool + 127;
                Serial.print("idx: ");
                Serial.print(idx);
                Serial.print(" | alloc1: ");
                Serial.print(*freelist, BIN);
                *freelist &= ~( 1 << idx );
                Serial.print(" | alloc2: ");
                Serial.println(*freelist, BIN);
                delayMicroseconds(1000);
                //Serial.println(idx);
                start->block = 0;
            }
            break;
        }
        idx++;
    } while ( ++start != end );*/
    
    if ( memory == NULL ) return NULL;
    
    start = ( mem_block_t * )pool + 32;
    end = ( mem_block_t * )pool + 64;
    do {
        if ( start->block == 0 ) {
            *memory = (uint32_t)start;
            start->block = memory + 1;
            start->length = nwords;
            uint32_t *bottom = start->block;
            uint32_t *top = start->block + nwords - 1;
            do {
                *bottom = fill_pattern;
            } while ( ++bottom != top );
            return start;
        }
    } while ( ++start != end );
    return NULL;
}
// --------------------------------------------------------------------------------------------
void mem_manager::free( uint32_t * p ) {

    uint32_t list = *( pool + 127 ) ^ 0xFFFFFFFF;
    int n = __builtin_ctz( list );
    uint32_t *freelist = pool + 127;
    *freelist |= ( 1 << n );
    mem_block_t *freed = ( mem_block_t * )pool + n;
    mem_block_t *allocated = ( mem_block_t * ) *( p - 1 );
    freed->block = p - 1;
    freed->length = allocated->length;
    
    /*uint32_t *freelistMax = pool + 126;
    uint16_t free_block_len = *freelistMax & 0xFFFF;
    if ( free_block_len < freed->length ) {
        *freelistMax = freed->length | 1 << ( n + 16 );
    }*/
    
    /*uint32_t *freelistMax   = pool + 126;
    list = *( pool + 127 );
    uint32_t mysize = 0;
    while ( list ) {
        int n = __builtin_ctz( list );
        list &= ~( 1 << n );
        mem_block_t *b = ( mem_block_t * )pool + n;
        b = ( mem_block_t * )pool + n;
        if ( b->length > mysize ) {
            *freelistMax = b->length | 1 << ( n + 16 );
        }
    }*/
    
    allocated->block = 0;
    allocated->length = 0;
    
    
    /*volatile int idx = 0;
    do {
        if ( start->block == 0 ) {
            
            if (start->block != t->block ) {
                Serial.println("OUCH");
                digitalWriteFast(LED_BUILTIN, HIGH);
            }
            volatile uint32_t *freelist = pool + 127;
            *freelist |= ( 1 << idx );
            
            mem_block_t *freed = ( mem_block_t * )*( p - 1 );
            start->block = p - 1;
            start->length = freed->length;
            freed->block = 0;
            freed->length = 0;
            return;
        }
        idx++;
    } while ( ++start != end );*/
}
// --------------------------------------------------------------------------------------------
void mem_manager::combine_free_blocks( void ) {
    mem_block_t *start = ( mem_block_t * )pool;
    mem_block_t *end = ( mem_block_t * )pool + 32;
    volatile int outer_idx = 0;
    do {
        if ( start->block != NULL ) {
            mem_block_t *stmp = ( mem_block_t * )pool;
            volatile int inner_idx = 0;
            do {
                if ( stmp->block + stmp->length == start->block && stmp->block != NULL ) {
                    stmp->length += start->length;
                    start->block = 0;
                    start->length = 0;
                    uint32_t *freelist = pool + 127;
                    *freelist &= ~( 1 << outer_idx );
                    
                    /*uint32_t *freelistMax = pool + 126;
                    uint16_t free_block_len = *freelistMax & 0xFFFF;
                    if ( free_block_len < stmp->length ) {
                        *freelistMax = stmp->length | 1 << ( 16 + outer_idx );
                    }*/
                }
                inner_idx++;
            } while ( ++stmp != end );
        }
        outer_idx++;
    } while ( ++start != end) ;
    
    /*uint32_t *freelistMax   = pool + 126;
    uint32_t list = *( pool + 127 );
    uint32_t mysize = 0;
    while ( list ) {
        int n = __builtin_ctz( list );
        list &= ~( 1 << n );
        mem_block_t *b = ( mem_block_t * )pool + n;
        b = ( mem_block_t * )pool + n;
        if ( b->length > mysize ) {
            *freelistMax = b->length | 1 << ( n + 16 );
        }
    }*/
}

//__attribute__((noinline))
uint16_t mem_manager::poolSize( void ) {
    return pool_size;
}

//__attribute__((noinline))
mem_block_t *mem_manager::allocList( void ) {
    mem_block_t *p = ( mem_block_t * )pool + 32;
    return p;
}
