
#include "logging_receiver_dsd.h"

bool log_dsd::logging = false;

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

	timestamp = time(NULL);
	starttime = time(NULL);

	float offset = center - (f*1000000);

	int samp_per_sym = 10;
	double samp_rate = 5000000;	
	double decim = 80;
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = samp_rate/decim;
	


	
    	lpf_taps =  gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000);
	prefilter = gr_make_freq_xlating_fir_filter_ccf(decim, 
						      lpf_taps,
						       offset, 
						       samp_rate);
	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);
	resampler_taps = design_filter(channel_rate, pre_channel_rate);

	downsample_sig = gr_make_rational_resampler_base_ccf(channel_rate, pre_channel_rate, resampler_taps); 
	//quiet = gr_make_multiply_const_ff(0.75);
	demod = gr_make_quadrature_demod_cf(1.6); //1.4);

	
	for (int i=0; i < samp_per_sym; i++) {
		sym_taps.push_back(1.0 / samp_per_sym);
	}
	sym_filter = gr_make_fir_filter_fff(1, sym_taps); 
	if (!logging) {
	iam_logging = true;
	logging = true;
	//dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,1,1, false, num);
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,0,0, false, num);
	
	} else {
	iam_logging = false;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,0,0, false, num);
	}


	tm *ltm = localtime(&starttime);
	
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
	
	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,freq);
	wav_sink = gr_make_wavfile_sink(filename,1,8000,16);
	null_sink = gr_make_null_sink(sizeof(gr_complex));
	//null_source = gr_make_null_source(sizeof(gr_complex));
	//copier = gr_make_kludge_copy(sizeof(gr_complex));
	

	connect(self(),0, null_sink,0);
	//connect(null_source,0,prefilter,0);
	//connect(self(), 0, prefilter, 0);	
	/*connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, dsd, 0);
	connect(dsd, 0, wav_sink,0);*/



	//std::cout << " Recv [ " << num << " ] \t Tg: " << t << "\t Freq: "  << f << std::endl;
}

log_dsd::~log_dsd() {
//std::cout<< "logging_receiver_dsd.cc: destructor" <<std::endl;

}
// from: /gnuradio/grc/grc_gnuradio/blks2/selector.py
void log_dsd::unmute() {
	// this function gets called everytime their is a TG continuation command. This keeps the timestamp updated.
	timestamp = time(NULL);

}

void log_dsd::mute() {


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

long log_dsd::timeleft() {
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
		
	//std::cout<< "logging_receiver_dsd.cc: finished close()" <<std::endl;
}



void log_dsd::tune_offset(float f) {
	freq = f;
	prefilter->set_center_freq(center - (f*1000000));
	//std::cout << "Offset set to: " << (center - f*1000000) << "Freq: "  << f << std::endl;
}
void log_dsd::deactivate() {
	//std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " <<std::endl;
	
  lock();

	if (iam_logging) {
	logging = false;
	}
	wav_sink->close();

	disconnect(self(), 0, prefilter, 0);
	connect(self(),0, null_sink,0);
	



	//disconnect(self(), 0, prefilter, 0);	
	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, dsd, 0);
	disconnect(dsd, 0, wav_sink,0);

	unlock();

/*	
	wav_sink.reset();
	prefilter.reset();
	downsample_sig.reset();
	demod.reset();
	sym_filter.reset();
	dsd.reset();
*/
	//std::cout<< "logging_receiver_dsd.cc: Deactivated Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " <<std::endl;
	
}

void log_dsd::activate(float f, int t, int num) {
	//std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ] - freq[ " << f << "] \t talkgroup[ " << t << " ] " <<std::endl;
	
	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;

	prefilter->set_center_freq(center - (f*1000000));
	std::cout << "logging_receiver_dsd.cc: Offset set to: " << (center - f*1000000) << "Freq: "  << f << std::endl;
	

	
	tm *ltm = localtime(&starttime);
	
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
	
	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%d.wav", path_stream.str().c_str(),talkgroup,timestamp,num);
	lock();
	disconnect(self(),0, null_sink, 0);
	connect(self(),0, prefilter,0);
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, dsd, 0);
	connect(dsd, 0, wav_sink,0);

	wav_sink->open(filename); // = gr_make_wavfile_sink(filename,1,8000,16);
	unlock();	
/*

// The  Commented section below creates all of the blocks and connects them

	float offset = center - (f*1000000);

	int samp_per_sym = 10;
	double samp_rate = 5000000;
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
	if (!logging) {
	iam_logging = true;
	logging = true;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,1,1, false);
	} else {
	iam_logging = false;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,0,0, false);
	}
	sprintf(filename, "%ld-%ld.wav", talkgroup,timestamp);
	wav_sink = gr_make_wavfile_sink(filename,1,8000,16); 

	connect(self(), 0, prefilter, 0);	
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, dsd, 0);
	connect(dsd, 0, wav_sink,0);*/


/*

	prefilter->set_center_freq(center - (f*1000000));
	std::cout << "logging_receiver_dsd.cc: Offset set to: " << (center - f*1000000) << "Freq: "  << f << std::endl;
	

	
	tm *ltm = localtime(&starttime);
	
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
	
	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld.wav", path_stream.str().c_str(),talkgroup,timestamp);
	wav_sink->open(filename); // = gr_make_wavfile_sink(filename,1,8000,16); */
}


	

	
