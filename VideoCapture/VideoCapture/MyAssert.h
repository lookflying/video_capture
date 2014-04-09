#ifndef MY_ASSERT_H
#define MY_ASSERT_H
#define MY_DEBUG 0
#if MY_DEBUG
#define my_assert(exp) assert(exp)
#else
#define my_assert(exp) if(!(exp)){throw exp;}
#endif
#endif