/*
 * Copyright 2011 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */


/*
 * GNU Radio C++ example creating dial tone
 * ("the simplest thing that could possibly work")
 *
 * Send a tone each to the left and right channels of stereo audio
 * output and let the user's brain sum them.
 *
 * GNU Radio makes extensive use of Boost shared pointers.  Signal processing
 * blocks are typically created by calling a "make" factory function, which
 * returns an instance of the block as a typedef'd shared pointer that can
 * be used in any way a regular pointer can.  Shared pointers created this way
 * keep track of their memory and free it at the right time, so the user
 * doesn't need to worry about it (really).
 *
 */

// Include header files for each block used in flowgraph

#include <iostream>
#include <string> 
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "logging_receiver_dsd.h"
#include "logging_receiver_pocsag.h"
//#include "logging_receiver_p25.h"
#include "smartnet_crc.h"
#include "smartnet_deinterleave.h"

#include <osmosdr/source.h>
#include <osmosdr/sink.h>

#include <boost/program_options.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/intrusive_ptr.hpp>


#include <gnuradio/msg_queue.h>
#include <gnuradio/message.h>
#include <gnuradio/blocks/file_sink.h>
#include <gnuradio/gr_complex.h>

#include <gnuradio/top_block.h>
#include <gnuradio/blocks/multiply_cc.h>

#include <gnuradio/filter/freq_xlating_fir_filter_ccf.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/firdes.h>
#include <gnuradio/filter/fir_filter_ccf.h>
#include <gnuradio/filter/fir_filter_ccf.h>

#include <gnuradio/digital/fll_band_edge_cc.h>
#include <gnuradio/digital/clock_recovery_mm_ff.h>
#include <gnuradio/digital/binary_slicer_fb.h>
#include <gnuradio/digital/correlate_access_code_tag_bb.h>



#include <gnuradio/analog/pll_freqdet_cf.h>
#include <gnuradio/analog/sig_source_f.h>
#include <gnuradio/analog/sig_source_c.h>







namespace po = boost::program_options;

using namespace std;


   



int lastcmd = 0;
int thread_num=0;
double center_freq;

gr::top_block_sptr tb;
osmosdr::source::sptr src;

/* static loggers
vector<log_dsd_sptr> loggers;*/

vector<log_dsd_sptr> active_loggers;
vector<log_pocsag_sptr> active_pocsags;

//vector<log_p25_sptr> active_loggers;

volatile sig_atomic_t exit_flag = 0;
void exit_interupt(int sig){ // can be called asynchronously
  exit_flag = 1; // set flag
}

void init_loggers(int num, float center_freq) {

/* static loggers
	for (int i = 0; i < num; i++) {
		log_dsd_sptr log = make_log_dsd( center_freq, center_freq, 0, i);			
		loggers.push_back(log);
 	}
*/
}

