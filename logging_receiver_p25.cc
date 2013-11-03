
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
	double capture_rate = 4000000;
	float offset = center - (f*1000000);
	
        float system_channel_rate = 48000; //125000;
        float symbol_rate = 4800;
        double symbol_deviation = 600.0;
	double channel_decim = floor(capture_rate / system_channel_rate);
        double channel_rate = floor(capture_rate / channel_decim);
        double trans_width = 12500 / 2;
        double trans_centre = trans_width + (trans_width / 2);
	std::vector<float> sym_taps;
	const double pi = M_PI; //boost::math::constants::pi<double>();

timestamp = time(NULL);
	starttime = time(NULL);

std::cout << " Decim: " << channel_decim << " Rate: " << channel_rate << " trans center: " << trans_centre << std::endl;

/*
        coeffs = gr.firdes.low_pass(1.0, capture_rate, trans_centre, trans_width, gr.firdes.WIN_HANN)
        self.channel_filter = gr.freq_xlating_fir_filter_ccf(channel_decim, coeffs, 0.0, capture_rate)
        self.set_channel_offset(0.0, 0, self.spectrum.win._units)
        # power squelch
        squelch_db = 0
        self.squelch = gr.pwr_squelch_cc(squelch_db, 1e-3, 0, True)
        self.set_squelch_threshold(squelch_db)
        # FM demodulator
        fm_demod_gain = channel_rate / (2.0 * pi * self.symbol_deviation)
        fm_demod = gr.quadrature_demod_cf(fm_demod_gain)
        # symbol filter        
        symbol_decim = 1
        # symbol_coeffs = gr.firdes.root_raised_cosine(1.0, channel_rate, self.symbol_rate, 0.2, 500)
        # boxcar coefficients for "integrate and dump" filter
        samples_per_symbol = channel_rate // self.symbol_rate
        symbol_coeffs = (1.0/samples_per_symbol,)*samples_per_symbol
        symbol_filter = gr.fir_filter_fff(symbol_decim, symbol_coeffs)
        # C4FM demodulator
        autotuneq = gr.msg_queue(2)
        self.demod_watcher = demod_watcher(autotuneq, self.adjust_channel_offset)
        demod_fsk4 = op25.fsk4_demod_ff(autotuneq, channel_rate, self.symbol_rate)
        # symbol slicer
        levels = [ -2.0, 0.0, 2.0, 4.0 ]
        slicer = op25.fsk4_slicer_fb(levels)*/



	prefilter = gr_make_freq_xlating_fir_filter_ccf(int(channel_decim),
		gr_firdes::low_pass(1.0, capture_rate, trans_centre, trans_width, gr_firdes::WIN_HANN),
		offset, 
		capture_rate);
	int squelch_db = 40;
	squelch = gr_make_pwr_squelch_cc(squelch_db, 0.001, 0, true);
	double fm_demod_gain = floor(channel_rate / (2.0 * pi * symbol_deviation));
	demod = gr_make_quadrature_demod_cf(fm_demod_gain);

	double symbol_decim = 1;
        double samples_per_symbol = floor(channel_rate / symbol_rate);
	std::cout << " FM Gain: " << fm_demod_gain << " Samples per sym: " << samples_per_symbol <<  std::endl;
	
	for (int i=0; i < samples_per_symbol; i++) {
		sym_taps.push_back(1.0 / samples_per_symbol);
	}
        //symbol_coeffs = (1.0/samples_per_symbol,)*samples_per_symbol
        sym_filter =  gr_make_fir_filter_fff(symbol_decim, sym_taps);
	tune_queue = gr_make_msg_queue();
	traffic_queue = gr_make_msg_queue();
	const float l[] = { -2.0, 0.0, 2.0, 4.0 };
	std::vector<float> levels( l,l + sizeof( l ) / sizeof( l[0] ) );
	op25_demod = op25_make_fsk4_demod_ff(tune_queue, channel_rate, symbol_rate);
	op25_decoder = op25_make_decoder_bf();
	op25_slicer = op25_make_fsk4_slicer_fb(levels);
	op25_decoder->set_msgq(traffic_queue);


/*
	unsigned int d = GCD(channel_rate, pre_channel_rate);
    	channel_rate = floor(channel_rate  / d);
    	pre_channel_rate = floor(pre_channel_rate / d);

	downsample_sig = gr_make_rational_resampler_base_ccf(channel_rate, pre_channel_rate, design_filter(channel_rate, pre_channel_rate)); 

*/
	
