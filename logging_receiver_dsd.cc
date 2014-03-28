
#include "logging_receiver_dsd.h"

bool log_dsd::logging = false;

log_dsd_sptr make_log_dsd(float freq, float center, long t, int n)
{
    return gnuradio::get_initial_sptr(new log_dsd(freq, center, t, n));
}

log_dsd::log_dsd(float f, float c, long t, int n)
    : gr::hier_block2 ("log_dsd",
          gr::io_signature::make  (1, 1, sizeof(gr_complex)),
          gr::io_signature::make  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
	talkgroup = t;
	num = n;

	timestamp = time(NULL);
	starttime = time(NULL);

	float offset = (f*1000000) - center;

	int samp_per_sym = 10;
	double samp_rate = 5000000;	
	double decim = 80;
	float channel_rate = 4800 * samp_per_sym;
	//double decim = samp_rate / channel_rate;
	float xlate_bandwidth = 14000; //24260.0;
	double pre_channel_rate = samp_rate/decim;
	//std::cout << "Decim: " << decim << " Pre Channel Rate: " << pre_channel_rate << std::endl;


	
    	lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000);
	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim, 
						      lpf_taps,
						       offset, 
						       samp_rate);
	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);
	resampler_taps = design_filter(channel_rate, pre_channel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(channel_rate, pre_channel_rate, resampler_taps); 
	demod = gr::analog::quadrature_demod_cf::make(1.6); //1.4);

	
	for (int i=0; i < samp_per_sym; i++) {
		sym_taps.push_back(1.0 / samp_per_sym);
	}
	sym_filter = gr::filter::fir_filter_fff::make(1, sym_taps); 
	if (!logging) {
	iam_logging = true;
	logging = true;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,1,1, false, num);
	} else {
	iam_logging = false;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,0,0, false, num);
	}


	tm *ltm = localtime(&starttime);
	
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
	
	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,freq);
	wav_sink = gr::blocks::wavfile_sink::make(filename,1,8000,16);


	connect(self(), 0, prefilter, 0);	
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	//connect(prefilter, 0, demod, 0);	
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, dsd, 0);
	connect(dsd, 0, wav_sink,0);

	
	


	std::cout << " Recv [ " << num << " ] \t Tg: " << t << "\t Freq: "  << f << std::endl;
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
		
	
}



void log_dsd::tune_offset(float f) {
	freq = f;
	prefilter->set_center_freq(center - (f*1000000));
	//std::cout << "Offset set to: " << (center - f*1000000) << "Freq: "  << f << std::endl;
}
void log_dsd::deactivate() {
	//std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " <<std::endl;
	

	if (iam_logging) {
	logging = false;
	}
	wav_sink->close();




	disconnect(self(), 0, prefilter, 0);	
	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	//disconnect(prefilter, 0, demod, 0);
		
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, dsd, 0);
	disconnect(dsd, 0, wav_sink,0);

	

	//std::cout<< "logging_receiver_dsd.cc: Deactivated Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " <<std::endl;
	
}

void log_dsd::activate(float f, int t) {
	//std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ] - freq[ " << f << "] \t talkgroup[ " << t << " ] " <<std::endl;
	
	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;



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


	

	
