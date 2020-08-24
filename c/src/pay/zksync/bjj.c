

#include "bjj.h"
#include "../../core/client/context_internal.h"
#include "../../core/util/bytes.h"
#include "../../core/util/error.h"
#include "../../third-party/crypto/bignum.h"
#include "../../third-party/crypto/ecdsa.h"
#include "../../third-party/crypto/sha2.h"
#include <stdio.h>
#include <string.h>

bignum256 bignum_from_dec(uint64_t a, uint64_t b, uint64_t c, uint64_t d) {
  bignum256 bn;
  uint8_t   tmp[32];
  long_to_bytes(a, tmp + 24);
  long_to_bytes(b, tmp + 16);
  long_to_bytes(c, tmp + 8);
  long_to_bytes(d, tmp);
  bn_read_be(tmp, &bn);
  return bn;
}

void print_fs_hex(const bignum256* bn) {
  bytes32_t tmp;
  bn_write_be(bn, tmp);
  for (int i = 24, n = 0; i >= 0; i -= 8, n++) {
    uint64_t l = bytes_to_long(((uint8_t*) tmp) + i, 8);
    printf("%i : %p\n", n, (void*) l);
  }
}

void print_bn(const bignum256 bn) {
  printf(" {.val = { ");
  for (int i = 0; i < 9; i++)
    printf("%p%s", (void*) bn.val[i], i == 8 ? "}" : ",");
  printf(" }\n");
}

void print_fs_dec(char* prefix, const bignum256* bn) {
  if (prefix) printf("%s", prefix);
  bytes32_t tmp;
  bn_write_be(bn, tmp);
  for (int i = 24, n = 0; i >= 0; i -= 8, n++) {
    uint64_t l = bytes_to_long(((uint8_t*) tmp) + i, 8);
    printf("   %i : %" PRIu64 "\n", n, l);
  }
}
void print_p(char* prefix, const bjj_point_t* p) {
  if (prefix) printf("%s", prefix);
  print_fs_dec(" x:\n", &p->x);
  print_fs_dec(" y:\n", &p->y);
  print_fs_dec(" t:\n", &p->t);
  print_fs_dec(" z:\n", &p->z);
}

//
const bignum256   MODULUS      = {.val = {0x392126f1, 0x1dca5f70, 0x120ee0a6, 0xfbb6e0e, 0x302b0bab, 0x2822db40, 0x23405370, 0x22739709, 0x60c}}; // p = 21888242871839275222246405745257275088548364400416034343698204186575808495617
const bignum256   EDWARDS_D    = {.val = {0x104f718d, 0x30ee21d9, 0x4346b45, 0x2f8b1ca6, 0x12667550, 0xdd47d64, 0x1c091478, 0x3d9a69e8, 0x305f}}; // d = 12181644023421730124874158521699555681764249180949974110617291017600649128846
const bignum256   EDWARDS_ONE  = {.val = {0xffffffb, 0x3258d071, 0x360cd29a, 0x3f1da567, 0x39462e36, 0x3a8dbde1, 0x3df2f666, 0x1df06681, 0xe0a}}; //
const bjj_point_t SPENDING_KEY = {
    .x = {.val = {0x26a67b3, 0x21ffb531, 0x1bc57f7d, 0x2c693fa, 0x34ac63c5, 0x3c3b7922, 0x1b5a5860, 0x22882d70, 0x691}},
    .y = {.val = {0x1e1ee165, 0x870b9c4, 0x33371b7e, 0x35aa9db, 0x181e0b4e, 0x7a099b9, 0x2c518886, 0x2faad4dd, 0x1880}},
    .t = {.val = {0xb5a0093, 0x244c335e, 0x105bafcc, 0x30aef4e1, 0x222b0e2d, 0x32807470, 0x2e7325d0, 0x3906d025, 0x2310}},
    .z = {.val = {0x3bd7555, 0x130cbb2, 0x2a791eb1, 0x294246f6, 0x12fc2dfc, 0x3d97ad31, 0x2ecb38ee, 0x2f3745d, 0x27b4}}};

