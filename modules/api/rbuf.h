/**
 ****************************************************************************************
 *
 * @file rbuf.h
 *
 * @brief Definitons of Ring Buffer module(Predefine Size, Byte-byte Copy).
 *
 ****************************************************************************************
 */

#ifndef _RBUF_H_
#define _RBUF_H_

#include <stdint.h>
#include <stdbool.h>


/*
 * DEFINES
 ****************************************************************************************
 */

/// Buffer space size *User should redefine it via #undef RBUF_SIZE.
#ifndef RBUF_SIZE
#define RBUF_SIZE           0x100
#endif

/// Length type of buffer
#ifndef rblen_t
#define rblen_t             uint16_t
#endif

/// Macro of utils function
#define IS_PWR_2(n)         ((n != 0) && ((n & (n - 1)) == 0))
#define RBUF_MIN(a, b)      ((a) < (b) ? (a) : (b))
#define RBUF_MAX(a, b)      ((a) > (b) ? (a) : (b))

#if IS_PWR_2(RBUF_SIZE)
// Increase 'p'(position) with 's'(step).
#define RBUF_INC(p, s)      ((p + s) % RBUF_SIZE)
// Get number of bytes currently in buffer.
#define RBUF_LEN(h, t)      ((RBUF_SIZE + h - t) % RBUF_SIZE)
// Get available size in buffer for write, 1 less than it actually is.
#define RBUF_AVAIL(h, t)    ((RBUF_SIZE + t - h - 1) % RBUF_SIZE)
#else
// Increase 'p'(position) with 's'(step).
#define RBUF_INC(p, s)      ((p == (RBUF_SIZE - s)) ? (s - 1) : (p + s))
// Get number of bytes currently in buffer.
#define RBUF_LEN(h, t)      ((h >= t) ? (h - t) : (RBUF_SIZE + h - t))
// Get available size in buffer for write, 1 less than it actually is.
#define RBUF_AVAIL(h, t)    ((t > h) ? (t - h - 1) : (RBUF_SIZE + t - h - 1))
#endif

/// Struct type of Ring Buffer
typedef struct ringbuffer
{
    volatile rblen_t head;
    volatile rblen_t tail;
    uint8_t  data[RBUF_SIZE];
} rbuf_t;


/*
 * INLINE FUNCTION
 ****************************************************************************************
 */

/**
 ****************************************************************************************
 * @brief Init/Reset the entire BUFF contents.
 *
 * @param[in] rb  The ringbuff to be emptied.
 * @return None.
 ****************************************************************************************
 */
static __forceinline void rbuf_init(rbuf_t *rb)
{
    rb->head = rb->tail = 0;
}

/**
 ****************************************************************************************
 * @brief Returns the size of the BUFF in bytes.
 *
 * @param[in] rb  The ringbuff to be used.
 * @return The size of the BUFF.
 ****************************************************************************************
 */
static __forceinline rblen_t rbuf_size(rbuf_t *rb)
{
    return RBUF_SIZE; // Direct return
}

/**
 ****************************************************************************************
 * @brief Returns the number of used bytes in the BUFF.
 *
 * @param[in] rb  The ringbuff to be used.
 * @return The number of used bytes.
 ****************************************************************************************
 */
static __forceinline rblen_t rbuf_len(rbuf_t *rb)
{
    rblen_t head = rb->head;
    rblen_t tail = rb->tail;
    
    return RBUF_LEN(head, tail);
}

/**
 ****************************************************************************************
 * @brief Returns the number of bytes available in the BUFF.
 *
 * @param[in] rb  The ringbuff to be used.
 * @return The number of bytes available.
 ****************************************************************************************
 */
static __forceinline rblen_t rbuf_avail(rbuf_t *rb)
{
    rblen_t head = rb->head;
    rblen_t tail = rb->tail;
    
    return RBUF_AVAIL(head, tail);
}

/**
 ****************************************************************************************
 * @brief Is the BUFF empty?
 *
 * @param[in] rb  The ringbuff to be used.
 * @return Yes or No.
 ****************************************************************************************
 */
static __forceinline bool rbuf_is_empty(rbuf_t *rb)
{
    return (rb->head == rb->tail);
}

/**
 ****************************************************************************************
 * @brief Is the BUFF full?
 *
 * @param[in] rb  The ringbuff to be used.
 * @return Yes or No.
 ****************************************************************************************
 */
static __forceinline bool rbuf_is_full(rbuf_t *rb)
{
    rblen_t head = rb->head;
    rblen_t tail = rb->tail;
    
    return (RBUF_AVAIL(head, tail) == 0);
}

/**
 ****************************************************************************************
 * @brief Force Put one byte into the BUFF.
 *
 * @param[in] rb  The ringbuff to be used.
 * @param[in] ch  The byte to be added.
 *
 * @return None.
 ****************************************************************************************
 */
static __forceinline void rbuf_putc(rbuf_t *rb, uint8_t ch)
{
    rblen_t head = rb->head;
    
    rb->data[head] = ch;
    rb->head = RBUF_INC(head, 1);
}

/**
 ****************************************************************************************
 * @brief Force Puts some data into the BUFF.
 *
 * @param[in] rb   The ringbuff to be used.
 * @param[in] in   The data to be added.
 * @param[in] len  The length of the data to be added.
 *
 * @return None.
 ****************************************************************************************
 */
static __forceinline void rbuf_puts(rbuf_t *rb, const uint8_t *in, rblen_t len)
{
    rblen_t head = rb->head;
    
    // copy byte to byte
    for (rblen_t i = 0; i < len; i++)
    {
        rb->data[head] = in[i];
        head = RBUF_INC(head, 1);
    }
    
    rb->head = head;
}

/**
 ****************************************************************************************
 * @brief Get one byte from the BUFF.
 *
 * @param[in]  rb  The ringbuff to be used.
 * @param[out] ch  Where the byte be copied.
 *
 * @return Byte copied or not.
 ****************************************************************************************
 */
static __forceinline bool rbuf_getc(rbuf_t *rb, uint8_t *ch)
{
    rblen_t head = rb->head;
    rblen_t tail = rb->tail;
    
    if (head != tail)
    {
        *ch = rb->data[tail];
        rb->tail = RBUF_INC(tail, 1);
        return 1;
    }
    
    return 0; // empty
}

/**
 ****************************************************************************************
 * @brief Gets some data from the BUFF.
 *
 * @param[in]  rb   The ringbuff to be used.
 * @param[out] out  Where the data must be copied.
 * @param[in]  max  The max size of the destination buffer.
 *
 * @return The number of copied bytes.
 ****************************************************************************************
 */
static __forceinline rblen_t rbuf_gets(rbuf_t *rb, uint8_t *out, rblen_t max)
{
    volatile rblen_t head = rb->head;
    volatile rblen_t tail = rb->tail;
    volatile rblen_t dlen = RBUF_LEN(head, tail);
    
    if (dlen > max) 
    {
        dlen = max;
    }
    
    if (dlen > 0) 
    {
        // copy byte to byte
        for (rblen_t i = 0; i < dlen; i++)
        {
            out[i] = rb->data[tail];
            tail = RBUF_INC(tail, 1);
        }
        
        rb->tail = tail;
    }
    
    return dlen;
}


#endif // _RBUF_H_
