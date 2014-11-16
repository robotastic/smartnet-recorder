
#include "logging_receiver_p25.h"


log_p25_sptr make_log_p25(float freq, float center, long s, long t, int n)
{
    return gnuradio::get_initial_sptr(new log_p25(freq, center, s, t, n));
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

    std::vector<float> result = gr::filter::firdes::low_pass(
                     interpolation,
                     1,
                     mid_transition_band/interpolation,
                     trans_width/interpolation,
                     gr::filter::firdes::WIN_KAISER,
                     beta
                     );

	return result;
}



log_p25::log_p25(float f, float c, long s, long t, int n)
    : gr::hier_block2 ("log_p25",
          gr::io_signature::make  (1, 1, sizeof(gr_complex)),
          gr::io_signature::make  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
	talkgroup = t;
	long capture_rate = s;
		num = n;
	active = false;

	float offset = (f*1000000) - center;
	
        
        float symbol_rate = 4800;
        double samples_per_symbol = 10;
        float system_channel_rate = symbol_rate * samples_per_symbol;
        double symbol_deviation = 600.0;
		double prechannel_decim = floor(capture_rate / system_channel_rate);
        double prechannel_rate = floor(capture_rate / prechannel_decim);
        double trans_width = 12500 / 2;
        double trans_centre = trans_width + (trans_width / 2);
	std::vector<float> sym_taps;
	const double pi = M_PI; //boost::math::constants::pi<double>();

timestamp = time(NULL);
	starttime = time(NULL);



	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(int(prechannel_decim),
		gr::filter::firdes::low_pass(1.0, capture_rate, trans_centre, trans_width, gr::filter::firdes::WIN_HANN),
		offset, 
		capture_rate);

std::cout << "Prechannel Decim: " << prechannel_decim << " Rate: " << prechannel_rate << " system_channel_rate: " << system_channel_rate << std::endl;
	
		unsigned int d = GCD(prechannel_rate, system_channel_rate);
    	system_channel_rate = floor(system_channel_rate  / d);
    	prechannel_rate = floor(prechannel_rate / d);
std::cout << "After GCD - Prechannel Decim: " << prechannel_decim << " Rate: " << prechannel_rate << " system_channel_rate: " << system_channel_rate << std::endl;


	resampler_taps = design_filter(prechannel_rate, system_channel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(prechannel_rate, system_channel_rate, resampler_taps);


	double fm_demod_gain = floor(system_channel_rate / (2.0 * pi * symbol_deviation));
	demod = gr::analog::quadrature_demod_cf::make(fm_demod_gain);

	double symbol_decim = 1;

	std::cout << " FM Gain: " << fm_demod_gain << " Samples per sym: " << samples_per_symbol <<  std::endl;
	
	for (int i=0; i < samples_per_symbol; i++) {
		sym_taps.push_back(1.0 / samples_per_symbol);
	}
        //symbol_coeffs = (1.0/samples_per_symbol,)*samples_per_symbol
    sym_filter =  gr::filter::fir_filter_fff::make(symbol_decim, sym_taps);
	tune_queue = gr::msg_queue::make(2);
	traffic_queue = gr::msg_queue::make(2);
	const float l[] = { -2.0, 0.0, 2.0, 4.0 };
	std::vector<float> levels( l,l + sizeof( l ) / sizeof( l[0] ) );
	op25_demod = gr::op25::fsk4_demod_ff::make(tune_queue, system_channel_rate, symbol_rate);
	op25_slicer = gr::op25::fsk4_slicer_fb::make(levels);
	op25_decoder = gr::op25::decoder_bf::make();
	op25_decoder->set_msgq(traffic_queue);

	tm *ltm = localtime(&starttime);

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,freq);
	wav_sink = gr::blocks::wavfile_sink::make(filename,1,8000,16);
	null_sink = gr::blocks::null_sink::make(sizeof(gr_complex));

	sprintf(raw_filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
	raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), raw_filename);




	
	connect(self(),0, null_sink,0);

	

}


log_p25::~log_p25() {

}
void log_p25::unmute() {
	// this function gets called everytime their is a TG continuation command. This keeps the timestamp updated.
	timestamp = time(NULL);

}

void log_p25::mute() {

}

bool log_p25::is_active() {
	return active;
}

long log_p25::get_talkgroup() {
	return talkgroup;
}

float log_p25::get_freq() {
	return freq;
}

char *log_p25::get_filename() {
	return filename;
}

int log_p25::lastupdate() {
	return time(NULL) - timestamp;
}

long log_p25::elapsed() {
	return time(NULL) - starttime;
}


void log_p25::tune_offset(float f) {
	freq = f;
	int offset_amount = ((f*1000000) - center);
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
	//std::cout << "Offset set to: " << offset_amount << " Freq: "  << freq << std::endl;
}

void log_p25::deactivate() {
	std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " << std::endl; 

	active = false;

  lock();

	wav_sink->close();
	
	raw_sink->close();
	disconnect(prefilter,0, raw_sink,0);
	connect(self(),0, null_sink,0);

	disconnect(self(), 0, prefilter, 0);

	disconnect(prefilter, 0, downsample_sig, 0);
	disconnect(downsample_sig, 0, demod, 0);
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, op25_demod, 0);
	disconnect(op25_demod,0, op25_slicer, 0);
	disconnect(op25_slicer,0, op25_decoder,0);
	disconnect(op25_decoder, 0, wav_sink,0);
	

	unlock();
}

void log_p25::activate(float f, int t, int n) {

	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;
  //num = n;
  	tm *ltm = localtime(&starttime);
  	std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]  "  <<std::endl;

	prefilter->set_center_freq( (f*1000000) - center); // have to flip for 3.7

	if (iam_logging) {
		//printf("Recording Freq: %f \n", f);
	}



	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,f);
	
	sprintf(raw_filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
    
	
	lock();

	raw_sink->open(raw_filename);
	wav_sink->open(filename);


	disconnect(self(),0, null_sink, 0);
	connect(self(),0, prefilter,0);
	connect(downsample_sig,0, raw_sink,0);
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, op25_demod, 0);
	connect(op25_demod,0, op25_slicer, 0);
	connect(op25_slicer,0, op25_decoder,0);
	connect(op25_decoder, 0, wav_sink,0);
	
	unlock();
	active = true;

}

	
