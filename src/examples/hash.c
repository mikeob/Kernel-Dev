/*
 * Simple MD5 implementation
 * NOTE: This code only works on little-endian machines
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "hash.h"
 
// Constants are the integer part of the sines of integers (in radians) * 2^32
const uint32_t k[64] = {
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
 
// Leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
 
// These vars will contain the hash
uint32_t h0, h1, h2, h3;
 
void md5(uint8_t *initial_msg, size_t initial_len, char *hashed_passwd) 
{
  int new_len;
  uint32_t bits_len;
  int offset;
  uint32_t *w;
  uint32_t a, b, c, d, i, f, g, temp;
 
  // Note: All variables are unsigned 32 bit and wrap modulo 2^32 when calculating
 
  // R specifies the per-round shift amounts
  const uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                        5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                        4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                        6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
 
  // Initialize variables - simple count in nibbles
  h0 = 0x67452301;
  h1 = 0xefcdab89;
  h2 = 0x98badcfe;
  h3 = 0x10325476;

  for(new_len = initial_len*8 + 1; new_len%512!=448; new_len++);
  new_len /= 8;
	
  char new_msg [1000];
	memset (new_msg, 0, 1000);
  memcpy(new_msg, initial_msg, initial_len);
  new_msg[initial_len] = 128; 
 
  bits_len = 8*initial_len; 
  memcpy(new_msg + new_len, &bits_len, 4); 
 
  // Process the message in successive 512-bit chunks
  // for each 512-bit chunk of message
  for(offset=0; offset<new_len; offset += (512/8)) 
	{
  	// Break chunk into sixteen 32-bit words w[j], 0 ≤ j ≤ 15
    w = (uint32_t *) (new_msg + offset);
 
    // Initialize hash value for this chunk
    a = h0;
    b = h1;
    c = h2;
    d = h3;
 
    // Main loop
    for(i = 0; i<64; i++) 
		{
    	if (i < 16) 
			{
      	f = (b & c) | ((~b) & d);
        g = i;      
			} 
			else if (i < 32) 
			{
      	f = (d & b) | ((~d) & c);
        g = (5*i + 1) % 16;
      } 
			else if (i < 48) 
			{
    		f = b ^ c ^ d;
        g = (3*i + 5) % 16;                
			} 
			else 
			{
      	f = c ^ (b | (~d));
        g = (7*i) % 16;
     	}
 
      temp = d;
			d = c;
      c = b;
      b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
      a = temp;
 
		}
 
    // Add this chunk's hash to result so far
    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d; 
	}

	// Put hash value in 33 byte string
	char temp_str [9];
	memset (hashed_passwd, 0, 33);
	memset (temp_str, 0, 9);
	uint8_t *p;

	p = (uint8_t *) &h0;
	snprintf (temp_str, 9, "%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
	strlcat (hashed_passwd, temp_str, 9);
	p = (uint8_t *) &h1;
	snprintf (temp_str, 9, "%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
	strlcat (hashed_passwd, temp_str, 17);
	p = (uint8_t *) &h2;
	snprintf (temp_str, 9, "%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
	strlcat (hashed_passwd, temp_str, 25);
	p = (uint8_t *) &h3;
	snprintf (temp_str, 9, "%2.2x%2.2x%2.2x%2.2x", p[0], p[1], p[2], p[3]);
	strlcat (hashed_passwd, temp_str, 33);
}
