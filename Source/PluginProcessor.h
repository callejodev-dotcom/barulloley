#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <escoita_nucleo/escoita_nucleo.h>
#include "Eco.h"

// Barullo Ley: un delay. Catro mandos (tempo, repeticións, ton, mestura) e o tipo para o día a
// día; detrás de «Máis», espallar, anchura, movemento, saturación, cortes, atenuación e volume.
// Os presets de fábrica son os programas.
class BLProcessor : public juce::AudioProcessor
{
public:
    BLProcessor();
    ~BLProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Barullo Ley"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override { return preset; }
    void setCurrentProgram (int index) override { cargarPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::UndoManager undo;   // desfacer e refacer (declárase antes ca o apvts)
    juce::AudioProcessorValueTreeState apvts;
    // presets propios (.blpreset en Documentos/Barullo Ley/Presets), A/B e desfacer, do núcleo
    escoita::Estado estado { apvts, &undo, opcionsEstado() };

    // ---- presets de fábrica (desde o fío da interface)
    void cargarPreset (int i);
    int getPreset() const { return preset; }
    juce::String nomePreset() const { return nome; }
    bool presetEditado() const;   // os mandos xa non son os do preset

    ley::Axustes axustes() const;   // os parámetros de agora, co tempo xa en ms (para a pantalla)
    double tempoDAW() const { return bpm.load(); }

    // para a pantalla (escríbese desde o fío de audio)
    std::atomic<float> nivelEntrada { 0 }, nivelEcos { 0 }, nivelSaida { 0 };
    float uiScale = 1.0f;

private:
    escoita::Estado::Opcions opcionsEstado();
    int preset = 0;
    juce::String nome;

    ley::Eco eco;
    escoita::Teito teito;
    escoita::Atraso atrasoL, atrasoR;   // o bypass, á par do teito (a súa anticipación)
    std::atomic<double> bpm { 120.0 };
    double sr = 48000, mestS = 0, volS = 1, bypassS = 0;
    std::atomic<float> *tipo = nullptr, *sincronia = nullptr, *nota = nullptr, *ms = nullptr, *repeticions = nullptr, *ton = nullptr,
                       *mestura = nullptr, *espallar = nullptr, *anchura = nullptr, *movemento = nullptr, *saturacion = nullptr,
                       *corteGraves = nullptr, *corteAgudos = nullptr, *atenuacion = nullptr, *conxelar = nullptr, *volume = nullptr, *bypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BLProcessor)
};
