
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
  precompiled.c
#  tommath/bn_error.c
#  tommath/bn_fast_mp_invmod.c
  tommath/bn_fast_mp_montgomery_reduce.c
  tommath/bn_fast_s_mp_mul_digs.c
  tommath/bn_fast_s_mp_mul_high_digs.c
  tommath/bn_fast_s_mp_sqr.c
  tommath/bn_fast_s_mp_sqr.c
  tommath/bn_mp_2expt.c
  tommath/bn_mp_abs.c
  tommath/bn_mp_add.c
#  tommath/bn_mp_add_d.c
#  tommath/bn_mp_addmod.c
#  tommath/bn_mp_and.c
  tommath/bn_mp_balance_mul.c
  tommath/bn_mp_clamp.c
  tommath/bn_mp_clear.c
  tommath/bn_mp_clear_multi.c
  tommath/bn_mp_cmp.c
  tommath/bn_mp_cmp_d.c
  tommath/bn_mp_cmp_mag.c
#  tommath/bn_mp_cnt_lsb.c
#  tommath/bn_mp_complement.c
  tommath/bn_mp_copy.c
  tommath/bn_mp_count_bits.c
#  tommath/bn_mp_decr.c
  tommath/bn_mp_div.c
  tommath/bn_mp_div_2.c
  tommath/bn_mp_div_2d.c
#  tommath/bn_mp_div_3.c
#  tommath/bn_mp_div_d.c
#  tommath/bn_mp_dr_is_modulus.c
#  tommath/bn_mp_dr_reduce.c
#  tommath/bn_mp_dr_setup.c
  tommath/bn_mp_exch.c
  tommath/bn_mp_export.c
#  tommath/bn_mp_expt_d.c
#  tommath/bn_mp_expt_d_ex.c
  tommath/bn_mp_exptmod.c
#  tommath/bn_mp_exptmod_fast.c
#  tommath/bn_mp_exteuclid.c
#  tommath/bn_mp_fread.c
#  tommath/bn_mp_fwrite.c
#  tommath/bn_mp_gcd.c
#  tommath/bn_mp_get_bit.c
#  tommath/bn_mp_get_double.c
#  tommath/bn_mp_get_int.c
#  tommath/bn_mp_get_long.c
#  tommath/bn_mp_get_long_long.c
  tommath/bn_mp_grow.c
#  tommath/bn_mp_ilogb.c
  tommath/bn_mp_import.c
#  tommath/bn_mp_incr.c
  tommath/bn_mp_init.c
  tommath/bn_mp_init_copy.c
  tommath/bn_mp_init_multi.c
#  tommath/bn_mp_init_set.c
#  tommath/bn_mp_init_set_int.c
  tommath/bn_mp_init_size.c
  tommath/bn_mp_invmod.c
  tommath/bn_mp_invmod_slow.c
#  tommath/bn_mp_is_square.c
#  tommath/bn_mp_iseven.c
#  tommath/bn_mp_isodd.c
#  tommath/bn_mp_jacobi.c
#  tommath/bn_mp_karatsuba_mul.c
#  tommath/bn_mp_karatsuba_sqr.c
#  tommath/bn_mp_kronecker.c
#  tommath/bn_mp_lcm.c
  tommath/bn_mp_lshd.c
  tommath/bn_mp_mod.c
  tommath/bn_mp_mod_2d.c
#  tommath/bn_mp_mod_d.c
  tommath/bn_mp_montgomery_calc_normalization.c
#  tommath/bn_mp_montgomery_reduce.c
  tommath/bn_mp_montgomery_setup.c
  tommath/bn_mp_mul.c
  tommath/bn_mp_mul_2.c
  tommath/bn_mp_mul_2d.c
  tommath/bn_mp_mul_d.c
  tommath/bn_mp_mulmod.c
#  tommath/bn_mp_n_root.c
#  tommath/bn_mp_n_root_ex.c
#  tommath/bn_mp_neg.c
#  tommath/bn_mp_or.c
#  tommath/bn_mp_prime_fermat.c
#  tommath/bn_mp_prime_frobenius_underwood.c
#  tommath/bn_mp_prime_is_divisible.c
#  tommath/bn_mp_prime_is_prime.c
#  tommath/bn_mp_prime_miller_rabin.c
#  tommath/bn_mp_prime_next_prime.c
#  tommath/bn_mp_prime_rabin_miller_trials.c
#  tommath/bn_mp_prime_random_ex.c
#  tommath/bn_mp_prime_strong_lucas_selfridge.c
#  tommath/bn_mp_radix_size.c
#  tommath/bn_mp_radix_smap.c
#  tommath/bn_mp_rand.c
#  tommath/bn_mp_read_radix.c
#  tommath/bn_mp_read_signed_bin.c
#  tommath/bn_mp_read_unsigned_bin.c
  tommath/bn_mp_reduce.c
#  tommath/bn_mp_reduce_2k.c
  tommath/bn_mp_reduce_2k_l.c
#  tommath/bn_mp_reduce_2k_setup.c
  tommath/bn_mp_reduce_2k_setup_l.c
#  tommath/bn_mp_reduce_is_2k.c
  tommath/bn_mp_reduce_is_2k_l.c
  tommath/bn_mp_reduce_setup.c
  tommath/bn_mp_rshd.c
  tommath/bn_mp_set.c
#  tommath/bn_mp_set_double.c
#  tommath/bn_mp_set_int.c
#  tommath/bn_mp_set_long.c
#  tommath/bn_mp_set_long_long.c
#  tommath/bn_mp_shrink.c
#  tommath/bn_mp_signed_bin_size.c
  tommath/bn_mp_sqr.c
#  tommath/bn_mp_sqrmod.c
#  tommath/bn_mp_sqrt.c
#  tommath/bn_mp_sqrtmod_prime.c
  tommath/bn_mp_sub.c
#  tommath/bn_mp_sub_d.c
#  tommath/bn_mp_submod.c
#  tommath/bn_mp_tc_and.c
#  tommath/bn_mp_tc_div_2d.c
#  tommath/bn_mp_tc_or.c
#  tommath/bn_mp_tc_xor.c
#  tommath/bn_mp_to_signed_bin.c
#  tommath/bn_mp_to_signed_bin_n.c
#  tommath/bn_mp_to_unsigned_bin.c
#  tommath/bn_mp_to_unsigned_bin_n.c
#  tommath/bn_mp_toom_mul.c
#  tommath/bn_mp_toom_sqr.c
#  tommath/bn_mp_toradix.c
#  tommath/bn_mp_toradix_n.c
#  tommath/bn_mp_unsigned_bin_size.c
#  tommath/bn_mp_xor.c
  tommath/bn_mp_zero.c
#  tommath/bn_prime_tab.c
#  tommath/bn_reverse.c
  tommath/bn_s_mp_add.c
  tommath/bn_s_mp_exptmod.c
  tommath/bn_s_mp_mul_digs.c
#  tommath/bn_s_mp_mul_high_digs.c
#  tommath/bn_s_mp_sqr.c
  tommath/bn_s_mp_sub.c
  tommath/bncore.c
  tommath/tommath.h
  tommath/tommath_class.h
  tommath/tommath_private.h
  tommath/tommath_superclass.h)

# add gas-calculation
#ADD_DEFINITIONS(-DEVM_GAS)


# add dependency
target_link_libraries(eth_full core eth_basic)

# props
# props
include(../core/compiler.cmake)

target_include_directories(eth_full PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")