static void sha256(bytes_t data, bytes32_t dst) {
  SHA256_CTX ctx;
  sha256_Init(&ctx);
  sha256_Update(&ctx, data.data, data.len);
  sha256_Final(&ctx, dst);
}
void bn_negate(bignum256* b) {
  bignum256 n;
  bn_subtract(&MODULUS, b, &n);
  *b = n;
  bn_mod(b, &MODULUS);
}

void bn_square(bignum256* b) {
  print_fs_dec("sq1=\n", b);

  bn_mod(b, &MODULUS);
  print_fs_dec("sq2=\n", b);
  bn_mod(b, &MODULUS);
  print_fs_dec("sq3=\n", b);
  bn_mod(b, &MODULUS);
  print_fs_dec("sq4=\n", b);
  bn_normalize(b);
  print_fs_dec("sq5=\n", b);

  bignum256 tmp = *b;
  bn_multiply(&tmp, b, &MODULUS);
  print_fs_dec("sq6=\n", b);
  bn_mod(b, &MODULUS);
  print_fs_dec("sq7=\n", b);
}

static void p_double(bjj_point_t* p) {

  // A = X1^2
  bignum256 a = p->x;
  bn_square(&a);

  print_fs_dec("a=\n", &a);

  // B = Y1^2
  bignum256 b = p->y;
  bn_square(&b);

  print_fs_dec("b=\n", &b);

  // C = 2*Z1^2
  bignum256 c = p->z;
  bn_square(&c);
  bn_lshift(&c);
  bn_mod(&c, &MODULUS);

  print_fs_dec("c=\n", &c);

  // D = a*A
  //   = -A
  bignum256 d = a;
  bn_negate(&d);

  print_fs_dec("d=\n", &d);

  // E = (X1+Y1)^2 - A - B
  bignum256 e = p->x;
  bn_addmod(&e, &p->y, &MODULUS);
  print_fs_dec("e=\n", &e);
  bn_square(&e);
  print_fs_dec("e=\n", &e);
  bn_addmod(&e, &d, &MODULUS);
  bn_subtractmod(&e, &b, &e, &MODULUS);
  bn_mod(&e, &MODULUS);
  print_fs_dec("e=\n", &e);

  print_fs_dec("e=\n", &e);

  // G = D+B
  bignum256 g = d;
  bn_addmod(&g, &b, &MODULUS);
  bn_mod(&g, &MODULUS);

  print_fs_dec("g=\n", &g);

  // F = G-C
  bignum256 f = g;
  bn_subtractmod(&f, &c, &f, &MODULUS);
  bn_mod(&f, &MODULUS);

  print_fs_dec("f=\n", &f);

  // H = D-B
  bignum256 h = d;
  bn_subtractmod(&h, &b, &h, &MODULUS);
  bn_mod(&h, &MODULUS);

  print_fs_dec("h=\n", &h);

  // X3 = E*F
  bignum256 x3 = e;
  bn_multiply(&x3, &f, &MODULUS);
  bn_mod(&x3, &MODULUS);

  print_fs_dec("x3=\n", &x3);

  // Y3 = G*H
  bignum256 y3 = g;
  bn_multiply(&y3, &h, &MODULUS);
  bn_mod(&y3, &MODULUS);

  print_fs_dec("y3=\n", &y3);

  // T3 = E*H
  bignum256 t3 = e;
  bn_multiply(&t3, &h, &MODULUS);
  bn_mod(&t3, &MODULUS);

  // T3 = E*H
  bignum256 z3 = f;
  bn_multiply(&z3, &g, &MODULUS);
  bn_mod(&z3, &MODULUS);

  p->x = x3;
  p->y = y3;
  p->t = t3;
  p->z = z3;
}

