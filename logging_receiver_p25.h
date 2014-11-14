#ifndef LPF_H
#define LPF_H

#define _USE_MATH_DEFINES

#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>


#include <boost/shared_ptr.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <gnuradio/io_signature.h>
#include <gnuradio/hier_block2.h>
#include <gnuradio/blocks/multiply_const_ff.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_fff.h>
#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/rational_resampler_base_ccc.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/quadrature_demod_cf.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/block.h>
#include <gnuradio/blocks/null_sink.h>

#include <op25/decoder_bf.h>
#include <op25/fsk4_demod_ff.h>
#include <op25/fsk4_slicer_fb.h>
#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>

#include <gnuradio/blocks/head.h>

#include <gnuradio/blocks/wavfile_sink.h>
#include <gnuradio/blocks/file_sink.h>

#include "smartnet.h"


class log_p25;

typedef boost::shared_ptr<log_p25> log_p25_sptr;

log_p25_sptr make_log_p25(float f, float c, long t);

class log_p25 : public gr_hier_block2
{
  friend log_p25_sptr make_log_p25(float f, float c, long t);
protected:
    log_p25(float f, float c, long t);

public:
    ~log_p25();
	void tune_offset(float f);
	float get_freq();
	long get_talkgroup();
	long timeout();
	void close();
	void mute();
	void unmute();	
	char *get_filename();
	//void forecast(int noutput_items, gr_vector_int &ninput_items_required);

private:
	float center, freq;
	bool muted;
	long talkgroup;
	time_t timestamp;
    	time_t starttime;

char filename[160];

    /* GR blocks */
    	gr_fir_filter_ccf_sptr lpf;
	gr_fir_filter_fff_sptr sym_filter;
	gr_freq_xlating_fir_filter_ccf_sptr prefilter;
	gr_sig_source_c_sptr offset_sig; 

	gr_multiply_cc_sptr mixer;
	gr_file_sink_sptr fs;

	gr_rational_resampler_base_ccf_sptr downsample_sig;
	gr_rational_resampler_base_fff_sptr upsample_audio;
	//gr::analog::quadrature_demod_cf::sptr demod;
	gr_quadrature_demod_cf_sptr demod;
	gr_pwr_squelch_cc_sptr squelch;
	gr_wavfile_sink_sptr wav_sink;
	//smartnet_wavsink_sptr wav_sink
	//gr::blocks::wavfile_sink::sptr wav_sink;
	gr_null_sink_sptr null_sink;
	gr_null_source_sptr null_source;
	gr_head_sptr head_source;
	gr_kludge_copy_sptr copier;
	op25_fsk4_demod_ff_sptr op25_demod;
	op25_decoder_bf_sptr op25_decoder;
	op25_fsk4_slicer_fb_sptr op25_slicer;
	gr_msg_queue_sptr tune_queue;
	gr_msg_queue_sptr traffic_queue;
	unsigned GCD(unsigned u, unsigned v);
	std::vector<float> design_filter(double interpolation, double deci);
};


#endif