float getfreq(int cmd) {
	float freq;
		if (cmd < 0x1b8) {	
			freq = float(cmd * 0.025 + 851.0125);
		} else if (cmd < 0x230) {
			freq = float(cmd * 0.025 + 851.0125 - 10.9875);
			} else {
			freq = 0;
			}
	
	return freq;
}
void parse_status(int command, int address, int groupflag) {
	    int Value = address << 1 | (groupflag ? (1) : 0);
            int GroupTimeout = Value & (0x1f);
            Value >>= 5;
            int ConnecTimeout = Value & (0x1f);
            Value >>= 5;
            int DispatchTimeout = Value & 0xf;
            Value >>= 4;
            int Power = Value & 1;
            Value >>= 1;
            int OpCode = Value;
            cout << "Status : " << OpCode << "\tPower: " << Power << "\tDispatchTimeout: " << DispatchTimeout << "\tConnectTimeOut: " << ConnecTimeout << "\tGroupTimeOut:" << GroupTimeout <<endl;
}
float parse_message(string s) {
	float retfreq = 0;
	bool rxfound = false;
	std::vector<std::string> x;
	boost::split(x, s, boost::is_any_of(","), boost::token_compress_on);
	//vector<string> x = split(s, ","); 
	int address = atoi( x[0].c_str() ) & 0xFFF0;
	int command = atoi( x[2].c_str() );
	char shell_command[200];
	    

        if (command < 0x2d0) {

		if ( lastcmd == 0x308) {
		        // Channel Grant
			if (  (address != 56016) && (address != 8176)) {
				retfreq = getfreq(command);
			}
		} else {
			// Call continuation
			if  ( (address != 56016) && (address != 8176))  {  //(address != 56016) &&
				retfreq = getfreq(command);
			}
		}
	}

	if (command == 0x03c0) {
		//parse_status(command, address, groupflag);
	}
        
	if (retfreq) {
		for(vector<log_dsd_sptr>::iterator it = active_loggers.begin(); it != active_loggers.end(); ++it) {	
			log_dsd_sptr rx = *it;	
		/*for(vector<log_p25_sptr>::iterator it = active_loggers.begin(); it != active_loggers.end(); ++it) {		
		
			log_p25_sptr rx = *it;
		*/	
						
			if (rx->get_talkgroup() == address) {		
				if (rx->get_freq() != retfreq) {
					rx->tune_offset(retfreq);
				}
				rx->unmute();
				
				rxfound = true;
			} else {
				if (rx->get_freq() == retfreq) {
					
					cout << "  !! Someone else is on my Channel - My TG: "<< rx->get_talkgroup() << " Freq: " <<rx->get_freq() << " Intruding TG: " << address << endl;
					rx->mute();
					
				}
			}
		}
		for(vector<log_pocsag_sptr>::iterator it = active_pocsags.begin(); it != active_pocsags.end(); ++it) {	
			log_pocsag_sptr rx = *it;	
		/*for(vector<log_p25_sptr>::iterator it = active_loggers.begin(); it != active_loggers.end(); ++it) {		
		
			log_p25_sptr rx = *it;
		*/	
						
			if (rx->get_talkgroup() == address) {		
				if (rx->get_freq() != retfreq) {
					rx->tune_offset(retfreq);
				}
				rx->unmute();
				
				rxfound = true;
			} else {
				if (rx->get_freq() == retfreq) {
					
					cout << "  !! Someone else is on my Channel - My TG: "<< rx->get_talkgroup() << " Freq: " <<rx->get_freq() << " Intruding TG: " << address << endl;
					rx->mute();
					
				}
			}
		}

		if ((!rxfound)){ 
			//cout << "smartnet.cc: Activating Logger - TG: " << address << "\t Freq: " << retfreq << "\tCmd: " <<command << "\t LastCmd: " <<lastcmd << "\t  Flag: "<< groupflag << endl;

			/* static loggers			
			log_dsd_sptr log = loggers.front();
			active_loggers.push_back(move(log));
			loggers.erase(loggers.begin());*/
			//cout << "smartnet.cc: Moved Logger, Loggers " << loggers.size() << " Active Loggers " << active_loggers.size() << endl;
			
					
			
			tb->lock();
			//tb->stop();
			//tb->wait();

			// Dynamic Logger	
			if (address != 56016) {		
				log_dsd_sptr log = make_log_dsd( retfreq, center_freq, address, thread_num++);			
				//log_p25_sptr log = make_log_p25( retfreq, center_freq, address);			
							
				active_loggers.push_back(log);

				tb->connect(src, 0, log, 0);

				/* static loggers
				log->activate(retfreq, address);
				*/
			} else {
				log_pocsag_sptr log = make_log_pocsag( retfreq, center_freq, address, thread_num++);			
				//log_p25_sptr log = make_log_p25( retfreq, center_freq, address);			
							
				active_pocsags.push_back(log);

				tb->connect(src, 0, log, 0);
			}


				tb->unlock();
			
			//tb->start();

			//cout << "smartnet.cc: Activated logger & unlocked" << endl;
		}
		
		//cout << "TG: " << address << "\tFreq: " << retfreq << "\tActive Loggers: " << active_loggers.size() << "\tCmd: "<< command << "\t LastCmd: " <<lastcmd   << endl;
	}


	for(vector<log_dsd_sptr>::iterator it = active_loggers.begin(); it != active_loggers.end();) {
		log_dsd_sptr rx = *it;
/*	
	for(vector<log_p25_sptr>::iterator it = active_loggers.begin(); it != active_loggers.end();) {
		log_p25_sptr rx = *it;
*/
		if (rx->timeout() > 5.0) {
			//cout << "smartnet.cc: Deleting Logger - TG: " << rx->get_talkgroup() << "\t Freq: " << rx->get_freq() << endl;
			
			tb->lock();
			//tb->stop();
			//tb->wait();

			tb->disconnect(src, 0, rx, 0);
			
			/* !!!!!!!!!!!!! don't forget to un comment this for log_dsd */
			rx->deactivate();
			//rx->close();

			tb->unlock();
			
			//tb->start();
			
						
			
			

			
			//cout << "smartnet.cc: Moved Active Logger, Loggers " << loggers.size() << " Active Loggers " << active_loggers.size() << endl;
			
			sprintf(shell_command,"./encode-upload.sh %s &", rx->get_filename());
			system(shell_command);

			/* static loggers
			loggers.push_back(move(rx));
			*/

			it = active_loggers.erase(it);
			

		} else {
			++it;
		}
	}
	for(vector<log_pocsag_sptr>::iterator it = active_pocsags.begin(); it != active_pocsags.end();) {
		log_pocsag_sptr rx = *it;

		if (rx->timeout() > 5.0) {
			
			tb->lock();

			tb->disconnect(src, 0, rx, 0);
			
			rx->deactivate();

			tb->unlock();
						
//			sprintf(shell_command,"./encode-upload.sh %s &", rx->get_filename());
//			system(shell_command);

			it = active_pocsags.erase(it);
			

		} else {
			++it;
		}
	}

	lastcmd = command;
       

	return retfreq;
}


