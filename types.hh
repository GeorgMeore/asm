typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef unsigned long  u64;
typedef signed   char  s8;
typedef signed   short s16;
typedef signed   int   s32;
typedef signed   long  s64;

static_assert(sizeof(u8)  == 1, "wrong u8  size");
static_assert(sizeof(s8)  == 1, "wrong s8  size");
static_assert(sizeof(u16) == 2, "wrong u16 size");
static_assert(sizeof(s16) == 2, "wrong s16 size");
static_assert(sizeof(u32) == 4, "wrong u32 size");
static_assert(sizeof(s32) == 4, "wrong s32 size");
static_assert(sizeof(u64) == 8, "wrong u64 size");
static_assert(sizeof(s64) == 8, "wrong s64 size");
