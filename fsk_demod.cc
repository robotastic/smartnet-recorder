#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fsk_demod.h>
#include <gr_io_signature.h>
#include <gr_complex.h>

fsk_demod_sptr
smartnet_make_fsk_demod ()
{
    return fsk_demod_sptr (new fsk_demod());
}

static const int MIN_IN = 1;    // mininum number of input streams
static const int MAX_IN = 1;    // maximum number of input streams
static const int MIN_OUT = 1;   // minimum number of output streams
static const int MAX_OUT = 1;   // maximum number of output streams

fsk_demod::fsk_demod ()
  : gr_block ("fsk_demod",
          gr_make_io_signature (MIN_IN, MAX_IN, sizeof (gr_complex)),
          gr_make_io_signature (MIN_OUT, MAX_OUT, sizeof (char)))
{
    // nothing else required in this example
}

fsk_demod::~fsk_demod ()
{
    // nothing else required in this example
}

int  fsk_demod::general_work (int                     noutput_items,
                                gr_vector_int               &ninput_items,
                                gr_vector_const_void_star   &input_items,
                                gr_vector_void_star         &output_items)
{
    const float *in = (const float *) input_items[0];
    float *out = (float *) output_items[0];

    for (int i = 0; i < noutput_items; i++){
        out[i] = in[i] * in[i];
    }

    consume_each (noutput_items);

    // Tell runtime system how many output items we produced.
    return noutput_items;
}
