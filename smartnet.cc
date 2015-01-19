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
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ncurses.h>
#include <menu.h>
#include "logging_receiver_dsd.h"
#include "smartnet_crc.h"
#include "smartnet_deinterleave.h"
#include "talkgroup.h"

#include <osmosdr/source.h>
#include <osmosdr/sink.h>

#include <boost/program_options.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/tokenizer.hpp>
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
 long lastaddress = 0;
 int thread_num=0;
 double center_freq;
bool console  = false;





 vector<log_dsd_sptr> loggers;
 unsigned int max_loggers = 0;
 unsigned int num_loggers = 0;
 vector<log_dsd_sptr> active_loggers;

gr::top_block_sptr tb;
osmosdr::source::sptr src;

 vector<Talkgroup *> talkgroups;
 vector<Talkgroup *> active_tg;
 char **menu_choices;
 char status[150];
 ITEM **tg_menu_items;


 WINDOW *active_tg_win;
 WINDOW *tg_menu_win;
 WINDOW *status_win;
 MENU *tg_menu;

 time_t lastMsgCountTime = time(NULL);
 time_t lastTalkgroupPurge = time(NULL);
int messagesDecodedSinceLastReport = 0;
volatile float msgs_decoded_per_second = 0;


 volatile sig_atomic_t exit_flag = 0;


 void update_active_tg_win() {
 	werase(active_tg_win);
 	box(active_tg_win, 0, 0);
 	int i=0;
 	for(std::vector<Talkgroup *>::iterator it = active_tg.begin(); it != active_tg.end(); ++it) {
 		Talkgroup *tg = (Talkgroup *) *it;
		/*s = tg->menu_string();
		c = (char *) malloc((s.size() + 1) * sizeof(char));
		//strncpy(c, s.c_str(), s.size());
    		//c[s.size()] = '\0';
		strcpy(c, s.c_str());
		*/

		mvwprintw(active_tg_win,i*2+1,2,"TG: %s", tg->alpha_tag.c_str());
		mvwprintw(active_tg_win,i*2+2,6,"%s ", tg->description.c_str());
		mvwprintw(active_tg_win,i*2+1,40,"Num: %5lu", tg->number);
		mvwprintw(active_tg_win,i*2+2,40,"Tag: %s", tg->tag.c_str());
		mvwprintw(active_tg_win,i*2+2,60,"Group: %s", tg->group.c_str());

		i++;
	}

	wrefresh(active_tg_win);

}
void update_status_win(char *c) {
	wclear(status_win);
	//wattron(status_win,A_REVERSE);
	mvwprintw(status_win,0,2,"%s",c);
	wrefresh(status_win);
}
void create_status_win() {
	int startx, starty, width, height;

	height = 1;
	width = COLS;
	starty = LINES-1;
	startx = 0;

	status_win = newwin(height, width, starty, startx);
}
void create_active_tg_win() {
	int startx, starty, width, height;

	height = 20;
	width = COLS;
	starty = 0;
	startx = 0;

	active_tg_win = newwin(height, width, starty, startx);
	box(active_tg_win, 0, 0);

	wrefresh(active_tg_win);
}

void create_tg_menu() {
	std::string s;
	int n_choices, i;
	char *c;

       	//printw("%s\n", s.c_str());
	menu_choices = new char*[talkgroups.size()];
	//menu_choices = malloc(talkgroups.size(), sizeof(char *));
	i=0;
	for(std::vector<Talkgroup *>::iterator it = talkgroups.begin(); it != talkgroups.end(); ++it) {

		Talkgroup *tg = (Talkgroup *) *it;
		s = tg->menu_string();
		c = (char *) malloc((s.size() + 1) * sizeof(char));
		//strncpy(c, s.c_str(), s.size());
    		//c[s.size()] = '\0';
		strcpy(c, s.c_str());
		menu_choices[i] = c;
		i++;
	}

	n_choices = talkgroups.size(); //ARRAY_SIZE(menu_choices);
	tg_menu_items = (ITEM **) calloc(n_choices + 1, sizeof(ITEM *));


	for (i=0; i < n_choices; ++i) {
		tg_menu_items[i] = new_item(menu_choices[i], menu_choices[i]);
		set_item_userptr(tg_menu_items[i], (void *) talkgroups[i]);
	}

	tg_menu = new_menu((ITEM **) tg_menu_items);

	tg_menu_win = newwin(LINES - 11, COLS, 10, 0);
	keypad(tg_menu_win, TRUE);


	set_menu_win(tg_menu, tg_menu_win);
	set_menu_sub(tg_menu, derwin(tg_menu_win, LINES - 15, COLS - 4, 2, 2));
	set_menu_format(tg_menu, LINES - 14 , 1);
	//set_menu_mark(tg_menu, " * ");
	box(tg_menu_win,0,0);
	menu_opts_off(tg_menu, O_SHOWDESC | O_ONEVALUE);
	//menu_opts_off(tg_menu, O_ONEVALUE);

	post_menu(tg_menu);

}







void exit_interupt(int sig){ // can be called asynchronously
  exit_flag = 1; // set flag
}