int main(int argc, char **argv)
{

std::string device_addr;
    double  samp_rate, chan_freq, error;
	int if_gain, bb_gain, rf_gain;
    //setup the program options
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("arg", po::value<std::string>(&device_addr)->default_value(""), "the device arguments in string format")
        ("rate", po::value<double>(&samp_rate)->default_value(1e6), "the sample rate in samples per second")
        ("center", po::value<double>(&center_freq)->default_value(10e6), "the center frequency in Hz")
	("error", po::value<double>(&error)->default_value(0), "the Error in frequency in Hz")
	("freq", po::value<double>(&chan_freq)->default_value(10e6), "the frequency in Hz of the trunking channel")
        ("rfgain", po::value<int>(&rf_gain)->default_value(14), "RF Gain")
	("bbgain", po::value<int>(&bb_gain)->default_value(25), "BB Gain")
	("ifgain", po::value<int>(&if_gain)->default_value(25), "IF Gain")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //print the help message
    if (vm.count("help")){
        std::cout
            << boost::format("SmartNet Trunking Reciever %s") % desc << std::endl
            << "The tags sink demo block will print USRP source time stamps." << std::endl
            << "The tags source demo block will send bursts to the USRP sink." << std::endl
            << "Look at the USRP output on a scope to see the timed bursts." << std::endl
            << std::endl;
        return ~0;
    }




 signal(SIGINT, exit_interupt);
 	tb = gr::make_top_block("Smartnet");

	src = osmosdr::source::make();

	cout << "Setting sample rate to: " << samp_rate << endl;
	src->set_sample_rate(samp_rate);
	cout << "Tunning to " << center_freq - error << "hz" << endl;
	src->set_center_freq(center_freq - error,0);

	cout << "Setting RF gain to " << rf_gain << endl;
	cout << "Setting BB gain to " << bb_gain << endl;
	cout << "Setting IF gain to " << if_gain << endl;

	src->set_gain(rf_gain);
	src->set_if_gain(if_gain);
	src->set_bb_gain(bb_gain);




	float samples_per_second = samp_rate;
	float syms_per_sec = 3600;
	float gain_mu = 0.01;
	float mu=0.5;
	float omega_relative_limit = 0.3;
	float offset = center_freq - chan_freq;
	float clockrec_oversample = 3;
	int decim = int(samples_per_second / (syms_per_sec * clockrec_oversample));
	float sps = samples_per_second/decim/syms_per_sec; 
	const double pi = boost::math::constants::pi<double>();
	
	/*cout << "Control channel offset: " << offset << endl;
	cout << "Decim: " << decim << endl;
	cout << "Samples per symbol: " << sps << endl;
*/

	init_loggers(10, center_freq);
	gr::msg_queue::sptr queue = gr::msg_queue::make();


	gr::analog::sig_source_c::sptr offset_sig = gr::analog::sig_source_c::make(samp_rate, gr::analog::GR_SIN_WAVE, offset, 1.0, 0.0);

	gr::blocks::multiply_cc::sptr mixer = gr::blocks::multiply_cc::make();
	
	gr::filter::fir_filter_ccf::sptr downsample = gr::filter::fir_filter_ccf::make(decim, gr::filter::firdes::low_pass(1, samples_per_second, 10000, 5000, gr::filter::firdes::WIN_HANN));

	/*prefilter = gr_make_freq_xlating_fir_filter_ccf(decim, 
						       gr_firdes::low_pass(1, samp_rate, xlate_bandwidth/2, 6000),
						       offset, 
						       samp_rate);*/

	//gr::filter::freq_xlating_fir_filter_ccf::sptr downsample = gr::filter::freq_xlating_fir_filter_ccf::make(decim, gr::filter::firdes::low_pass(1, samples_per_second, 10000, 1000, gr::filter::firdes::WIN_HANN), 0,samples_per_second);

	gr::analog::pll_freqdet_cf::sptr pll_demod = gr::analog::pll_freqdet_cf::make(2.0 / clockrec_oversample, 										 2*pi/clockrec_oversample, 
										-2*pi/clockrec_oversample);

	gr::digital::fll_band_edge_cc::sptr carriertrack = gr::digital::fll_band_edge_cc::make(sps, 0.6, 64, 1.0);

	gr::digital::clock_recovery_mm_ff::sptr softbits = gr::digital::clock_recovery_mm_ff::make(sps, 0.25 * gain_mu * gain_mu, mu, gain_mu, omega_relative_limit); 


	gr::digital::binary_slicer_fb::sptr slicer =  gr::digital::binary_slicer_fb::make();
	gr::digital::correlate_access_code_tag_bb::sptr start_correlator = gr::digital::correlate_access_code_tag_bb::make("10101100",0,"smartnet_preamble");


	smartnet_deinterleave_sptr deinterleave = smartnet_make_deinterleave();

	smartnet_crc_sptr crc = smartnet_make_crc(queue);



		tb->connect(offset_sig, 0, mixer, 0);
	tb->connect(src, 0, mixer, 1);
	tb->connect(mixer, 0, downsample, 0);
	tb->connect(downsample, 0, carriertrack, 0);
	tb->connect(carriertrack, 0, pll_demod, 0);
	tb->connect(pll_demod, 0, softbits, 0);
	tb->connect(softbits, 0, slicer, 0);
	tb->connect(slicer, 0, start_correlator, 0);
	tb->connect(start_correlator, 0, deinterleave, 0);
	tb->connect(deinterleave, 0, crc, 0);

	

	tb->start();
/*
for (int i=0; i < 8; i++ ){
			tb->lock();

			// Dynamic Logger			
			log_dsd_sptr log = make_log_dsd( 856.6, 856.6, 1616, 0);			
			

			tb->connect(src, 0, log, 0);

			tb->unlock();
active_loggers.push_back(log);
usleep(1000);
}


			usleep(1000*1000*30);
	for(vector<log_dsd_sptr>::iterator it = active_loggers.begin(); it != active_loggers.end();) {
		log_dsd_sptr rx = *it;
			tb->lock();
			tb->disconnect(src, 0, rx, 0);
			rx->deactivate();
			tb->unlock();
			
it = active_loggers.erase(it);
}*/
	while (1) {
		if(exit_flag){ // my action when signal set it 1
       			printf("\n Signal caught!\n");
			tb->stop();
			return 0;
		} 
		if (!queue->empty_p())
		{
			std::string sentence;
			gr::message::sptr msg;
			msg = queue->delete_head();
			sentence = msg->to_string();
			parse_message(sentence);	
			
		} else {
			
			boost::this_thread::sleep(boost::posix_time::milliseconds(1.0/10));
		}

	}
	
  

  // Exit normally.
  return 0;
}
