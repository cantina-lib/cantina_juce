//
// Created by piptouque on 23/04/2020.
//

#include <algorithm>
#include <cmath>
#include <memory>

#include <cant/common/info.hpp>
#include <cant/common/types.hpp>

#include <cant/Cantina.hpp>
#include <cant/pan/control/control.hpp>
#include <cant/pan/envelope/envelope.hpp>
#include <cant/pan/note/note.hpp>

#include <cant/common/CantinaException.hpp>
#include <cant/common/config.hpp>

extern "C" {
#include <m_pd.h>
}
#include "../include/cantina~.hpp"

/******** declaration ********/
static t_class *cantina_tilde_class;

/**
 * @brief First inlet is the signal to be tracked
 * if second inlet is not given, it is also the signal to be shifted (the seed).
 *
 */
typedef struct {
  t_object x_obj;
  t_sample f;
  // in_seed: first inlet is implicitly stored by pd
  // NO OTHER INLET OR OUTLET IS HANDLED BY IT!!
  t_inlet *x_in_track;
  t_inlet *x_in_notes;
  t_inlet *x_in_controls;
  /* outlets */
  /** signals **/
  t_outlet **x_out_harmonics;
  /** pitch-tracking-related stuff **/
  t_outlet *x_out_pitch;
  /* internal */
  std::unique_ptr<cant::Cantina> cantina;
  /* cache */
  /** time **/
  // see const!
  double x_t_start_systime;
  /** dsp args **/
  std::vector<t_int> x_vec_dspargs;
  /** atoms (list) **/
  t_atom *x_a_pitch;

} t_cantina_tilde;

/******** utility *******/

void allocate_pitch_atoms(t_cantina_tilde *x) {
  x->x_a_pitch = static_cast<t_atom *>(getbytes(2 * sizeof(t_atom)));
  if (!x->x_a_pitch) {
    bug("cantina~: failed to allocate output pitch message.");
  }
}

void free_control_atoms(t_cantina_tilde *x) {
  freebytes(x->x_a_pitch, 2 * sizeof(t_atom));
}

void copy_pitch(t_cantina_tilde *x, const cant::Pitch &pitch) {
  t_atom *a = x->x_a_pitch;
  // [value (Hz), confidence (in [0, 1])]
  SETFLOAT(a, static_cast<t_float>(pitch.getFreq()));
  SETFLOAT(a + 1, static_cast<t_float>(pitch.getConfidence()));
}

/******** implementation ********/

void *cantina_tilde_new(const t_symbol *, const int argc, t_atom *argv) {
  auto *x = reinterpret_cast<t_cantina_tilde *>(pd_new(cantina_tilde_class));
  /* args */
  t_int n_arg = 0;
  if (argc) {
    n_arg = atom_getint(argv);
  }
  const auto numberHarmonics =
      static_cast<cant::size_u>(std::max<t_int>(0, n_arg));
  /* time */
  x->x_t_start_systime = clock_getlogicaltime();
  /* cantina */
  try {
    x->cantina = std::make_unique<cant::Cantina>(
        numberHarmonics,
        static_cast<cant::type_i>(std::round(sys_getsr())), // sample rate
        1                                                   // channel
    );
    const double start_systime = x->x_t_start_systime;
    /*
     * So, there are issues with using <chrono> utility with pd,
     * delta time is not regular.
     * So now we hook pd's own clock system to our midi timer.
     */
    x->cantina->setCustomClock([start_systime]() -> cant::time_d {
      const cant::type_d t = clock_gettimesince(start_systime) / 1000;
      return t;
    });
  } catch (const cant::CantinaException &e) {
    std::cerr << e.what() << std::endl;
  }
  /* inlet */
  /* first one managed automatically */
  x->x_in_track = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
  x->x_in_notes =
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_list, gensym("notes"));
  x->x_in_controls =
      inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_list, gensym("controls"));
  /* outlets */
  /** scalars **/
  x->x_out_pitch = outlet_new(&x->x_obj, &s_list);
  /** signals again **/
  x->x_out_harmonics = static_cast<t_outlet **>(
      getbytes(x->cantina->getNumberVoices() * sizeof(t_outlet *)));
  if (!x->x_out_harmonics) {
    bug("cantina~: failed to allocate harmonic outlets.");
  }
  for (cant::size_u i = 0; i < x->cantina->getNumberVoices(); ++i) {
    x->x_out_harmonics[i] = outlet_new(&x->x_obj, &s_signal);
  }
  /* atoms */
  allocate_pitch_atoms(x);
  return static_cast<void *>(x);
}

