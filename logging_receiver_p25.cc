
#include "logging_receiver_p25.h"


log_p25_sptr make_log_p25(float freq, float center, long t)
{
    return gnuradio::get_initial_sptr(new log_p25(freq, center, t));
}


unsigned log_p25::GCD(unsigned u, unsigned v) {
    while ( v != 0) {
        unsigned r = u % v;
        u = v;
        v = r;
    }
    return u;
}

std::vector<float> log_p25::design_filter(double interpolation, double deci) {
    float beta = 5.0;
    float trans_width = 0.5 - 0.4;
    float mid_transition_band = 0.5 - trans_width/2;

	std::vector<float> result = gr_firdes::low_pass(
		              interpolation,
				1,	                     
	                      mid_transition_band/interpolation, 
                              trans_width/interpolation,         
                              gr_firdes::WIN_KAISER,
                              beta                               
                              );

	return result;
}


log_p25::log_p25(float f, float c, long t)
    : gr_hier_block2 ("log_p25",
          gr_make_io_signature  (1, 1, sizeof(gr_complex)),
          gr_make_io_signature  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
	talkgroup = t;
	float offset = center - (f*1000000);
	char filename[160];
	//int samp_per_sym = 10;
	int samp_per_sym = 6;
	double samp_rate = 4000000;
	int decim = 80;
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = double(samp_rate/decim);
	double vocoder_rate = 8000;
	double audio_rate = 44100;
	int symbol_deviation = 600;
	int symbol_rate = 4800;
	const double pi = M_PI; //boost::math::constants::pi<double>();

	timestamp = time(NULL);




    
	prefilter = gr_make_freq_xlating_fir_filter_ccf(decim, 
						       gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000),
						       offset, 
						       samp_rate);


	tune_queue = gr_make_msg_queue();
	traffic_queue = gr_make_msg_queue();
	const float l[] = { -2.0, 0.0, 2.0, 4.0 };
	std::vector<float> levels( l,l + sizeof( l ) / sizeof( l[0] ) );
	op25_demod = op25_make_fsk4_demod_ff(tune_queue, channel_rate, 4800);
	op25_decoder = op25_make_decoder_bf();
	op25_slicer = op25_make_fsk4_slicer_fb(levels);
	op25_decoder->set_msgq(traffic_queue);

	demod = gr_make_quadrature_demod_cf(channel_rate/(2.0 * pi * symbol_deviation));

	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);

	downsample_sig = gr_make_rational_resampler_base_ccf(channel_rate, pre_channel_rate, design_filter(channel_rate, pre_channel_rate)); 


	
/*
	d = GCD(audio_rate, vocoder_rate);
    	audio_rate = floor(audio_rate  / d);
    	vocoder_rate = floor(vocoder_rate / d);
	upsample_audio = gr_make_rational_resampler_base_fff(audio_rate, vocoder_rate, design_filter(audio_rate, vocoder_rate));
*/
	
	//demod = gr_make_quadrature_demod_cf(1.6);
	

	//const float a[] = { 0.1, 0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1};
	const float a[] = { 1.0/6, 1.0/6, 1.0/6, 1.0/6, 1.0/6, 1.0/6};


   	std::vector<float> data( a,a + sizeof( a ) / sizeof( a[0] ) );
	sym_filter = gr_make_fir_filter_fff(1, data); 
	//dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_AUTO_SELECT,3,2,0, false, talkgroup);


	//null_sink = gr_make_null_sink(sizeof(gr_complex));
null_sink = gr_make_null_sink(sizeof(float));
	null_source = gr_make_null_source(sizeof(gr_complex));
	copier = gr_make_kludge_copy(sizeof(gr_complex));
	head_source = gr_make_head(sizeof(gr_complex),0);
	sprintf(filename, "%ld-%ld.wav", talkgroup,timestamp);
	wav_sink = gr_make_wavfile_sink(filename,1,8000,16); 
	//gr_file_sink_sptr fs = gr_make_file_sink(sizeof(float),"sym_filter.dat");



	
	connect(self(), 0, prefilter, 0);
	

	//start off muted	
/*
	connect(self(), 0, null_sink, 0);
	connect(null_source,0,head_source,0);
	connect(head_source,0,prefilter,0);*/
	muted  = true;

	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, op25_demod, 0);
	connect(op25_demod,0, op25_slicer, 0);
	connect(op25_slicer,0, op25_decoder,0);
	connect(op25_decoder, 0, wav_sink,0);
	//connect(sym_filter, 0, wav_sink,0);
		
	
	//connect(dsd, 0, upsample_audio,0);
	//connect(upsample_audio, 0, self(),0);
	//connect(sym_filter,0,fs,0);	
}

log_p25::~log_p25() {

}
// from: /gnuradio/grc/grc_gnuradio/blks2/selector.py
void log_p25::unmute() {
	// this function gets called everytime their is a TG continuation command. This keeps the timestamp updated.
	timestamp = time(NULL);
	if (muted) {

	/*disconnect(self(),0, null_sink,0);
	disconnect(head_source,0, prefilter,0);
	connect(head_source,0, null_sink,0);*/
	/*connect(self(),0, copier,0);
	connect(copier,0, prefilter,0);*/
	//connect(self(),0, prefilter,0);
	muted = false;
	}
}

void log_p25::mute() {

	if (!muted) {
	
	//disconnect(self(),0, prefilter,0);
	
	/*disconnect(self(),0, copier,0);
	disconnect(copier,0, prefilter,0);*/
	/*disconnect(head_source,0, null_sink,0);
	
	connect(head_source,0,prefilter,0);
	connect(self(),0, null_sink,0);
	*/muted = true;
	}
}

long log_p25::get_talkgroup() {
	return talkgroup;
}

float log_p25::get_freq() {
	return freq;
}

long log_p25::timeout() {
	return time(NULL) - timestamp;
}

void log_p25::close() {
	mute();
	wav_sink->close();	
}

/*
void log_p25::forecast(int noutput_items, gr_vector_int &ninput_items_required) {
	ninput_items_required[0] = 4 * noutput_items;
}*/


void log_p25::tune_offset(float f) {
	freq = f;
	prefilter->set_center_freq(center - (f*1000000));
	std::cout << "Offset set to: " << (f*1000000-center) << std::endl;
}
	

	
