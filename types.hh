typedef unsigned char u8;
typedef unsigned int  u32;
typedef unsigned long u64;
typedef signed   char s8;
typedef signed   int  s32;
typedef signed   long s64;

static_assert(sizeof(u8)  == 1);
static_assert(sizeof(s8)  == 1);
static_assert(sizeof(u32) == 4);
static_assert(sizeof(s32) == 4);
static_assert(sizeof(u64) == 8);
static_assert(sizeof(s64) == 8);