/*
	d = GCD(audio_rate, vocoder_rate);
    	audio_rate = floor(audio_rate  / d);
    	vocoder_rate = floor(vocoder_rate / d);
	upsample_audio = gr_make_rational_resampler_base_fff(audio_rate, vocoder_rate, design_filter(audio_rate, vocoder_rate));
*/
	
	//demod = gr_make_quadrature_demod_cf(1.6);
	

	//const float a[] = { 0.1, 0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1};
	/*const float a[] = { 1.0/6, 1.0/6, 1.0/6, 1.0/6, 1.0/6, 1.0/6};


   	std::vector<float> data( a,a + sizeof( a ) / sizeof( a[0] ) );
	sym_filter = gr_make_fir_filter_fff(1, data); 


	tm *ltm = localtime(&starttime);
	
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
	
	boost::filesystem::create_directories(path_stream.str());
	//sprintf(filename, "%s/%ld-%ld.wav", path_stream.str().c_str(),talkgroup,timestamp);
*/
	sprintf(filename, "%ld-%ld.wav",talkgroup,timestamp);
	
	wav_sink = gr_make_wavfile_sink(filename,1,8000,16);

	
	//gr_file_sink_sptr fs = gr_make_file_sink(sizeof(float),"sym_filter.dat");



	
	connect(self(), 0, prefilter, 0);
	
	muted  = true;

	connect(prefilter, 0, squelch, 0); 
	connect(squelch, 0, demod, 0);
//connect(demod, 0, wav_sink,0);

	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, op25_demod, 0);
	connect(op25_demod,0, op25_slicer, 0);
	connect(op25_slicer,0, op25_decoder,0);
	connect(op25_decoder, 0, wav_sink,0);
/*


	int samp_per_sym = 10;  //6
	double samp_rate = 5000000;
	int decim = 80;  //20
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = double(samp_rate/decim);
	double vocoder_rate = 8000;
	double audio_rate = 44100;
	int symbol_deviation = 600;
	int symbol_rate = 4800;
	const double pi = M_PI; //boost::math::constants::pi<double>();

	timestamp = time(NULL);
	starttime = time(NULL);



    
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

*/
	
/*
	d = GCD(audio_rate, vocoder_rate);
    	audio_rate = floor(audio_rate  / d);
    	vocoder_rate = floor(vocoder_rate / d);
	upsample_audio = gr_make_rational_resampler_base_fff(audio_rate, vocoder_rate, design_filter(audio_rate, vocoder_rate));
*/
	
	//demod = gr_make_quadrature_demod_cf(1.6);
	

	//const float a[] = { 0.1, 0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1};
/*	const float a[] = { 1.0/6, 1.0/6, 1.0/6, 1.0/6, 1.0/6, 1.0/6};


   	std::vector<float> data( a,a + sizeof( a ) / sizeof( a[0] ) );
	sym_filter = gr_make_fir_filter_fff(1, data); 


	tm *ltm = localtime(&starttime);
	
	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;
	
	boost::filesystem::create_directories(path_stream.str());
	//sprintf(filename, "%s/%ld-%ld.wav", path_stream.str().c_str(),talkgroup,timestamp);

	sprintf(filename, "%ld-%ld.wav",talkgroup,timestamp);
	
	wav_sink = gr_make_wavfile_sink(filename,1,8000,16);

	
	//gr_file_sink_sptr fs = gr_make_file_sink(sizeof(float),"sym_filter.dat");



	
	connect(self(), 0, prefilter, 0);
	
	muted  = true;

	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, sym_filter, 0);
	connect(sym_filter, 0, op25_demod, 0);
	connect(op25_demod,0, op25_slicer, 0);
	connect(op25_slicer,0, op25_decoder,0);
	connect(op25_decoder, 0, wav_sink,0);*/
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

char *log_p25::get_filename() {
	return filename;
}

void log_p25::close() {
	mute();
	wav_sink->close();	
	/*disconnect(prefilter, 0, demod, 0);
	disconnect(demod, 0, sym_filter, 0);
	disconnect(sym_filter, 0, op25_demod, 0);
	disconnect(op25_demod,0, op25_slicer, 0);
	disconnect(op25_slicer,0, op25_decoder,0);
	disconnect(op25_decoder, 0, wav_sink,0);*/
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
	

	
