/*  In this file, I will define 17.14 fixed-point arithmetic
 *  operations.
 *  
 *  Integers:
 *    Priority
 *    Nice
 *    Ready threads
 *
 *  Floating Point:
 *    recent_cpu
 *    load_avg
 *
 * */

/* I will refer to floating numbers as x or y 
 * and integers as n */

#define FLOAT 16384

typedef struct
{
  int32_t num;
} fixed_point_t;

/* Returns a fixed_point representation of an integer */
static inline fixed_point_t
int_to_fixed(int n)
{
  return (fixed_point_t){n * FLOAT};
}

/* Rounds to nearest */
static inline int
fixed_to_int(fixed_point_t x)
{
  if (((int)x.num) >= 0)
  {
    return ((x.num + FLOAT / 2) / FLOAT);
  }
  else 
  {
    return ((x.num - FLOAT/2) / FLOAT);
  }
}

/* Rounds down */
static inline int
fixed_to_int_down(fixed_point_t x)
{
  return (x.num / FLOAT);
}

static inline fixed_point_t
fixed_add(fixed_point_t x, fixed_point_t y)
{
  return (fixed_point_t) {x.num + y.num};
}

/* Adds two fixed_point numbers */
static inline fixed_point_t
fixed_sub(fixed_point_t x, fixed_point_t y)
{
  return (fixed_point_t) {x.num - y.num};
}

/* Adds an integer to a fixed_point number */
static inline fixed_point_t
fixed_add_int(fixed_point_t x, int n)
{
  return (fixed_point_t) {x.num + n*FLOAT};
}

/* Subtracts an integer from a fixed_point number */
static inline fixed_point_t
fixed_sub_int(fixed_point_t x, int n)
{
  return (fixed_point_t) {x.num - n*FLOAT};
}

/* Multiplies two fixed point numbers */
static inline fixed_point_t
fixed_mult(fixed_point_t x, fixed_point_t y)
{
  return (fixed_point_t) {((int64_t) x.num) * y.num / FLOAT};
}

/* Multiplies a fixed_point by an integer */
static inline fixed_point_t
fixed_mult_int(fixed_point_t x, int n)
{
  return (fixed_point_t) {x.num * n};
}

/* Divides two fixed_point numbers */
static inline fixed_point_t
fixed_div(fixed_point_t x, fixed_point_t y)
{
  return (fixed_point_t) {((int64_t) x.num) * FLOAT / y.num};
}

/* Divides a fixed_point number by an integer */
static inline fixed_point_t
fixed_div_int(fixed_point_t x, int n)
{
  return (fixed_point_t) {x.num/n};
}