void init_loggers(int num, float center_freq, long samp_rate) {

// static loggers
	for (int i = 0; i < num; i++) {
		log_dsd_sptr log = make_log_dsd( center_freq, center_freq, samp_rate, 0, i);

		loggers.push_back(log);
		tb->connect(src, 0, log, 0);
	}

}


void parse_file(string filename) {
	ifstream in(filename.c_str());
	if (!in.is_open()) return;

	boost::char_separator<char> sep(",");
	typedef boost::tokenizer< boost::char_separator<char> > t_tokenizer;

	vector< string > vec;
	string line;

	while (getline(in,line))
	{

		t_tokenizer tok(line, sep);
	
		vec.assign(tok.begin(),tok.end());
		if (vec.size() < 8) continue;

		Talkgroup *tg = new Talkgroup(atoi( vec[0].c_str()), vec[2].at(0),vec[3].c_str(),vec[4].c_str(),vec[5].c_str() ,vec[6].c_str(),atoi(vec[7].c_str()) );

		talkgroups.push_back(tg);
	}
}





int main(int argc, char **argv)
{
	unsigned int last_crc_error_count=0;
	time_t currentTime = time(NULL);
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
	parse_file("ChanList.csv");

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
	float offset = chan_freq - center_freq;  // have to reverse it for 3.7 because it swapped in the switch.
	float clockrec_oversample = 3;
	int decim = int(samples_per_second / (syms_per_sec * clockrec_oversample));
	float sps = samples_per_second/decim/syms_per_sec;
	const double pi = boost::math::constants::pi<double>();

	cout << "Control channel offset: " << offset << endl;
	cout << "Decim: " << decim << endl;
	cout << "Samples per symbol: " << sps << endl;

  std::vector<float> lpf_taps;

  //init_loggers(max_loggers, center_freq, samp_rate);
  
  gr::msg_queue::sptr queue = gr::msg_queue::make();

	lpf_taps =  gr::filter::firdes::low_pass(1, samp_rate, 4500, 2000);

	gr::filter::freq_xlating_fir_filter_ccf::sptr prefilter = gr::filter::freq_xlating_fir_filter_ccf::make(decim,
						       lpf_taps,
						       offset,
						       samp_rate);

	gr::digital::fll_band_edge_cc::sptr carriertrack = gr::digital::fll_band_edge_cc::make(sps, 0.6, 64, 0.35);

	gr::analog::pll_freqdet_cf::sptr pll_demod = gr::analog::pll_freqdet_cf::make(2.0 / clockrec_oversample, 1*pi/clockrec_oversample, -1*pi/clockrec_oversample);

	gr::digital::clock_recovery_mm_ff::sptr softbits = gr::digital::clock_recovery_mm_ff::make(sps, 0.25 * gain_mu * gain_mu, mu, gain_mu, omega_relative_limit);

	gr::digital::binary_slicer_fb::sptr slicer =  gr::digital::binary_slicer_fb::make();

	gr::digital::correlate_access_code_tag_bb::sptr start_correlator = gr::digital::correlate_access_code_tag_bb::make("10101100",0,"smartnet_preamble");

	smartnet_deinterleave_sptr deinterleave = smartnet_make_deinterleave();

	smartnet_crc_sptr crc = smartnet_make_crc(queue);

	tb->connect(src,0,prefilter,0);
	tb->connect(prefilter,0,carriertrack,0);
	tb->connect(carriertrack, 0, pll_demod, 0);
	tb->connect(pll_demod, 0, softbits, 0);
	tb->connect(softbits, 0, slicer, 0);
	tb->connect(slicer, 0, start_correlator, 0);
	tb->connect(start_correlator, 0, deinterleave, 0);
	tb->connect(deinterleave, 0, crc, 0);

	tb->start();

	parse_file("ChanList.csv");
	if (console) {
		initscr();
		cbreak();
		noecho();
		nodelay(active_tg_win,TRUE);


		create_active_tg_win();
		create_status_win();
	}


	gr::message::sptr msg;
	while (1) {
		if(exit_flag){ // my action when signal set it 1
			printf("\n Signal caught!\n");
			tb->stop();
			endwin();
			return 0;
		}


	msg = queue->delete_head();
	std::cout << msg->to_string() << std:endl;
	currentTime = time(NULL);
	if ((currentTime - lastTalkgroupPurge) >= 1.0 )
	{	
		//stop_inactive_loggers();
		lastTalkgroupPurge = currentTime;
	}
	messagesDecodedSinceLastReport++;
		
   float timeDiff = currentTime - lastMsgCountTime;
	if (timeDiff >= 3.0) {
		msgs_decoded_per_second = messagesDecodedSinceLastReport/timeDiff; 
		messagesDecodedSinceLastReport = 0;
		lastMsgCountTime = currentTime;
		if (!console) {
			std::cout << "Control Channel Message Decode Rate: " << msgs_decoded_per_second << "/sec" << std::endl;
		}
	}
	msg.reset();
	
	
	}

	endwin();

  // Exit normally.
	return 0;
}
