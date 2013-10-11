
#include "logging_receiver_dsd.h"

log_dsd_sptr make_log_dsd(float freq, float center, long t, int n)
{
    return gnuradio::get_initial_sptr(new log_dsd(freq, center, t, n));
}
unsigned GCD(unsigned u, unsigned v) {
    while ( v != 0) {
        unsigned r = u % v;
        u = v;
        v = r;
    }
    return u;
}

std::vector<float> design_filter(double interpolation, double deci) {
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
log_dsd::log_dsd(float f, float c, long t, int n)
    : gr_hier_block2 ("log_dsd",
          gr_make_io_signature  (1, 1, sizeof(gr_complex)),
          gr_make_io_signature  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
	talkgroup = t;
	num = n;



	muted  = true;
}

log_dsd::~log_dsd() {
std::cout<< "logging_receiver_dsd.cc: destructor" <<std::endl;

}
// from: /gnuradio/grc/grc_gnuradio/blks2/selector.py
void log_dsd::unmute() {
	// this function gets called everytime their is a TG continuation command. This keeps the timestamp updated.
	timestamp = time(NULL);
	if (muted) {
	muted = false;
	}
}

void log_dsd::mute() {

	if (!muted) {
	
		muted = true;
	}
}

long log_dsd::get_talkgroup() {
	return talkgroup;
}

float log_dsd::get_freq() {
	return freq;
}

char *log_dsd::get_filename() {
	return filename;
}

long log_dsd::timeout() {
	return time(NULL) - timestamp;
}

long log_dsd::elapsed() {
	return time(NULL) - starttime;
}

void log_dsd::close() {

	std::cout<< "logging_receiver_dsd.cc: close()" <<std::endl;
	wav_sink->close();

	disconnect(self(), 0, prefilter, 0);	
	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, dsd, 0);
	disconnect(dsd, 0, wav_sink,0);
		
	std::cout<< "logging_receiver_dsd.cc: finished close()" <<std::endl;
}



void log_dsd::tune_offset(float f) {
	freq = f;
	prefilter->set_center_freq(center - (f*1000000));
	std::cout << "Offset set to: " << (center - f*1000000) << "Freq: "  << f << std::endl;
}
void log_dsd::deactivate() {
	std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " <<std::endl;
	
	wav_sink->close();

	disconnect(self(), 0, prefilter, 0);	
	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, dsd, 0);
	disconnect(dsd, 0, wav_sink,0);
	
	wav_sink.reset();
	prefilter.reset();
	downsample_sig.reset();
	demod.reset();
	sym_filter.reset();
	dsd.reset();

	std::cout<< "logging_receiver_dsd.cc: Deactivated Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " <<std::endl;
	
}

void log_dsd::activate(float f, int t) {
	std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ] - freq[ " << f << "] \t talkgroup[ " << t << " ] " <<std::endl;
	
	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;


	float offset = center - (f*1000000);
	
	int samp_per_sym = 10;
	double samp_rate = 4000000;
	int decim = 80;
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = double(samp_rate/decim);
	double vocoder_rate = 8000;
	double audio_rate = 44100;




    
	prefilter = gr_make_freq_xlating_fir_filter_ccf(decim, 
						       gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000),
						       offset, 
						       samp_rate);
	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);

	downsample_sig = gr_make_rational_resampler_base_ccf(channel_rate, pre_channel_rate, design_filter(channel_rate, pre_channel_rate)); 

	demod = gr_make_quadrature_demod_cf(1.6);

	const float a[] = { 0.1, 0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1};

   	std::vector<float> data( a,a + sizeof( a ) / sizeof( a[0] ) );
	sym_filter = gr_make_fir_filter_fff(1, data); 
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_AUTO_SELECT,3,0,0, false);
	sprintf(filename, "%ld-%ld.wav", talkgroup,timestamp);
	wav_sink = gr_make_wavfile_sink(filename,1,8000,16); 

	connect(self(), 0, prefilter, 0);	
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, dsd, 0);
	connect(dsd, 0, wav_sink,0);

/*
	prefilter->set_center_freq(center - (f*1000000));
	std::cout << "logging_receiver_dsd.cc: Offset set to: " << (center - f*1000000) << "Freq: "  << f << std::endl;

	sprintf(filename, "%ld-%ld.wav", talkgroup,timestamp);
	wav_sink->open(filename); // = gr_make_wavfile_sink(filename,1,8000,16); 

*/

	std::cout<< "logging_receiver_dsd.cc: Activated Logger [ " << num << " ] - freq[ " << f << "] \t talkgroup[ " << talkgroup << " ] " <<std::endl;
}


	

	
