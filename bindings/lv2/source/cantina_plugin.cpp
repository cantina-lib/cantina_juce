/**
 * following: https://lv2plug.in/book/
 */

#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <cmath>
#include <vector>

#include "cantina_plugin.hpp"

#include <lv2/atom/util.h>
#include <lv2/core/lv2_util.h>

#define DEFAULT_BUFFER_SIZE 1024
#define DEFAULT_NB_VOICES 4

static constexpr float db_gain_to_coef(float gain) {
    return gain > -90.0f ? std::pow(10.0f, gain * 0.05f) : 0.0f;
}


size_t get_block_size(CantinaPlugin *self) {
    return self->outputBuffers.at(0).size();
}


void set_cantina(CantinaPlugin * self, size_t nb_voices) {
    auto cantina = std::make_unique<cant::Cantina>(
            nb_voices,
            self->rate,
            1 // channel
            );

    self->cantina->setCustomClock([&self]() -> double{
        // last block size
        return self->rate * get_block_size(self);
    });
}

void allocate_output_buffers(CantinaPlugin * self, size_t nb_voices, uint32_t block_size) {
    self->outputBuffers = std::vector<std::vector<float>>(nb_voices, std::vector<float>(block_size));
}
void fit_output_buffers(CantinaPlugin * self, uint32_t block_size) {
    if (block_size > self->outputBuffers.at(0).size()) {
        std::for_each(
                self->outputBuffers.begin(),
                self->outputBuffers.end(),
                [block_size](auto buffer){ buffer.resize(block_size); }
                );
    }
}

static LV2_Handle
instantiate([[maybe_unused]] LV2_Descriptor const * descriptor,
            [[maybe_unused]] double rate,
            [[maybe_unused]] char const* bundle_path,
            LV2_Feature const * const * features) {
    CantinaPlugin * self;
    try {
        self = new CantinaPlugin();
    }
    catch (const std::invalid_argument& ia) 
    {
        std::cerr << ia.what() << std::endl;
        return nullptr;
    }
    catch (const std::bad_alloc& ba)
    {
        std::cerr << "Failed to allocate memory. Can't instantiate mySimplePolySynth" << std::endl;
        return nullptr;
    }
    self->rate = rate;
    allocate_output_buffers(self, DEFAULT_NB_VOICES, DEFAULT_BUFFER_SIZE);
    // default value for number of voices

    // Scan host features for URID map
    char const * missing = lv2_features_query(
            features,
            LV2_LOG__log, &self->logger.log, false,
            LV2_URID__map, &self->map, true,
            nullptr);
    lv2_log_logger_set_map(&self->logger, self->map);
    if (missing) {
        lv2_log_error(&self->logger, "Missing feature <%s> \n", missing);
        free(self);
        return nullptr;
    }

    map_cantina_uris(self->map, &self->uris);

    size_t nb_voices = self->ports.nb_voices ? *self->ports.nb_voices : DEFAULT_NB_VOICES;

    try {
        set_cantina(self, nb_voices);
    } catch (cant::CantinaException const & e) {
        std::cerr << e.what() << std::endl;
    }

    return reinterpret_cast<LV2_Handle>(self);
}


static void
cleanup(LV2_Handle instance) {
    auto self = reinterpret_cast<CantinaPlugin *>(instance);
    if (self) {
        free(self);
    }
}


void merge_output(CantinaPlugin * self) {
    std::fill(self->ports.output, self->ports.output + get_block_size(self), 0.);
    for (auto voice=0; voice < self->outputBuffers.size(); ++voice) {
        std::transform(
                self->ports.output,
                self->ports.output + get_block_size(self),
                self->outputBuffers.at(voice).begin(),
                self->ports.output,
                std::plus<float>()
            );
    }
    //  std::copy(output.begin(), output.end(), self->ports.output);
}