void cantina_tilde_free(t_cantina_tilde *x) {
  /* atom */
  free_control_atoms(x);
  /* utility */
  // clock_free(x->x_clock);
  /* inlets */
  /** signals **/
  inlet_free(x->x_in_track);
  /* outlets */
  /** signals **/
  for (cant::size_u i = 0; i < x->cantina->getNumberVoices(); ++i) {
    outlet_free(x->x_out_harmonics[i]);
  }
  freebytes(x->x_out_harmonics,
            x->cantina->getNumberVoices() * sizeof(t_outlet *));
  /** scalars **/
  outlet_free(x->x_out_pitch);
  /* inlets */
  inlet_free(x->x_in_notes);
  inlet_free(x->x_in_controls);
  /** cantina **/
  x->cantina.reset();
}

void fill_vec_dspargs(t_cantina_tilde *x, t_signal **sp) {
  auto &vec = x->x_vec_dspargs;
  /* dsp args vector */
  vec = std::vector<t_int>(4 + x->cantina->getNumberVoices());
  vec.at(0) = reinterpret_cast<t_int>(x);            // x
  vec.at(1) = static_cast<t_int>(sp[0]->s_n);        // block_size
  vec.at(2) = reinterpret_cast<t_int>(sp[0]->s_vec); // in seed
  vec.at(3) = reinterpret_cast<t_int>(sp[1]->s_vec); // in track
  for (cant::size_u i = 0; i < x->cantina->getNumberVoices(); ++i) {
    vec.at(4 + i) = reinterpret_cast<t_int>(sp[2 + i]->s_vec); // out harmonics
  }
}

t_int *cantina_tilde_perform(t_int *w) {
  auto *x = reinterpret_cast<t_cantina_tilde *>(w[1]);
  auto block_size = static_cast<std::size_t>(w[2]);
  auto *in_seed = reinterpret_cast<t_sample *>(w[3]);
  auto *in_track = reinterpret_cast<t_sample *>(w[4]);
  if (!in_track) {
    // If the tracked signal is not given,
    // the seed should be tracked as well.
    in_track = in_seed;
  }
  auto **out_harmonics = reinterpret_cast<t_sample **>(&w[5]);
  /** DOING STUFF **/
  for (int i = 0; i < static_cast<int>(x->cantina->getNumberVoices()); ++i) {
    /* resetting samples in outlet before filling it again */
    std::fill(out_harmonics[i], out_harmonics[i] + block_size, 0.);
  }
  // seed
  /** CANT **/
  try {
    x->cantina->update();
    x->cantina->perform(in_seed, in_track, out_harmonics, block_size);

    copy_pitch(x, x->cantina->getPitch());
    outlet_list(x->x_out_pitch, &s_list, 2, x->x_a_pitch);

  } catch (const cant::CantinaException &e) {
    std::cerr << e.what() << std::endl;
  }
  const auto size = static_cast<t_int>(x->x_vec_dspargs.size());
  return (w + size + 1);
}

void cantina_tilde_dsp(t_cantina_tilde *x, t_signal **sp) {
  fill_vec_dspargs(x, sp);
  dsp_addv(cantina_tilde_perform, static_cast<int>(x->x_vec_dspargs.size()),
           x->x_vec_dspargs.data());
}

