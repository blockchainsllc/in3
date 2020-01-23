#ifndef MH_GENERIC
#define MH_GENERIC

#define mh_assert_static(isTrue) void mh_assert_static(char x[1 - (!(isTrue))])

#define UNUSED(x) (void)(x)

#endif /* end of include guard */