static void
connect_port(LV2_Handle instance, uint32_t port, void * data) {
    auto self = reinterpret_cast<CantinaPlugin *>(instance);
    size_t nb_voices;
    switch (static_cast<EPortIndex>(port)) {
        case CANTINA_CONTROL:
            self->ports.control = reinterpret_cast<LV2_Atom_Sequence const *>(data);
            break;
        case CANTINA_NUMBERVOICES:
            self->ports.nb_voices = reinterpret_cast<int const *>(data);
                nb_voices = self->ports.nb_voices ? *self->ports.nb_voices : DEFAULT_NB_VOICES;

            // todo: reset Cantina
            try {
                set_cantina(self, nb_voices);
            } catch (cant::CantinaException const & e) {
                std::cerr << e.what() << std::endl;
            }
            break;
        case CANTINA_GAIN:
            self->ports.gain = reinterpret_cast<float const*>(data);
            break;
        case CANTINA_INPUT_SEED:
            self->ports.input_seed = reinterpret_cast<float const*>(data);
            break;
        case CANTINA_INPUT_TRACK:
            self->ports.input_track = reinterpret_cast<float const*>(data);
            break;
        case CANTINA_OUTPUT:
            self->ports.output = reinterpret_cast<float *>(data);
            break;
    }
}

static void
activate(LV2_Handle instance) {
    // todo
    auto self = reinterpret_cast<CantinaPlugin *>(instance);

}

static void
deactivate([[maybe_unused]] LV2_Handle instance) {

}

static void const *
extension_data([[maybe_unused]] char const * uri) {
    return nullptr;
}

static void
run(LV2_Handle instance, uint32_t nb_samples) {
    auto self = reinterpret_cast<CantinaPlugin *>(instance);

    fit_output_buffers(self, nb_samples);

    // Notes and controls
    LV2_Atom_Sequence  const * seq = self->ports.control;
    LV2_ATOM_SEQUENCE_FOREACH(seq, ev) {
        if (ev->body.type != self->uris.midi_Event) { continue; }
        auto msg = reinterpret_cast<uint8_t const *>(ev + 1);
        switch(lv2_midi_message_type(msg)) {
            case LV2_MIDI_MSG_NOTE_ON:
            case LV2_MIDI_MSG_NOTE_OFF:
                try {
                    self->cantina->receiveNote(
                            cant::pan::MidiNoteInputData(
                                    msg[0],
                                    static_cast<int8_t>(msg[1]),
                                    static_cast<int8_t>(msg[2])));
                }
                catch (cant::CantinaException const & e) {
                    std::cerr << e.what() << std::endl;
                }
                break;
            case LV2_MIDI_MSG_CONTROLLER:
                try {
                    self->cantina->receiveControl(
                            cant::pan::MidiControlInputData(
                                    msg[0],
                                    static_cast<uint8_t>(msg[1]),
                                    static_cast<uint8_t>(msg[2])));
                }
                catch (cant::CantinaException const & e) {
                    std::cerr << e.what() << std::endl;
                }
                break;
            default:
                break;
        }
    }

    float **interfaceBuffers;
    try {
        // convert vector of vector of float to float**.
        interfaceBuffers = new float*[self->cantina->getNumberVoices()];
        std::transform(
            self->outputBuffers.begin(),
            self->outputBuffers.end(),
            interfaceBuffers,
            [](auto v) {
                return v.data();
            }
        );
    }
    catch (const std::bad_alloc& ba)
    {
        return;
    }

    try {
        self->cantina->update();

        auto track = self->ports.input_track ? self->ports.input_track : self->ports.input_seed;
        self->cantina->perform(self->ports.input_seed, track, interfaceBuffers, get_block_size(self));
    } catch (cant::CantinaException const &e) {
        std::cerr << e.what() << std::endl;
    }
    delete interfaceBuffers;

    merge_output(self);
}

static LV2_Descriptor const descriptor = {
    PLUGIN_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data
};

LV2_SYMBOL_EXPORT
LV2_Descriptor const *
lv2_descriptor(uint32_t index) {
    return index == 0 ? &descriptor : nullptr;
}