void cantina_tilde_envelope(t_cantina_tilde *x, t_symbol *, int argc,
                            t_atom *argv) {
  // todo still
  if (!argv) {
    bug("cantina~: MidiEnvelope method not set.");
    return;
  }
  // get envelope type
  char buf[20];
  atom_string(argv, buf, 20);
  std::string type(buf);
  if (type == cant::ENVELOPE_TYPE_ADSR) {
    if (argc < 2) {
      bug("cantina~: not enough arguments for envelope %s, "
          "need: damper controller id.",
          type.data());
    }
    // make adsr envelope
    auto adsr = cant::pan::ADSREnvelope::make(x->cantina->getNumberVoices());

    // make damper
    auto const controllerId =
        static_cast<cant::pan::id_u8>(atom_getint(argv + 1));
    auto const channel = static_cast<cant::pan::id_u8>(atom_getint(argv + 2));
    auto damper = cant::pan::MidiDamper::make(channel, controllerId);

    // link the two
    adsr->setController(std::move(damper));
    x->cantina->addEnvelope(std::move(adsr));
  } else {
    bug("cantina~: envelope '%s' not known.", type.data());
    return;
  }
}

void cantina_tilde_notes(t_cantina_tilde *x, t_symbol *, int argc,
                         t_atom *argv) {
  if (argc < 3) {
    bug("cantina~: Wrong format for note input: expected [tone, velocity, "
        "channel]");
    return;
  }
  /* voices of the poly object start at 1 */
  auto const tone = static_cast<cant::pan::tone_i8>(atom_getfloat(argv));
  auto const velocity = static_cast<cant::pan::vel_i8>(atom_getfloat(argv + 1));
  auto const channel = static_cast<cant::pan::id_u8>(atom_getfloat(argv + 2));
  auto const data = cant::pan::MidiNoteInputData(channel, tone, velocity);
  std::optional<cant::size_u> optVoice;
  try {
    optVoice = x->cantina->receiveNote(data);
  } catch (const cant::CantinaException &e) {
    std::cerr << e.what() << std::endl;
  }
}

void cantina_tilde_controls(t_cantina_tilde *x, t_symbol *, int argc,
                            t_atom *argv) {
  if (argc < 3) {
    bug("cantina~: Wrong format for control input: expected [value, "
        "controller id, channel]");
    return;
  }
  /*
   * Alright, so in my former set-up pd is always one step behind when it comes
   * to controls,
   * -- I used to send resp. controllerId to left and value to right inlets of
   * [pack]
   * -- It's because of the [ctlin], I think the output bang from right to left,
   * -- so when I switch the order, when inputting them to the [pack],
   * -- The hot inlet bangs first and sends the oudated control.
   */
  const auto value = static_cast<cant::pan::id_u8>(atom_getfloat(argv));
  const auto controllerId =
      static_cast<cant::pan::id_u8>(atom_getint(argv + 1));
  const auto channel = static_cast<cant::pan::id_u8>(atom_getint(argv + 2));
  const auto data =
      cant::pan::MidiControlInputData(channel, controllerId, value);
  try {
    x->cantina->receiveControl(data);
  } catch (const cant::CantinaException &e) {
    std::cerr << e.what() << std::endl;
  }
}

extern "C" void cantina_tilde_setup(void) {
  cantina_tilde_class =
      class_new(gensym("cantina~"),
                // gcc gives a cast-function-type error here, (remove
                // because t_newmethod is a void* ()(void)
                // whereas cantina_tilde_new takes arguments.
                // oh well.
                reinterpret_cast<t_newmethod>(cantina_tilde_new),
                reinterpret_cast<t_method>(cantina_tilde_free),
                sizeof(t_cantina_tilde), CLASS_DEFAULT, A_GIMME, 0);

  class_addmethod(cantina_tilde_class,
                  reinterpret_cast<t_method>(cantina_tilde_dsp), gensym("dsp"),
                  A_CANT, 0);
  class_addmethod(cantina_tilde_class,
                  reinterpret_cast<t_method>(cantina_tilde_envelope),
                  gensym("envelope"), A_GIMME, 0);
  class_addmethod(cantina_tilde_class,
                  reinterpret_cast<t_method>(cantina_tilde_notes),
                  gensym("notes"), A_GIMME, 0);
  class_addmethod(cantina_tilde_class,
                  reinterpret_cast<t_method>(cantina_tilde_controls),
                  gensym("controls"), A_GIMME, 0);
  CLASS_MAINSIGNALIN(cantina_tilde_class, t_cantina_tilde, f);
  post("Cant version : " CANTINA_VERSION);
  post("Cant brew    : " CANTINA_BREW);
  post("~ tut-tut-tut-tut-tulut-tut ~");
}
