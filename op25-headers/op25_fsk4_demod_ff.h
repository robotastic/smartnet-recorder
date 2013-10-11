/* -*- C++ -*- */

/*
 * Copyright 2006, 2007 Frank (Radio Rausch)
 * Copyright 2011 Steve Glass
 * 
 * This file is part of OP25.
 * 
 * OP25 is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * OP25 is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with OP25; see the file COPYING.  If not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Boston, MA
 * 02110-1301, USA.
 */

#ifndef INCLUDED_OP25_FSK4_DEMOD_FF_H
#define INCLUDED_OP25_FSK4_DEMOD_FF_H

#include <gr_block.h>
#include <gr_msg_queue.h>

#include <boost/scoped_array.hpp>

typedef boost::shared_ptr<class op25_fsk4_demod_ff> op25_fsk4_demod_ff_sptr;

op25_fsk4_demod_ff_sptr op25_make_fsk4_demod_ff(gr_msg_queue_sptr queue, float sample_rate, float symbol_rate);

/**
 * op25_fsk4_demod_ff is a GNU Radio block for demodulating APCO P25
 * CF4M signals. This class expects its input to consist of a 4 level
 * FSK modulated baseband signal. It produces a stream of symbols.
 *
 * All inputs are post FM demodulator and symbol shaping filter data
 * is normalized before being sent to this block so these parameters
 * should not need adjusting even when working on different signals.
 *
 * Nominal levels are -3, -1, +1, and +3.
 */
class op25_fsk4_demod_ff : public gr_block
{
public:

   /**
    * op25_fsk4_demod_ff (virtual) destructor.
    */
   virtual ~op25_fsk4_demod_ff();

   /**
    * Estimate nof_input_items_reqd for a given nof_output_items.
    */
   virtual void forecast(int noutput_items, gr_vector_int &inputs_required);

   /**
    * Process baseband into symbols.
    */
   virtual int general_work(int noutput_items, gr_vector_int &ninput_items, gr_vector_const_void_star &input_items, gr_vector_void_star &output_items);

private:

   /**
    * Expose class to public ctor. Create a new instance of
    * op25_fsk4_demod_ff and wrap it in a shared_ptr. This is
    * effectively the public constructor.
    */
   friend op25_fsk4_demod_ff_sptr op25_make_fsk4_demod_ff(gr_msg_queue_sptr queue, float sample_rate, float symbol_rate);

   /**
    * op25_fsk4_demod_ff private constructor.
    */
   op25_fsk4_demod_ff(gr_msg_queue_sptr queue, float sample_rate, float symbol_rate);

   /**
    * Called when we want the input frequency to be adjusted.
    */
   void send_frequency_correction();

   /**
    * Tracking loop.
    */
   bool tracking_loop_mmse(float input, float *output);

private:

   const float d_block_rate;

   boost::scoped_array<float> d_history;

   size_t d_history_last;

   gr_msg_queue_sptr d_queue;

   double d_symbol_clock;

   double d_symbol_spread;

   const float d_symbol_time;

   double fine_frequency_correction;

   double coarse_frequency_correction;

};

#endif /* INCLUDED_OP25_FSK4_DEMOD_FF_H */
