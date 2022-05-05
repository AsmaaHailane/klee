#include <assert.h>
#include <klee/klee.h>

typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned char BitSequence; 
typedef unsigned long long DataLength; 

typedef enum { SUCCESS=0, FAIL=1, BAD_HASHBITLEN=2  } HashReturn;

typedef struct  { 
  int hashbitlen;  /* length of the hash value (bits) */
  int datalen;     /* amount of remaining data to hash (bits) */
  //u32 h; // TODO
  /*
    variables for the 32-bit version  
  */
  u32 t32[2];         /* number of bits hashed so far */
  BitSequence data32[64];     /* remaining data to hash (less than a block) */
  /*
    variables for the 64-bit version  
  */
  u64 t64[2];      /* number of bits hashed so far */
  BitSequence data64[128];  /* remaining data to hash (less than a block) */
} hashState;

void *memcpy(void *dest, const void *src, size_t n) {
  // do nothing
  return dest;
}

int memcmp(const void *r1, const void *r2, size_t n) {
  const unsigned char *s1 = (const unsigned char*) r1;
  const unsigned char *s2 = (const unsigned char*) r2;
  
  while (n-- > 0) {
    if (*s1++ != *s2++)\
      return s1[-1] < s2[-1] ? -1 : 1;;
  }
  return 0;
}

static HashReturn compress32( hashState * state, const BitSequence * datablock ) {
  // do nothing
  //(*state)+=nblocks; // number of calls
  return SUCCESS;
}

static HashReturn compress64( hashState * state, const BitSequence * datablock ) {
  // do nothing
  return SUCCESS;
}

// BEGIN: unmodified code from blake_ref.c
static HashReturn Update32(hashState * state, const BitSequence * data, DataLength databitlen ) {


  int fill;
  int left; /* to handle data inputs of up to 2^64-1 bits */
  
  if ( ( databitlen == 0 ) && (state->datalen != 512 ) )
    return SUCCESS;

  left = (state->datalen >> 3); 
  fill = 64 - left;

  /* compress remaining data filled with new bits */
  if( left && ( ((databitlen >> 3) & 0x3F) >= fill ) ) {
    memcpy( (void *) (state->data32 + left),
	    (void *) data, fill );
    /* update counter */
    state->t32[0] += 512;
    if (state->t32[0] == 0)
      state->t32[1]++;
      
    compress32( state, state->data32 );
    data += fill;
    databitlen  -= (fill << 3); 
      
    left = 0;
  }

  /* compress data until enough for a block */
  while( databitlen >= 512 ) {

    /* update counter */
    state->t32[0] += 512;

    if (state->t32[0] == 0)
      state->t32[1]++;
    compress32( state, data );
    data += 64;
    databitlen  -= 512;
  }
  
  if( databitlen > 0 ) {
    memcpy( (void *) (state->data32 + left),
	    (void *) data, databitlen>>3 );
    state->datalen = (left<<3) + databitlen;
    /* when non-8-multiple, add remaining bits (1 to 7)*/
    if ( databitlen & 0x7 )
      state->data32[left + (databitlen>>3)] = data[databitlen>>3];
  }
  else
    state->datalen=0;


  return SUCCESS;
}

static HashReturn Update64(hashState * state, const BitSequence * data, DataLength databitlen ) {


  int fill;
  int left;

  if ( ( databitlen == 0 ) && (state->datalen != 1024 ) )
    return SUCCESS;

  left = (state->datalen >> 3);
  fill = 128 - left;

  /* compress remaining data filled with new bits */
  if( left && ( ((databitlen >> 3) & 0x7F) >= fill ) ) {
    memcpy( (void *) (state->data64 + left),
	    (void *) data, fill );
    /* update counter  */
   state->t64[0] += 1024;

   compress64( state, state->data64 );
   data += fill;
   databitlen  -= (fill << 3); 
      
    left = 0;
  }

  /* compress data until enough for a block */
  while( databitlen >= 1024 ) {
  
    /* update counter */
   state->t64[0] += 1024;
   compress64( state, data );
    data += 128;
    databitlen  -= 1024;
  }

  if( databitlen > 0 ) {
    memcpy( (void *) (state->data64 + left),
	    (void *) data, ( databitlen>>3 ) & 0x7F );
    state->datalen = (left<<3) + databitlen;

    /* when non-8-multiple, add remaining bits (1 to 7)*/
    if ( databitlen & 0x7 )
      state->data64[left + (databitlen>>3)] = data[databitlen>>3];
  }
  else
    state->datalen=0;

  return SUCCESS;
}


HashReturn Update(hashState * state, const BitSequence * data, DataLength databitlen ) {

  if ( state->hashbitlen < 384 )
    return Update32( state, data, databitlen );
  else
    return Update64( state, data, databitlen );
}
// END: unmodified code from blake_ref.c

void test(int hashbitlen) {
  hashState state, state1;
  DataLength databitlen, databitlen1, databitlen2;

  klee_make_symbolic(&state, sizeof(state), "state");
  klee_make_symbolic(&state1, sizeof(state1), "state1");
  klee_make_symbolic(&databitlen, sizeof(databitlen), "databitlen");
  klee_make_symbolic(&databitlen1, sizeof(databitlen1), "databitlen1");
  klee_make_symbolic(&databitlen2, sizeof(databitlen2), "databitlen2");

  klee_assume(memcmp(&state, &state1, sizeof(state))==0);
  
  state.hashbitlen = hashbitlen;
  state1.hashbitlen = hashbitlen;
  
  if (hashbitlen < 384) {
    klee_assume(state.datalen < 512);
    klee_assume(state1.datalen < 512);
  } else {
    klee_assume(state.datalen < 1024);
    klee_assume(state1.datalen < 1024);
  }

  klee_assume(databitlen == databitlen1 + databitlen2);
  klee_assume(databitlen >= databitlen1);
  klee_assume(databitlen >= databitlen2);
  klee_assume(databitlen1 % 8 == 0);

  Update(&state, NULL, databitlen);

  Update(&state1, NULL, databitlen1);
  Update(&state1, NULL, databitlen2);

  if (memcmp(&state, &state1, sizeof(state))!=0) klee_assert(0);
}

int main() {
  //test(224);
  test(256);
  //test(384);
  //test(512);
  
  return 0;
}
