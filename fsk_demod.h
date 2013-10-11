#ifndef INCLUDED_FSK_DEMOD_H
#define INCLUDED_FSK_DEMOD_H

#include <gr_hier_block.h>

class fsk_demod;

typedef boost::shared_ptr<fsk_demod> fsk_demod_sptr;

fsk_demod_sptr smartnet_make_fsk_demod();


class fsk_demod : public gr_block
{
private:
  // The friend declaration allows smartnet_make_deinterleave to
  // access the private constructor.

  friend fsk_demod_sptr smartnet_make_fsk_demod();

 fsk_demod();  // private constructor

 public:
  ~fsk_demod();  // public destructor

	 int general_work (  int                         noutput_items,
                        gr_vector_int               &ninput_items,
                        gr_vector_const_void_star   &input_items,
                        gr_vector_void_star         &output_items);
};

#endif
