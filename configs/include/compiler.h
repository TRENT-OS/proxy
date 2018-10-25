#ifndef INLINE
#  if __GNUC__ && !__GNUC_STDC_INLINE__
#    if __OPTIMIZE__
#      define INLINE extern inline
#    else
#      define INLINE static inline
#    endif
# else
#  define INLINE inline
# endif
#endif
