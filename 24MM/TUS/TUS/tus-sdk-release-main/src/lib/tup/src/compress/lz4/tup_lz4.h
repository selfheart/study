#ifndef TUP_LZ4_H
#define TUP_LZ4_H

/* system header */
#include <stdint.h>

/* user header */

/* object macro */
#define TUP_LZ4_32 (0x345a4c0005010000UL)
#define TUP_LZ4_64K (0x345a4c0010010000UL)

/* function macro */

/* typedef definition */

/* enum definition */

/* struct/union definition */

/* extern variable */

/* function prototype */
int tup_lz4_decompress(uint8_t *decompress_buffer, uint64_t decompress_size,
  const uint8_t *compress_buffer, uint64_t compress_size,
  const uint8_t *attribution, uint64_t attribution_size);

/* inline function */

#endif /* !TUP_LZ4_H */
