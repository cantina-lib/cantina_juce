/**
 * following: https://lv2plug.in/book/
 */

#include <memory>
#include <cmath>

#include "cantina_plugin.hpp"

#include <lv2/atom/util.h>
#include <lv2/core/lv2_util.h>

static constexpr float db_gain_to_coef(float gain) {
    return gain > -90.0f ? std::pow(10.0f, gain * 0.05f) : 0.0f;
}

static LV2_Handle
instantiate([[maybe_unused]] LV2_Descriptor const * descriptor,
            [[maybe_unused]] double rate,
            [[maybe_unused]] char const* bundle_path,
            LV2_Feature const * const * features) {
    auto self = static_cast<CantinaPlugin *>(calloc(1, sizeof(CantinaPlugin)));
    if (!self) {
        // ERROR
        return nullptr;
    }
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


    try {
        self->cantina = std::make_unique<cant::Cantina>(
                *self->ports.n_voices,
                rate,
                1 // channel
                );
        self->cachedBlockSize = 0; // init, whatever.

        self->cantina->setCustomClock([&self, rate]() -> double{
            // last block size
            return rate * self->cachedBlockSize;
        });
    } catch (cant::CantinaException const & e) {
        std::cerr << e.what() << std::endl;
    }

    return reinterpret_cast<LV2_Handle>(self);
}

static void
cleanup(LV2_Handle instance) {
    free(instance);
}

void update_output_buffers(CantinaPlugin * self, uint32_t block_size) {
    if (block_size <= self->cachedBlockSize) {
        return;
    }
    for (std::size_t voice = 0; voice < self->cantina->getNumberHarmonics(); ++voice) {
        self->outputBuffers[voice] = static_cast<float *>(
                realloc(self->outputBuffers[voice], block_size * sizeof(float)));
    }
    self->cachedBlockSize = block_size;
}

void merge_output(CantinaPlugin * self, float coef, uint32_t block_size) {
    for (uint32_t i = 0; i < block_size; ++i) {
        self->ports.output[i] = 0.f;
        for (std::size_t voice = 0; voice < self->cantina->getNumberHarmonics(); ++voice) {
            self->ports.output[i] += self->outputBuffers[voice][i];
        }
        self->ports.output[i] *= coef;
    }
}

static void
connect_port(LV2_Handle instance, uint32_t port, void * data) {
    auto self = reinterpret_cast<CantinaPlugin *>(instance);
    switch (static_cast<EPortIndex>(port)) {
        case CANTINA_CONTROL:
            self->ports.control = reinterpret_cast<LV2_Atom_Sequence const *>(data);
            break;
        case CANTINA_NUMBERHARMONICS:
            self->ports.n_voices = reinterpret_cast<int const *>(data);
            // todo: reset Cantina
            break;
        case CANTINA_GAIN:
            self->ports.gain = reinterpret_cast<float const*>(data);
            break;
        case CANTINA_INPUT:
            self->ports.input = reinterpret_cast<float const*>(data);
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
run(LV2_Handle instance, uint32_t n_samples) {
    auto self = reinterpret_cast<CantinaPlugin *>(instance);

    update_output_buffers(self, n_samples);

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

    try {
        self->cantina->update();

        self->cantina->perform(self->ports.input, self->outputBuffers, n_samples);

    } catch (cant::CantinaException const &e) {
        std::cerr << e.what() << std::endl;
    }

    float coef = db_gain_to_coef(*self->ports.gain);
    merge_output(self, coef, n_samples);
}

LV2_SYMBOL_EXPORT
LV2_Descriptor const *
lv2_descriptor(uint32_t index) {
    static LV2_Descriptor const descriptor = {
        CANTINA_URI,
        instantiate,
        connect_port,
        activate,
        run,
        deactivate,
        cleanup,
        extension_data
    };
    return index == 0 ? &descriptor : nullptr;
}
