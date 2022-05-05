#include <assert.h>
#include <klee/klee.h>
#include <stdio.h>

typedef unsigned int u32;
typedef unsigned long long u64;
typedef unsigned char BitSequence; 
typedef unsigned long long DataLength;

typedef enum { SUCCESS=0, FAIL=1, BAD_HASHBITLEN=2  } HashReturn;

typedef struct  { 
  int datalen;     /* amount of remaining data to hash (bits) */
  /*
    variables for the 32-bit version  
  */
  u32 t32[2];         /* number of bits hashed so far */
} hashState;

static HashReturn Update32(hashState * state, const BitSequence * data, DataLength databitlen ) {


  int fill;
  int left; /* to handle data inputs of up to 2^64-1 bits */
  
  if ( ( databitlen == 0 ) && (state->datalen != 512 ) )
    return SUCCESS;

  left = (state->datalen >> 3); 
  fill = 64 - left;

  /* compress remaining data filled with new bits */
  if( left && ( ((databitlen >> 3) ) >= fill ) ) {
    //memcpy( (void *) (state->data32 + left),
	    //(void *) data, fill );
    /* update counter */
    state->t32[0] += 512;
    if (state->t32[0] == 0)
      state->t32[1]++;
      
    //compress32( state, state->data32 );
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
    //compress32( state, data );
    data += 64;
    databitlen  -= 512;
  }
  
  if( databitlen > 0 ) {
    //memcpy( (void *) (state->data32 + left),
	    //(void *) data, databitlen>>3 );
    state->datalen = (left<<3) + databitlen;
    /* when non-8-multiple, add remaining bits (1 to 7)*/
    //if ( databitlen & 0x7 )
      //state->data32[left + (databitlen>>3)] = data[databitlen>>3];
  }
  else
    state->datalen=0;


  return SUCCESS;
}

int main() {
  hashState state, state1;
  DataLength databitlen, databitlen1, databitlen2;

  klee_make_symbolic(&state, sizeof(state), "state");
  klee_make_symbolic(&state1, sizeof(state1), "state1");
  klee_make_symbolic(&databitlen, sizeof(databitlen), "databitlen");
  klee_make_symbolic(&databitlen1, sizeof(databitlen1), "databitlen1");
  klee_make_symbolic(&databitlen2, sizeof(databitlen2), "databitlen2");

  klee_assume(state.datalen == state1.datalen);
  klee_assume(state.datalen >= 0);
  klee_assume(state.t32[0] == state1.t32[0]);
  klee_assume(state.t32[1] == state1.t32[1]);
  klee_assume(state.t32[0] == 0);// TODO
  klee_assume(state.t32[1] == 0);// TODO  
  
  klee_assume(state.datalen < 512); // TODO: maybe <= 512?

  klee_assume(databitlen == databitlen1 + databitlen2);
  klee_assume(databitlen >= databitlen1);
  klee_assume(databitlen >= databitlen2);
  klee_assume(databitlen1 % 8 == 0);
  
  //klee_assume(databitlen == 512);// TODO
  //klee_assume(databitlen1 == 248); // TODO

  Update32(&state, NULL, databitlen);

  Update32(&state1, NULL, databitlen1);
  Update32(&state1, NULL, databitlen2);

  //printf("PRINT: %i != %i\n",state.datalen,state1.datalen);
  if (state.datalen != state1.datalen) klee_assert(0);
  // TODO more asserts ...
  
  return 0;
}
