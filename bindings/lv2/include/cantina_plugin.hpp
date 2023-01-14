//
// Created by binabik on 30/05/2021.
//

#ifndef CANTINA_LV2_INCLUDE_CANTINA_PLUGIN_HPP
#define CANTINA_LV2_INCLUDE_CANTINA_PLUGIN_HPP

#include <vector>
#include <memory>

#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <lv2/patch/patch.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/midi/midi.h>
#include <lv2/urid/urid.h>

#include <cant/Cantina.hpp>
#include <cant/pan/Pantoufle.hpp>

enum EPortIndex {
    CANTINA_CONTROL = 0,
    CANTINA_NUMBERHARMONICS = 1,
    CANTINA_GAIN = 2,
    CANTINA_INPUT = 3,
    CANTINA_OUTPUT = 4
} ;

struct CantinaURIs {
    LV2_URID midi_Event;
};

struct CantinaPlugin {
    // Features
    LV2_URID_Map * map;
    LV2_Log_Logger  logger;

    CantinaURIs uris;

    struct {
        LV2_Atom_Sequence  const * control;
        float const * gain;
        int const *nb_voices;
        float const * input;
        float * output;
    } ports;

    double rate;
    //Cantina
    std::unique_ptr<cant::Cantina> cantina;
    // std::vector<float> seedOutputBuffer;
    // std::vector<std::vector<float>> harmonicsOutputBuffers;
    float *seedOutputBuffer;
    float **harmonicsOutputBuffers;
    uint32_t cachedBlockSize;

};

static inline void
map_cantina_uris(LV2_URID_Map * map, CantinaURIs * uris) {
    uris->midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);
}

#endif //CANTINA_LV2_INCLUDE_CANTINA_PLUGIN_HPP
