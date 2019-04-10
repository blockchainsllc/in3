
# static lib
add_library( eth_full STATIC
  eth_full.c
  evm.c
  opcodes.c
  big.c
  call.c
  code.c
  env.c
  mem.c
  mini-gmp.c
  precompiled.c

        tommath/bn_fast_mp_invmod.c
        tommath/bn_fast_mp_montgomery_reduce.c
        tommath/bn_mp_abs.c
        tommath/bn_mp_clamp.c
        tommath/bn_mp_clear.c
        tommath/bn_mp_clear_multi.c
        tommath/bn_mp_cmp_d.c
        tommath/bn_mp_copy.c
        tommath/bn_mp_count_bits.c
        tommath/bn_mp_div.c
        tommath/bn_mp_div_2d.c
        tommath/bn_mp_dr_is_modulus.c
        tommath/bn_mp_dr_reduce.c
        tommath/bn_mp_dr_setup.c
        tommath/bn_mp_exch.c
        tommath/bn_mp_export.c
        tommath/bn_mp_exptmod.c
        tommath/bn_mp_exptmod_fast.c
        tommath/bn_mp_grow.c
        tommath/bn_mp_import.c
        tommath/bn_mp_init.c
        tommath/bn_mp_init_copy.c
        tommath/bn_mp_init_size.c
        tommath/bn_mp_invmod.c
        tommath/bn_mp_invmod_slow.c
        tommath/bn_mp_lshd.c
        tommath/bn_mp_mod.c
        tommath/bn_mp_mod_2d.c
        tommath/bn_mp_montgomery_calc_normalization.c
        tommath/bn_mp_montgomery_reduce.c
        tommath/bn_mp_montgomery_setup.c
        tommath/bn_mp_mul.c
        tommath/bn_mp_mul_2d.c
        tommath/bn_mp_mulmod.c
        tommath/bn_mp_reduce_2k.c
        tommath/bn_mp_reduce_2k_setup.c
        tommath/bn_mp_reduce_is_2k.c
        tommath/bn_mp_reduce_is_2k_l.c
        tommath/bn_mp_rshd.c
        tommath/bn_mp_set.c
        tommath/bn_mp_sqr.c
        tommath/bn_mp_zero.c
        tommath/bn_s_mp_exptmod.c
        tommath/tommath.h
        tommath/tommath_class.h
        tommath/tommath_private.h
        tommath/tommath_superclass.h
        )

# add gas-calculation
#ADD_DEFINITIONS(-DEVM_GAS)


# add dependency
target_link_libraries(eth_full core eth_basic)

# props
# props
include(../core/compiler.cmake)

target_include_directories(eth_full PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")