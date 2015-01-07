
#include "logging_receiver_dsd.h"
using namespace std;

bool log_dsd::logging = false;

log_dsd_sptr make_log_dsd(float freq, float center, long s, long t, int n)
{
    return gnuradio::get_initial_sptr(new log_dsd(freq, center, s, t, n));
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

log_dsd::log_dsd(float f, float c, long s, long t, int n)
    : gr::hier_block2 ("log_dsd",
          gr::io_signature::make  (1, 1, sizeof(gr_complex)),
          gr::io_signature::make  (0, 0, sizeof(float)))
{
	freq = f;
	center = c;
 	samp_rate = s;
	talkgroup = t;
	num = n;
	active = false;

	timestamp = time(NULL);
	starttime = time(NULL);

	float offset = (f*1000000) - center; //have to flip for 3.7

	int samp_per_sym = 10;
	double decim = 80;
	float xlate_bandwidth = 14000; //24260.0;
	float channel_rate = 4800 * samp_per_sym;
	double pre_channel_rate = samp_rate/decim;

        lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000);
	//lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 3000);
	
	prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim,
						      lpf_taps,
						       offset,
						       samp_rate);
	unsigned int d = GCD(channel_rate, pre_channel_rate); //4000 GCD(48000, 100000)
    	channel_rate = floor(channel_rate  / d);  // 12
    	pre_channel_rate = floor(pre_channel_rate / d);  // 25
	resampler_taps = design_filter(channel_rate, pre_channel_rate);

	downsample_sig = gr::filter::rational_resampler_base_ccf::make(channel_rate, pre_channel_rate, resampler_taps); //downsample from 100k to 48k

	//k = quad_rate/(2*math.pi*max_dev) = 48k / (6.283185*5000) = 1.527
     
	demod = gr::analog::quadrature_demod_cf::make(1.527); //1.6 //1.4);
	levels = gr::blocks::multiply_const_ff::make(1); //33);
	valve = gr::blocks::copy::make(sizeof(gr_complex));
	valve->set_enabled(false);

	float tau = 0.000075; //75us
        float w_p = 1/tau;
	float w_pp = tan(w_p / (48000.0*2));

        float a1 = (w_pp - 1)/(w_pp + 1);
        float b0 = w_pp/(1 + w_pp);
        float b1 = b0;

	std::vector<double> btaps(2);// = {b0, b1};
	std::vector<double> ataps(2);// = {1, a1};
	
	btaps[0] = b0;
	btaps[1] = b1;
	ataps[0] = 1;
	ataps[1] = a1;

	deemph = gr::filter::iir_filter_ffd::make(btaps,ataps);

	audio_resampler_taps = design_filter(1, 6);
	decim_audio = gr::filter::fir_filter_fff::make(6, audio_resampler_taps); //downsample from 48k to 8k


/*
    FM Deemphasis IIR filter.
            fs: sampling frequency in Hz (float)
            tau: Time constant in seconds (75us in US, 50us in EUR) (float)

	//tau = 0.000075
        w_p = 1/tau
        w_pp = math.tan(w_p / (fs * 2))  # prewarped analog freq

        a1 = (w_pp - 1)/(w_pp + 1)
        b0 = w_pp/(1 + w_pp)
        b1 = b0

        btaps = [b0, b1]
        ataps = [1, a1]

        if 0:
            print "btaps =", btaps
            print "ataps =", ataps
            global plot1
            plot1 = gru.gnuplot_freqz(gru.freqz(btaps, ataps), fs, True)

        deemph = filter.iir_filter_ffd(btaps, ataps)
        self.connect(self, deemph, self)

*/

/*

        # FM Demodulator  input: complex; output: float
        k = quad_rate/(2*math.pi*max_dev)
        self.quad_demod = analog.quadrature_demod_cf(k)

        # FM Deemphasis IIR filter
        self.deemph = fm_deemph(quad_rate, tau=tau)

        # compute FIR taps for audio filter
        audio_decim = quad_rate // audio_rate
        audio_taps = filter.firdes.low_pass(1.0,            # gain
                                            quad_rate,      # sampling rate
                                            2.7e3,          # Audio LPF cutoff
                                            0.5e3,          # Transition band
                                            filter.firdes.WIN_HAMMING)  # filter type

        print "len(audio_taps) =", len(audio_taps)

        # Decimating audio filter
        # input: float; output: float; taps: float
        self.audio_filter = filter.fir_filter_fff(audio_decim, audio_taps)

        self.connect(self, self.quad_demod, self.deemph, self.audio_filter, self)
*/

