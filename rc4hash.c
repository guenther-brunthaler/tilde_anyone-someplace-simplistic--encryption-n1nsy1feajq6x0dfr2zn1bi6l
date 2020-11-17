/*
 * rc4hash - abuse ARCFOUR as a simplistic and reasonably fast hash algorithm
 *
 * Version 2020.322.3
 *
 * Copyright (c) 2020 Guenther Brunthaler. All rights reserved.
 *
 * This source file is free software.
 * Distribution is permitted under the terms of the GPLv3.
 */

#include "arc4_common.h"
#include <dim_sdbrke8ae851uitgzm4nv3ea2.h>
#include <getopt_nh7lll77vb62ycgwzwf30zlln.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static char const b32custom_alphabet[]= {
   /*
   $ perl -e \
   'print join(", ", map "'\'\$_\''", grep /[^01OI]/, (A..Z, 0..9)), "\n"'
   */
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'
   ,  'J', 'K', 'L', 'M', 'N', 'P', 'Q', 'R'
   ,  'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
   ,  '2', '3', '4', '5', '6', '7', '8', '9'
};

static char const hex_alphabet[]= {
   /* $ perl -e 'print join(", ", map "'\'\$_\''", 0..9, A..F), "\n"' */
      '0', '1', '2', '3', '4', '5', '6', '7'
   ,  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

int main(int argc, char **argv) {
   char const *error= 0;
   int a= 0;
   long digest_bits= 256;
   char const *alphabet= b32custom_alphabet;
   unsigned alphabet_bitmask= DIM(b32custom_alphabet) - 1, alphabet_bits;
   ARCFOUR_VARDEFS(static);
   {
      int optpos= 0;
      for (;;) {
         int opt;
         switch (opt= getopt_simplest(&a, &optpos, argc, argv)) {
            case 0: goto no_more_options;
            case 'x':
               alphabet= hex_alphabet; alphabet_bitmask= DIM(hex_alphabet) - 1;
               break;
            case 'r': alphabet= 0; alphabet_bitmask= (1 << 8) - 1; break;
            case 'B': case 'b':
               {
                  char const *optarg;
                  if (!(optarg= getopt_simplest_mand_arg(
                     &a, &optpos, argc, argv
                  ))) {
                     getopt_simplest_perror_missing_arg(opt); goto leave;
                  }
                  if ((digest_bits= atol(optarg)) < 1) goto bad_digest_size;
                  if (opt == 'B') {
                     long old= digest_bits;
                     if ((digest_bits<<= 3) <= old) goto bad_digest_size;
                  }
               }
               break;
            default: getopt_simplest_perror_opt(opt); goto leave;
         }
      }
   }
   no_more_options:
   {
      unsigned bm;
      for (alphabet_bits= bm= 0; bm != alphabet_bitmask; ++alphabet_bits) {
         bm+= bm + 1;
      }
   }
   if (digest_bits & (long)(alphabet_bits - 1)) {
      bad_digest_size:
      error= "Invalid digest size requested!"; goto fail;
   }
   for (;;) {
      if (a < argc) {
         if (!freopen(argv[a], "rb", stdin)) {
            (void)fputs("Could not open", stderr);
            add_arg:
            (void)fputs(" \"", stderr);
            (void)fputs(argv[a], stderr);
            error= "\"!"; goto fail;
         }
      }
      /* Hash current standard input */
      ARCFOUR_STEP_1;
      ARCFOUR_STEP_2;
      /* Process input as an (overly long) key to set. */
      {
         int c;
         while ((c= getchar()) != EOF) {
            ARCFOUR_STEP_3;
            assert(c >= 0); assert(c < SBOX_SIZE);
            ARCFOUR_STEP_4_SETUP((unsigned)c);
            ARCFOUR_STEP_5_DROP;
         }
      }
      if (ferror(stdin)) {
         if (a < argc) {
            (void)fputs("Error reading", stderr);
            goto add_arg;
         }
         error= "Read error!"; goto fail;
      }
      assert(feof(stdin));
      /* Finish key setup. */
      ARCFOUR_STEP_2;
      /* Drop the initial pseudorandom output. */
      {
         unsigned k;
         for (k= DROP_N; k--; ) {
            ARCFOUR_STEP_3;
            ARCFOUR_STEP_4;
            ARCFOUR_STEP_5_DROP;
         }
      }
      /* Produce the message digest. */
      {
         long k;
         unsigned buf, bufbits= 0;
         #ifndef NDEBUG
            buf= 0;
         #endif
         for (k= digest_bits; k > 0; k-= (long)alphabet_bits) {
            if (bufbits < alphabet_bits) {
               /* Append the bits of another ARCFOUR output octet to <buf>. */
               ARCFOUR_STEP_3;
               ARCFOUR_STEP_4;
               ARCFOUR_STEP_5;
               buf= buf << 8 | ARCFOUR_STEP_6();
               bufbits+= 8;
            }
            assert(bufbits >= alphabet_bits);
            {
               int c= (int)(buf >> bufbits - alphabet_bits & alphabet_bitmask);
               if (alphabet) c= alphabet[c];
               bufbits-= alphabet_bits;
               if (putchar(c) != c) goto wrerr;
            }
         }
         if (a < argc) {
            if (alphabet) if (putchar(' ') == EOF) goto wrerr;
            if (fputs(argv[a], stdout) < 0) goto wrerr;
            if (!alphabet) if (putchar('\0') == EOF) goto wrerr;
         }
         if (alphabet) if (putchar('\n') == EOF) goto wrerr;
      }
      if (!(++a < argc)) break;
   }
   if (fflush(0)) {
      wrerr: error= "Write error!";
      fail:
      (void)fputs(error, stderr);
      (void)fputc('\n', stderr);
   }
   leave: return error ? EXIT_FAILURE : EXIT_SUCCESS;
}
