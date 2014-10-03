#ifndef LPOCSAG_H
#define LPOCSAG_H

#include <cstdio>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>


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
#include "dsd_block_ff.h"
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>
#include <gnuradio/blocks/multiply_cc.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/filter/rational_resampler_base_ccf.h>
#include <gnuradio/filter/rational_resampler_base_fff.h>
#include <gnuradio/block.h>
//Valve
#include <gnuradio/blocks/null_sink.h>
#include <gnuradio/blocks/null_source.h>
#include <gnuradio/blocks/head.h>
//#include <gnuradio/kludge_copy.h>
//#include <smartnet_wavsink.h>
#include <gnuradio/blocks/wavfile_sink.h>
//#include <blocks/wavfile_sink.h>

class log_pocsag;

typedef boost::shared_ptr<log_pocsag> log_pocsag_sptr;

log_pocsag_sptr make_log_pocsag(float f, float c, long t, int n);

class log_pocsag : public gr::hier_block2
{
  friend log_pocsag_sptr make_log_pocsag(float f, float c, long t, int n);
protected:
    log_pocsag(float f, float c, long t, int n);

public:
    ~log_pocsag();
	void tune_offset(float f);
	void activate(float f, int talkgroup);
	void deactivate();
	float get_freq();
	long get_talkgroup();
	long timeout();
	long elapsed();
	void close();
	void mute();
	void unmute();	
	char *get_filename();
	//void forecast(int noutput_items, gr_vector_int &ninput_items_required);
	static bool logging;
private:
	float center, freq;
	bool muted;
	long talkgroup;
	time_t timestamp;
	time_t starttime;
	char filename[160];
	int num;
    	
	bool iam_logging;
	std::vector<float> lpf_taps;
	std::vector<float> resampler_taps;
	std::vector<float> sym_taps;

    /* GR blocks */
    	gr::filter::fir_filter_ccf::sptr lpf;
	gr::filter::fir_filter_fff::sptr sym_filter;
	gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter;
	gr::analog::sig_source_c::sptr offset_sig; 

	gr::blocks::multiply_cc::sptr mixer;
	gr::blocks::file_sink::sptr fs;
 	gr::blocks::multiply_const_ff::sptr quiet;

	gr::filter::rational_resampler_base_ccf::sptr downsample_sig;
	gr::filter::rational_resampler_base_fff::sptr upsample_audio;
	//gr::analog::quadrature_demod_cf::sptr demod;
	gr::analog::quadrature_demod_cf::sptr demod;
	dsd_block_ff_sptr dsd;
	gr::blocks::wavfile_sink::sptr wav_sink;
	//smartnet_wavsink_sptr wav_sink
	//gr::blocks::wavfile_sink::sptr wav_sink;
	gr::blocks::null_sink::sptr null_sink;
	gr::blocks::null_source::sptr null_source;
	gr::blocks::head::sptr head_source;
	//gr_kludge_copy_sptr copier;

};


#endif

