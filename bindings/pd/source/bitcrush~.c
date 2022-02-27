//
// Created by binabik on 18/09/2020.
//

#include "../include/bitcrush~.h"

#include <math.h>

/*** struct ***/

static t_class *bitcrush_tilde_class;

static const int DEFAULT_BIT_DEPTH = 8;
static const t_float DEFAULT_CRUSH = 0;

typedef struct {
  t_object x_obj;
  t_sample f;

  // inlet
  // no need to manage first inlet (signal)
  t_inlet *x_f_in_crush;
  t_inlet *x_i_in_bit_depth;

  // outlet
  t_outlet *x_s_out;

  // internal
  t_int i_bit_depth;
  t_float f_crush;

} t_bitcrush_tilde;

/*** utility ***/

void set_crush(t_bitcrush_tilde *x, t_floatarg crush_arg) {
  x->f_crush = crush_arg < 0. ? 0 : crush_arg > 1. ? 1. : crush_arg;
}

void set_bit_depth(t_bitcrush_tilde *x, t_floatarg bit_depth_arg) {
  x->i_bit_depth = bit_depth_arg < 1.    ? 1
                   : bit_depth_arg > 32. ? 32
                                         : (int)round(bit_depth_arg);
}

/*** implementation ***/

void *bitcrush_tilde_new(t_floatarg bit_depth_arg, t_floatarg crush_arg) {
  t_bitcrush_tilde *x = (t_bitcrush_tilde *)pd_new(bitcrush_tilde_class);

  set_bit_depth(x, bit_depth_arg ? bit_depth_arg : DEFAULT_BIT_DEPTH);
  set_crush(x, crush_arg ? crush_arg : DEFAULT_CRUSH);

  // inlet
  x->x_i_in_bit_depth =
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("bit_depth"));
  x->x_f_in_crush =
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("crush"));

  // outlet
  x->x_s_out = outlet_new(&x->x_obj, &s_signal);

  return (void *)x;
}

void bitcrush_tilde_free(t_bitcrush_tilde *x) {
  outlet_free(x->x_s_out);
  inlet_free(x->x_f_in_crush);
  inlet_free(x->x_i_in_bit_depth);
}

t_int *bitcrush_tilde_perform(t_int *w) {
  t_bitcrush_tilde *x = (t_bitcrush_tilde *)(w[1]);
  int block_size = (int)(w[2]);
  t_sample *in = (t_sample *)(w[3]);
  t_sample *out = (t_sample *)(w[4]);

  // assuming signal in [-1, 1]
  const t_sample sig_min = -1;
  const t_sample sig_max = 1;
  // in order to get to [0, 1]
  const t_sample sig_amplitude = sig_max - sig_min;
  // DON'T forget long 'cause it might overflow.
  const long int down = 1 << (x->i_bit_depth - 1);
  t_sample stored = 0;
  t_float acc = 0;
  // shameful copy-paste from decimate~ from sigpack.
  while (--block_size) {
    acc += x->f_crush;
    const t_sample cache = *in++;
    if (acc >= 1.) {
      acc -= 1.;
      stored = cache;
      // normalising to [0, 1];
      // stored /= sig_amplitude;
      // processing
      // DO NOT USE floor(), for some reason it breaks signal
      stored = (t_sample)(int)(stored * down) / (t_sample)down;
      // back to [sig_min, sig_max]
      // stored *= sig_amplitude;
    }
    *out++ = stored;
  }
  return (t_int *)(w + 5);
}

void bitcrush_tilde_dsp(t_bitcrush_tilde *x, t_signal **sp) {
  dsp_add(bitcrush_tilde_perform, 4, x, sp[0]->s_n, sp[0]->s_vec, sp[1]->s_vec);
}

void bitcrush_tilde_crush(t_bitcrush_tilde *x, t_floatarg crush_arg) {
  set_crush(x, crush_arg);
}

void bitcrush_tilde_bit_depth(t_bitcrush_tilde *x, t_floatarg bit_depth_arg) {
  set_bit_depth(x, bit_depth_arg);
}

void bitcrush_tilde_setup(void) {
  bitcrush_tilde_class =
      class_new(gensym("bitcrush~"), (t_newmethod)bitcrush_tilde_new,
                (t_method)bitcrush_tilde_free, sizeof(t_bitcrush_tilde),
                CLASS_DEFAULT, A_DEFFLOAT, A_DEFFLOAT, 0);
  class_addmethod(bitcrush_tilde_class, (t_method)bitcrush_tilde_dsp,
                  gensym("dsp"), A_CANT, 0);
  class_addmethod(bitcrush_tilde_class, (t_method)bitcrush_tilde_crush,
                  gensym("crush"), A_DEFFLOAT, 0);
  class_addmethod(bitcrush_tilde_class, (t_method)bitcrush_tilde_bit_depth,
                  gensym("bit_depth"), A_DEFFLOAT, 0);
  CLASS_MAINSIGNALIN(bitcrush_tilde_class, t_bitcrush_tilde, f);
}