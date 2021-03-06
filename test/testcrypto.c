/*
 * by Christopher Adam Telfer
 *
 * Copyright 2003-2012 -- See accompanying license
 *
 */
#include <stdio.h>
#include <string.h>

#include <cat/crypto.h>
#include <cat/time.h>

#define NITER 100000
#define DLEN 256

uchar rand_data[DLEN]; 
byte_t sipkey[16];

void init_data()
{
  const char seed[] = "Hi world!";
  struct arc4ctx arc4;
  arc4_init(&arc4, seed, sizeof(seed));
  arc4_gen(&arc4, rand_data, sizeof(rand_data));
  arc4_gen(&arc4, sipkey, sizeof(sipkey));
}


void test_arc4()
{
  struct arc4ctx arc4;
  int i, j, match;
  ulong len;

  unsigned char test_vectors[3][3][32] = {
    {"Key", "Plaintext", "\xBB\xF3\x16\xE8\xD9\x40\xAF\x0A\xD3"},
    {"Wiki", "pedia", "\x10\x21\xBF\x04\x20"},
    {"Secret", "Attack at dawn", 
      "\x45\xA0\x1F\x64\x5F\xC3\x5B\x38\x35\x52\x54\x4B\x9B\xF5"}
  };
  unsigned char out[32];

  match = 1;
  for ( i = 0; i < 3; ++i ) {
    arc4_init(&arc4, test_vectors[i][0], strlen(test_vectors[i][0]));
    len = strlen(test_vectors[i][1]);
    arc4_encrypt(&arc4, test_vectors[i][1], out, len);
    printf("RC4|  Plaintext:'%s', Key:'%s', Ciphertext:'",
           test_vectors[i][0], test_vectors[i][1]);
    for ( j = 0; j < len; ++j ) {
      printf("%02x", out[j]);
      match = match && (out[j] == test_vectors[i][2][j]);
    }
    printf("' (%s)\n", match ? "match" : "no match");
  }
}


void perf_arc4()
{
  int i;
  cat_time_t start, diff;
  const char key[] = "hey there RC4!";
  struct arc4ctx arc4;

  arc4_init(&arc4, key, sizeof(key));
  start = tm_uget();
  for ( i = 0; i < NITER; ++i )
    arc4_encrypt(&arc4, rand_data, rand_data, sizeof(rand_data));
  diff = tm_sub(tm_uget(), start);
  printf("%lf seconds to run %d ARC4 encryptions on %d-byte data\n",
         tm_2dbl(diff), NITER, DLEN);
  printf("%lf ns per byte\n", (tm_2dbl(diff) * 1e9 / (DLEN * NITER)));
  fflush(stdout);
}


