#include <gnuradio/gr_file_sink.h>
#include <gnuradio/gr_sig_source_f.h>
#include <gnuradio/gr_hier_block2.h>
#include <gnuradio/gr_io_signature.h>
#include <gnuradio/gr_top_block.h>

static void connect(gr_top_block_sptr block, gr_sig_source_f_sptr 
source,
                    gr_hier_block2_sptr block2) {
  fprintf(stderr, "connect: calling lock, connect, unlock\n");
  block->lock();
  block->connect(source, 0, block2, 0);
  block->unlock();
  fprintf(stderr, "connect: done\n");
}

static void disconnect(gr_top_block_sptr block, gr_sig_source_f_sptr 
source,
                       gr_hier_block2_sptr block2) {
  fprintf(stderr, "disconnect: calling block->lock\n");
  block->lock();

  fprintf(stderr, "disconnect: calling block->disconnect\n");
  block->disconnect(source, 0, block2, 0);

  fprintf(stderr, "disconnect: calling block->unlock\n");
  block->unlock();                  // It usually hangs here.

  fprintf(stderr, "disconnect: done\n");
}

int main(int argc, char** argv) {
  // Inner block: block to sink.
  gr_hier_block2_sptr inner;
  inner = gr_make_hier_block2("inner",
                              gr_make_io_signature(1, 1, sizeof(float)),
                              gr_make_io_signature(0, 0, 0));

  gr_file_sink_sptr sink;
  sink = gr_make_file_sink(sizeof(float), "/dev/null");
  inner->connect(inner, 0, sink, 0);

  // Outer block: signal source to inner block.
  gr_top_block_sptr outer = gr_make_top_block("outer");
  gr_sig_source_f_sptr src = gr_make_sig_source_f(11025, GR_COS_WAVE,
                                                  400, .1, 0);

  // Hook it up and get it going.
  connect(outer, src, inner);
  outer->start();

  // Frob it until we die.
  while (true) {
    disconnect(outer, src, inner);
    fprintf(stderr, "\n\n------------------------\n\n");

    connect(outer, src, inner);
  }

  return 0;
}