//	for (int i=0; i < samp_per_sym; i++) {
//		sym_taps.push_back(1.0 / samp_per_sym);
//	}
//	sym_filter = gr::filter::fir_filter_fff::make(1, sym_taps);
	/*
	if (!logging) {
	iam_logging = true;
	logging = true;
	std::cout << "I am the one true only!" << std::endl;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,1,1, false, num);
	} else {
	iam_logging = false;
	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,0,0, false, num);
	}*/
	
	iam_logging = false;
//	dsd = dsd_make_block_ff(dsd_FRAME_P25_PHASE_1,dsd_MOD_C4FM,3,0,0, false, num);

	tm *ltm = localtime(&starttime);

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,freq);
	sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,timestamp,freq);
	wav_sink = gr::blocks::wavfile_sink::make(filename,1,8000,16);

	sprintf(raw_filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
	sprintf(debug_filename, "%s/%ld-%ld_%g_debug.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
	raw_sink = gr::blocks::file_sink::make(sizeof(gr_complex), raw_filename);
//	debug_sink = gr::blocks::file_sink::make(sizeof(float), debug_filename);


	connect(self(),0, valve,0);
	connect(valve,0, prefilter,0);
	connect(prefilter, 0, downsample_sig, 0);
	connect(downsample_sig,0, raw_sink,0);	
	connect(downsample_sig, 0, demod, 0);
	connect(demod, 0, deemph, 0); 
	connect(deemph, 0, decim_audio, 0); 
	connect(decim_audio, 0, wav_sink, 0); 


}

log_dsd::~log_dsd() {

}
// from: /gnuradio/grc/grc_gnuradio/blks2/selector.py
void log_dsd::unmute() {
	// this function gets called everytime their is a TG continuation command. This keeps the timestamp updated.
	//std::cout<< "logging_receiver_dsd.cc: Refreshing Logger [ " << num << " ] - Elapsed[ " << time(NULL) - timestamp << "]  " << std::endl; 

	timestamp = time(NULL);

}

void log_dsd::mute() {


}

bool log_dsd::is_active() {
	return active;
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

int log_dsd::lastupdate() {
	return time(NULL) - timestamp;
}

long log_dsd::elapsed() {
	return time(NULL) - starttime;
}


void log_dsd::tune_offset(float f) {
	freq = f;
	int offset_amount = ((f*1000000) - center);
	prefilter->set_center_freq(offset_amount); // have to flip this for 3.7
	//std::cout << "Offset set to: " << offset_amount << " Freq: "  << freq << std::endl;
}

void log_dsd::deactivate() {
	//std::cout<< "logging_receiver_dsd.cc: Deactivating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ] " << std::endl; 

	active = false;

	valve->set_enabled(false);

	wav_sink->close();
	
	raw_sink->close();
//	debug_sink->close();


	ofstream myfile (status_filename);
	if (myfile.is_open())
	  {
	    myfile << "{\n";
	    myfile << "\"freq\": " << freq << ",\n";
	    myfile << "\"num\": " << num << ",\n";
	    myfile << "\"talkgroup\": " << talkgroup << "\n";
	    myfile << "}\n";
	    myfile.close();
	  }
	else cout << "Unable to open file";
}

void log_dsd::activate(float f, int t, int n) {

	timestamp = time(NULL);
	starttime = time(NULL);

	talkgroup = t;
	freq = f;
        //num = n;
  	tm *ltm = localtime(&starttime);
  	//std::cout<< "logging_receiver_dsd.cc: Activating Logger [ " << num << " ] - freq[ " << freq << "] \t talkgroup[ " << talkgroup << " ]  "  <<std::endl;

  	//if (logging) {
	prefilter->set_center_freq( (f*1000000) - center); // have to flip for 3.7

	if (iam_logging) {
		//printf("Recording Freq: %f \n", f);
	}

	std::stringstream path_stream;
	path_stream << boost::filesystem::current_path().string() <<  "/" << 1900 + ltm->tm_year << "/" << 1 + ltm->tm_mon << "/" << ltm->tm_mday;

	boost::filesystem::create_directories(path_stream.str());
	sprintf(filename, "%s/%ld-%ld_%g.wav", path_stream.str().c_str(),talkgroup,timestamp,f);
	sprintf(status_filename, "%s/%ld-%ld_%g.json", path_stream.str().c_str(),talkgroup,timestamp,freq);
	sprintf(raw_filename, "%s/%ld-%ld_%g.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
	sprintf(debug_filename, "%s/%ld-%ld_%g_debug.raw", path_stream.str().c_str(),talkgroup,timestamp,freq);
	
	raw_sink->open(raw_filename);
//	debug_sink->open(debug_filename);
	wav_sink->open(filename);

	//}
	active = true;
	valve->set_enabled(true);

}