static void p_add(bjj_point_t* p1, bjj_point_t* p2) {
  // A = x1 * x2
  bignum256 a = p1->x;
  bn_multiply(&a, &p2->x, &MODULUS);
  bn_mod(&a, &MODULUS);

  // B = y1 * y2
  bignum256 b = p1->y;
  bn_multiply(&b, &p2->y, &MODULUS);
  bn_mod(&b, &MODULUS);

  // C = d * t1 * t2
  bignum256 c = EDWARDS_D;
  bn_multiply(&c, &p1->t, &MODULUS);
  bn_mod(&c, &MODULUS);
  bn_multiply(&c, &p2->t, &MODULUS);
  bn_mod(&c, &MODULUS);

  // D = z1 * z2
  bignum256 d = p1->z;
  bn_multiply(&d, &p2->z, &MODULUS);
  bn_mod(&d, &MODULUS);

  // H = B - aA
  //   = B + A
  bignum256 h = b;
  bn_addmod(&h, &a, &MODULUS);

  // E = (x1 + y1) * (x2 + y2) - A - B
  //   = (x1 + y1) * (x2 + y2) - H
  bignum256 e = p1->x;
  bn_addmod(&e, &p1->y, &MODULUS);
  {
    bignum256 tmp = p2->x;
    bn_addmod(&tmp, &p2->y, &MODULUS);
    bn_multiply(&e, &tmp, &MODULUS);
    bn_mod(&e, &MODULUS);
  }
  bn_subtractmod(&e, &h, &e, &MODULUS);
  bn_mod(&e, &MODULUS);

  // F = D - C
  bignum256 f = d;
  bn_subtractmod(&f, &c, &f, &MODULUS);
  bn_mod(&f, &MODULUS);

  // G = D + C
  bignum256 g = d;
  bn_addmod(&g, &c, &MODULUS);

  // x3 = E * F
  p1->x = e;
  bn_multiply(&p1->x, &f, &MODULUS);
  bn_mod(&p1->x, &MODULUS);

  // y3 = G * H
  p1->y = g;
  bn_multiply(&p1->y, &h, &MODULUS);
  bn_mod(&p1->y, &MODULUS);

  // t3 = E * H
  p1->t = e;
  bn_multiply(&p1->t, &h, &MODULUS);
  bn_mod(&p1->t, &MODULUS);

  // t3 = E * H
  p1->z = f;
  bn_multiply(&p1->z, &g, &MODULUS);
  bn_mod(&p1->z, &MODULUS);
}

static void multiply(bignum256* scalar, const bjj_point_t* p, bjj_point_t* dst) {

  memset(dst, 0, sizeof(bjj_point_t));
  dst->y = dst->z = EDWARDS_ONE;
  print_p("G=\n", p);
  print_p("res=\n", dst);

  for (int i = 63; i >= 0; i--) {
    p_double(dst);
    if (bn_testbit(scalar, i)) p_add(dst, (bjj_point_t*) p);
  }
}

in3_ret_t bjj_create_pk_from_seed(bytes_t seed, bytes32_t pk) {
  print_bn(bignum_from_dec(12436184717236109307ULL, 3962172157175319849ULL, 7381016538464732718ULL, 1011752739694698287ULL));

  uint8_t t[32] = {0};
  t[31]         = 0;
  char      buf[200];
  bignum256 h2;
  bn_read_be(t, &h2);

  bn_format(&h2, "REST ", " data", 0, 3, true, buf, 200);
  printf("%s\n", buf);
  bn_negate(&h2);
  bn_write_be(&h2, t);
  bn_format(&h2, "REST ", " data", 0, 3, true, buf, 200);
  printf("%s\n", buf);

  print_bn(bignum_from_dec(1174369594240759125ULL, 18204967083952280043ULL, 10299562934091840557ULL, 2860924641897540787ULL));

  bignum256 bn;
  sha256(seed, pk);
  while (true) {
    sha256(bytes(pk, 32), pk);
    bn_read_be(pk, &bn);
    if (bn_is_less(&bn, &MODULUS)) return IN3_OK;
  }
  return IN3_OK;
}

in3_ret_t bjj_sign_musig(in3_ctx_t* ctx, bytes_t data, bytes32_t pk) {
  UNUSED_VAR(data);

  //  print_bn(bignum_from_dec(12436184717236109307ULL, 3962172157175319849ULL, 7381016538464732718ULL, 1011752739694698287ULL));

  bjj_point_t dst;
  bignum256   bpk;
  bn_read_be(pk, &bpk);
  if (!bn_is_less(&bpk, &MODULUS)) return ctx_set_error(ctx, "invalid pk, must be less then prime", IN3_EINVAL);

  multiply(&bpk, &SPENDING_KEY, &dst);

  // get the public key

  return IN3_OK;
}