void test_sha256()
{
  struct sha256ctx s256;
  char str1[] = "abc";
  char str2[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
  byte_t out[32];
  byte_t res1[32] = "\xBA\x78\x16\xBF\x8F\x01\xCF\xEA\x41\x41\x40\xDE"
	 	    "\x5D\xAE\x22\x23\xB0\x03\x61\xA3\x96\x17\x7A\x9C"
		    "\xB4\x10\xFF\x61\xF2\x00\x15\xAD";
  byte_t res2[32] = "\x24\x8D\x6A\x61\xD2\x06\x38\xB8\xE5\xC0\x26\x93"
	            "\x0C\x3E\x60\x39\xA3\x3C\xE4\x59\x64\xFF\x21\x67"
		    "\xF6\xEC\xED\xD4\x19\xDB\x06\xC1";
  int i;

  sha256(str1, sizeof(str1)-1, out);
  printf("sha256 hash of \"%s\" is:\n\t", str1);
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", out[i]);
  printf("\n");
  printf("Expected value is:\n\t");
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", res1[i]);
  printf("\n");
  if (memcmp(out, res1, 32))
    printf("FAILURE!\n");
  else
    printf("SUCCESS\n");
  printf("\n");

  sha256(str2, sizeof(str2)-1, out);
  printf("sha256 hash of \"%s\" is: \n\t", str2);
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", out[i]);
  printf("\n");
  printf("Expected value is:\n\t");
  for ( i = 0 ; i < 32; ++i )
    printf("%02x", res2[i]);
  printf("\n");
  if (memcmp(out, res2, 32))
    printf("FAILURE!\n");
  else
    printf("SUCCESS\n");
}


void perf_sha256()
{
  int i;
  cat_time_t start, diff;

  start = tm_uget();
  for ( i = 0; i < NITER; ++i )
    sha256(rand_data, sizeof(rand_data), rand_data); 
  diff = tm_sub(tm_uget(), start);
  printf("%lf seconds to run %d SHA256 hashes on %d-byte data\n",
         tm_2dbl(diff), NITER, DLEN);
  printf("%lf ns per byte\n", (tm_2dbl(diff) * 1e9 / (DLEN * NITER)));
  fflush(stdout);
}


/*
 * SipHash-2-4 output with
 *   k = 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f
 * and
 *   input = (empty string)
 *   input = 00 (1 byte)
 *   input = 00 01 (2 bytes)
 *   input = 00 01 02 (3 bytes)
 *   ...
 *   input = 00 01 02 ... 3e (63 bytes)
 */
byte_t siphash24_test_vectors[64][8] =
{
  { 0x31, 0x0e, 0x0e, 0xdd, 0x47, 0xdb, 0x6f, 0x72, },
  { 0xfd, 0x67, 0xdc, 0x93, 0xc5, 0x39, 0xf8, 0x74, },
  { 0x5a, 0x4f, 0xa9, 0xd9, 0x09, 0x80, 0x6c, 0x0d, },
  { 0x2d, 0x7e, 0xfb, 0xd7, 0x96, 0x66, 0x67, 0x85, },
  { 0xb7, 0x87, 0x71, 0x27, 0xe0, 0x94, 0x27, 0xcf, },
  { 0x8d, 0xa6, 0x99, 0xcd, 0x64, 0x55, 0x76, 0x18, },
  { 0xce, 0xe3, 0xfe, 0x58, 0x6e, 0x46, 0xc9, 0xcb, },
  { 0x37, 0xd1, 0x01, 0x8b, 0xf5, 0x00, 0x02, 0xab, },
  { 0x62, 0x24, 0x93, 0x9a, 0x79, 0xf5, 0xf5, 0x93, },
  { 0xb0, 0xe4, 0xa9, 0x0b, 0xdf, 0x82, 0x00, 0x9e, },
  { 0xf3, 0xb9, 0xdd, 0x94, 0xc5, 0xbb, 0x5d, 0x7a, },
  { 0xa7, 0xad, 0x6b, 0x22, 0x46, 0x2f, 0xb3, 0xf4, },
  { 0xfb, 0xe5, 0x0e, 0x86, 0xbc, 0x8f, 0x1e, 0x75, },
  { 0x90, 0x3d, 0x84, 0xc0, 0x27, 0x56, 0xea, 0x14, },
  { 0xee, 0xf2, 0x7a, 0x8e, 0x90, 0xca, 0x23, 0xf7, },
  { 0xe5, 0x45, 0xbe, 0x49, 0x61, 0xca, 0x29, 0xa1, },
  { 0xdb, 0x9b, 0xc2, 0x57, 0x7f, 0xcc, 0x2a, 0x3f, },
  { 0x94, 0x47, 0xbe, 0x2c, 0xf5, 0xe9, 0x9a, 0x69, },
  { 0x9c, 0xd3, 0x8d, 0x96, 0xf0, 0xb3, 0xc1, 0x4b, },
  { 0xbd, 0x61, 0x79, 0xa7, 0x1d, 0xc9, 0x6d, 0xbb, },
  { 0x98, 0xee, 0xa2, 0x1a, 0xf2, 0x5c, 0xd6, 0xbe, },
  { 0xc7, 0x67, 0x3b, 0x2e, 0xb0, 0xcb, 0xf2, 0xd0, },
  { 0x88, 0x3e, 0xa3, 0xe3, 0x95, 0x67, 0x53, 0x93, },
  { 0xc8, 0xce, 0x5c, 0xcd, 0x8c, 0x03, 0x0c, 0xa8, },
  { 0x94, 0xaf, 0x49, 0xf6, 0xc6, 0x50, 0xad, 0xb8, },
  { 0xea, 0xb8, 0x85, 0x8a, 0xde, 0x92, 0xe1, 0xbc, },
  { 0xf3, 0x15, 0xbb, 0x5b, 0xb8, 0x35, 0xd8, 0x17, },
  { 0xad, 0xcf, 0x6b, 0x07, 0x63, 0x61, 0x2e, 0x2f, },
  { 0xa5, 0xc9, 0x1d, 0xa7, 0xac, 0xaa, 0x4d, 0xde, },
  { 0x71, 0x65, 0x95, 0x87, 0x66, 0x50, 0xa2, 0xa6, },
  { 0x28, 0xef, 0x49, 0x5c, 0x53, 0xa3, 0x87, 0xad, },
  { 0x42, 0xc3, 0x41, 0xd8, 0xfa, 0x92, 0xd8, 0x32, },
  { 0xce, 0x7c, 0xf2, 0x72, 0x2f, 0x51, 0x27, 0x71, },
  { 0xe3, 0x78, 0x59, 0xf9, 0x46, 0x23, 0xf3, 0xa7, },
  { 0x38, 0x12, 0x05, 0xbb, 0x1a, 0xb0, 0xe0, 0x12, },
  { 0xae, 0x97, 0xa1, 0x0f, 0xd4, 0x34, 0xe0, 0x15, },
  { 0xb4, 0xa3, 0x15, 0x08, 0xbe, 0xff, 0x4d, 0x31, },
  { 0x81, 0x39, 0x62, 0x29, 0xf0, 0x90, 0x79, 0x02, },
  { 0x4d, 0x0c, 0xf4, 0x9e, 0xe5, 0xd4, 0xdc, 0xca, },
  { 0x5c, 0x73, 0x33, 0x6a, 0x76, 0xd8, 0xbf, 0x9a, },
  { 0xd0, 0xa7, 0x04, 0x53, 0x6b, 0xa9, 0x3e, 0x0e, },
  { 0x92, 0x59, 0x58, 0xfc, 0xd6, 0x42, 0x0c, 0xad, },
  { 0xa9, 0x15, 0xc2, 0x9b, 0xc8, 0x06, 0x73, 0x18, },
  { 0x95, 0x2b, 0x79, 0xf3, 0xbc, 0x0a, 0xa6, 0xd4, },
  { 0xf2, 0x1d, 0xf2, 0xe4, 0x1d, 0x45, 0x35, 0xf9, },
  { 0x87, 0x57, 0x75, 0x19, 0x04, 0x8f, 0x53, 0xa9, },
  { 0x10, 0xa5, 0x6c, 0xf5, 0xdf, 0xcd, 0x9a, 0xdb, },
  { 0xeb, 0x75, 0x09, 0x5c, 0xcd, 0x98, 0x6c, 0xd0, },
  { 0x51, 0xa9, 0xcb, 0x9e, 0xcb, 0xa3, 0x12, 0xe6, },
  { 0x96, 0xaf, 0xad, 0xfc, 0x2c, 0xe6, 0x66, 0xc7, },
  { 0x72, 0xfe, 0x52, 0x97, 0x5a, 0x43, 0x64, 0xee, },
  { 0x5a, 0x16, 0x45, 0xb2, 0x76, 0xd5, 0x92, 0xa1, },
  { 0xb2, 0x74, 0xcb, 0x8e, 0xbf, 0x87, 0x87, 0x0a, },
  { 0x6f, 0x9b, 0xb4, 0x20, 0x3d, 0xe7, 0xb3, 0x81, },
  { 0xea, 0xec, 0xb2, 0xa3, 0x0b, 0x22, 0xa8, 0x7f, },
  { 0x99, 0x24, 0xa4, 0x3c, 0xc1, 0x31, 0x57, 0x24, },
  { 0xbd, 0x83, 0x8d, 0x3a, 0xaf, 0xbf, 0x8d, 0xb7, },
  { 0x0b, 0x1a, 0x2a, 0x32, 0x65, 0xd5, 0x1a, 0xea, },
  { 0x13, 0x50, 0x79, 0xa3, 0x23, 0x1c, 0xe6, 0x60, },
  { 0x93, 0x2b, 0x28, 0x46, 0xe4, 0xd7, 0x06, 0x66, },
  { 0xe1, 0x91, 0x5f, 0x5c, 0xb1, 0xec, 0xa4, 0x6c, },
  { 0xf3, 0x25, 0x96, 0x5c, 0xa1, 0x6d, 0x62, 0x9f, },
  { 0x57, 0x5f, 0xf2, 0x8e, 0x60, 0x38, 0x1b, 0xe5, },
  { 0x72, 0x45, 0x06, 0xeb, 0x4c, 0x32, 0x8a, 0x95, }
};


char *out2hex(byte_t out[8], char *s)
{
  sprintf(s, "%02x%02x%02x%02x%02x%02x%02x%02x",
    out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
  return s;
}


void test_siphash24()
{
  byte_t key[16] = { 0 };
  byte_t input[64] = { 0 };
  byte_t out[8];
  char ostr[64], eostr[64];
  int i;
  int fail = 0;

  printf("Testing siphash24 using 64 vectors\n");
  for ( i = 0; i < sizeof(key); ++i ) {
    key[i] = i;
  }
  for ( i = 0; i < sizeof(input); ++i ) {
    input[i] = i;
  }

  for ( i = 0; i < 64; ++i ) {
    siphash24(key, input, i, out);
    if ( memcmp(out, siphash24_test_vectors[i], sizeof(out)) ) {
      printf("siphash for input %d produces output %s: expected %s\n",
             i, out2hex(out, ostr), out2hex(siphash24_test_vectors[i], eostr));
      fail = 1;
    }
  }
  if ( !fail ) {
    printf("All siphash24 test passed\n");
  }
}


void perf_siphash24()
{
  int i;
  cat_time_t start, diff;

  start = tm_uget();
  for ( i = 0; i < NITER; ++i )
    siphash24(sipkey, rand_data, sizeof(rand_data), rand_data); 
  diff = tm_sub(tm_uget(), start);
  printf("%lf seconds to run %d SipHash-2-4 hashes on %d-byte data\n",
         tm_2dbl(diff), NITER, DLEN);
  printf("%lf ns per byte\n", (tm_2dbl(diff) * 1e9 / (DLEN * NITER)));
  fflush(stdout);
}

int main(int argc, char *argv[])
{
  init_data();
  test_arc4();
  perf_arc4();
  test_sha256();
  perf_sha256();
  test_siphash24();
  perf_siphash24();
  return 0;
}
