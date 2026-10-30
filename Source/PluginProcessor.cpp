#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

using escoita::U;

BLProcessor::BLProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "BarulloLey", createLayout())
{
    auto p = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    tipo = p ("tipo"); sincronia = p ("sincronia"); nota = p ("nota"); ms = p ("ms"); repeticions = p ("repeticions"); ton = p ("ton");
    mestura = p ("mestura"); espallar = p ("espallar"); anchura = p ("anchura"); movemento = p ("movemento"); saturacion = p ("saturacion");
    corteGraves = p ("corteGraves"); corteAgudos = p ("corteAgudos"); atenuacion = p ("atenuacion"); conxelar = p ("conxelar");
    volume = p ("volume"); bypass = p ("bypass");
    nome = U (ley::PRESETS[0].nome);
}

juce::AudioProcessorValueTreeState::ParameterLayout BLProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    const auto& p0 = ley::PRESETS[0];   // os valores de partida son os do primeiro preset
    const StringArray onOff { "Off", "On" };
    // o texto cun número fixo de decimais (texto → valor → texto dá o mesmo texto)
    auto real = [] (const char* id, const String& nomeP, NormalisableRange<float> r, float def, const char* unidade, int decimais = 1)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nomeP, r, def, AudioParameterFloatAttributes().withLabel (unidade)
                   .withStringFromValueFunction ([decimais] (float v, int) { return String (v, decimais); }));
    };
    auto centro = [] (float mn, float mx, float c) { NormalisableRange<float> r (mn, mx); r.setSkewForCentre (c); return r; };
    StringArray notas;
    for (auto* n : ley::NOTAS) notas.add (n);
    // o tipo: o texto pode volver do DAW noutra codificación («Analóxico»), así que abonda co comezo
    const StringArray tipos { "Dixital", "Cinta", U ("Analóxico"), "Pingue-pongue", "Inverso" };
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "tipo", 1 }, "Tipo", tipos, p0.tipo,
               AudioParameterChoiceAttributes().withValueFromStringFunction ([tipos] (const String& t)
               {
                   for (int k = 0; k < tipos.size(); ++k) if (t == tipos[k] || (t.length() >= 4 && tipos[k].startsWith (t.substring (0, 4)))) return (float) k;
                   return 0.0f;
               })));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "sincronia", 1 }, U ("Á par do DAW"), onOff, (int) p0.sincronia));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "nota", 1 }, "Figura", notas, p0.nota));
    l.add (real ("ms", "Tempo (ms)", centro (1.0f, 2000.0f, 300.0f), (float) p0.ms, "ms", 0));
    l.add (real ("repeticions", U ("Repeticións"), { 0.0f, 100.0f }, (float) p0.repeticions * 100.0f, "%"));
    l.add (real ("ton", "Ton", { 0.0f, 100.0f }, (float) p0.ton * 100.0f, "%"));
    l.add (real ("mestura", "Mestura", { 0.0f, 100.0f }, (float) p0.mestura * 100.0f, "%"));
    l.add (real ("espallar", "Espallar", { 0.0f, 100.0f }, (float) p0.espallar * 100.0f, "%"));
    l.add (real ("anchura", "Anchura", { 0.0f, 150.0f }, (float) p0.anchura * 100.0f, "%"));
    l.add (real ("movemento", "Movemento", { 0.0f, 100.0f }, (float) p0.movemento * 100.0f, "%"));
    l.add (real ("saturacion", U ("Saturación"), { 0.0f, 100.0f }, (float) p0.saturacion * 100.0f, "%"));
    l.add (real ("corteGraves", "Corte de graves", centro (20.0f, 1000.0f, 120.0f), (float) p0.corteGraves, "Hz", 0));
    l.add (real ("corteAgudos", "Corte de agudos", centro (1000.0f, 20000.0f, 6000.0f), (float) p0.corteAgudos, "Hz", 0));
    l.add (real ("atenuacion", U ("Atenuación"), { 0.0f, 100.0f }, (float) p0.atenuacion * 100.0f, "%"));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "conxelar", 1 }, "Conxelar", onOff, 0));
    l.add (real ("volume", "Volume", { -24.0f, 6.0f, 0.1f }, 0.0f, "dB"));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { "bypass", 1 }, "Bypass", onOff, 0));
    return l;
}

// ================================================================ presets
int BLProcessor::getNumPrograms() { return ley::NUM_PRESETS; }
const juce::String BLProcessor::getProgramName (int i) { return U (ley::PRESETS[juce::jlimit (0, ley::NUM_PRESETS - 1, i)].nome); }

static std::vector<std::pair<const char*, float>> valoresDe (const ley::Preset& p)
{
    return { { "tipo", (float) p.tipo }, { "sincronia", p.sincronia ? 1.0f : 0.0f }, { "nota", (float) p.nota }, { "ms", (float) p.ms },
             { "repeticions", (float) p.repeticions * 100.0f }, { "ton", (float) p.ton * 100.0f }, { "mestura", (float) p.mestura * 100.0f },
             { "espallar", (float) p.espallar * 100.0f }, { "anchura", (float) p.anchura * 100.0f }, { "movemento", (float) p.movemento * 100.0f },
             { "saturacion", (float) p.saturacion * 100.0f }, { "corteGraves", (float) p.corteGraves }, { "corteAgudos", (float) p.corteAgudos },
             { "atenuacion", (float) p.atenuacion * 100.0f }, { "conxelar", 0.0f } };
}

void BLProcessor::cargarPreset (int i)
{
    i = juce::jlimit (0, ley::NUM_PRESETS - 1, i);
    const auto antes = std::make_pair (preset, nome);
    for (auto [id, v] : valoresDe (ley::PRESETS[i]))
        if (auto* prm = apvts.getParameter (id))
        {
            const float n = prm->convertTo0to1 (v);
            if (std::abs (prm->getValue() - n) < 1.0e-6f) continue;
            prm->beginChangeGesture(); prm->setValueNotifyingHost (n); prm->endChangeGesture();
        }
    const auto despois = std::make_pair (i, U (ley::PRESETS[i].nome));
    // o número e o nome do preset tamén se desfán (os parámetros xa os leva o desfacer do APVTS)
    estado.cambio (U ("preset"), [this, despois] { preset = despois.first; nome = despois.second; },
                                 [this, antes] { preset = antes.first; nome = antes.second; });
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

bool BLProcessor::presetEditado() const
{
    const int i = juce::jlimit (0, ley::NUM_PRESETS - 1, preset);
    if (nome != U (ley::PRESETS[i].nome)) return false;   // un preset propio
    const auto& p = ley::PRESETS[i];
    for (auto [id, v] : valoresDe (p))
    {
        if (p.sincronia ? juce::String (id) == "ms" : juce::String (id) == "nota") continue;   // o tempo que non se usa non conta
        if (auto* prm = apvts.getParameter (id))
            if (std::abs (prm->getValue() - prm->convertTo0to1 (v)) > 0.002f) return true;
    }
    return false;
}

escoita::Estado::Opcions BLProcessor::opcionsEstado()
{
    escoita::Estado::Opcions o;
    o.extension = "blpreset";
    o.excluir = { "bypass" };
    // o nome do que se está a escoitar vai co estado (un preset propio leva o seu)
    o.extra = [this] { juce::ValueTree t ("Preset"); t.setProperty ("numero", preset, nullptr); t.setProperty ("nome", nome, nullptr); return t; };
    o.aplicarExtra = [this] (const juce::ValueTree& t) { preset = juce::jlimit (0, ley::NUM_PRESETS - 1, (int) t.getProperty ("numero", 0)); nome = t.getProperty ("nome", nome).toString(); };
    return o;
}

ley::Axustes BLProcessor::axustes() const
{
    ley::Axustes a;
    a.tipo = (int) std::lround (tipo->load());
    const int n = juce::jlimit (0, ley::NUM_NOTAS - 1, (int) std::lround (nota->load()));
    a.tempo = sincronia->load() > 0.5f ? ley::NEGRAS[n] * 60000.0 / juce::jlimit (20.0, 400.0, bpm.load()) : (double) ms->load();
    a.repeticions = repeticions->load() / 100.0; a.ton = ton->load() / 100.0; a.espallar = espallar->load() / 100.0;
    a.anchura = anchura->load() / 100.0; a.movemento = movemento->load() / 100.0; a.saturacion = saturacion->load() / 100.0;
    a.corteGraves = corteGraves->load(); a.corteAgudos = corteAgudos->load(); a.atenuacion = atenuacion->load() / 100.0;
    a.conxelar = conxelar->load() > 0.5f;
    return a;
}

double BLProcessor::getTailLengthSeconds() const
{
    if (conxelar->load() > 0.5f) return 30.0;
    const auto a = axustes();
    const double g = juce::jlimit (0.001, 0.999, a.repeticions);
    const double voltas = std::log (0.001) / std::log (g);   // ata −60 dB
    return juce::jlimit (0.1, 30.0, a.tempo / 1000.0 * (voltas + 1.0) * 1.25);
}

// ================================================================ audio
// o volume en pasos de 0,1 dB: a 0 dB, a ganancia é 1 xusto (coa mestura a 0, o son sae tal cal)
static double ganancia (float db) { return juce::Decibels::decibelsToGain (std::round ((double) db * 10.0) / 10.0); }

bool BLProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void BLProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    eco.preparar (sampleRate);
    eco.axustar (axustes());
    // o teito mira 1,5 ms por diante (baixa suave antes do pico): esa é a latencia, e o bypass vai igual de atrasado
    teito.preparar (sampleRate, 0.080, 0.891, 0.0015);
    atrasoL.preparar (teito.latencia()); atrasoR.preparar (teito.latencia());
    setLatencySamples (teito.latencia());
    mestS = mestura->load() / 100.0;
    volS = ganancia (volume->load());
    bypassS = bypass->load() > 0.5f ? 1.0 : 0.0;
}

void BLProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int ins = getTotalNumInputChannels(), outs = getTotalNumOutputChannels();
    float* L = buffer.getWritePointer (0);
    float* R = outs > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R != nullptr && ins < 2) std::copy (L, L + n, R);   // mono → estéreo

    // o tempo do DAW (sen DAW, 120)
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) if (*b > 1.0) bpm.store (*b);

    eco.axustar (axustes());
    const double mestT = mestura->load() / 100.0, volT = ganancia (volume->load()), bypassT = bypass->load() > 0.5f ? 1.0 : 0.0;
    const double sm = 1.0 - std::exp (-1.0 / (0.02 * sr));
    float pe = 0, pc = 0, ps = 0;
    for (int i = 0; i < n; ++i)
    {
        mestS += (mestT - mestS) * sm; volS += (volT - volS) * sm; bypassS += (bypassT - bypassS) * sm;
        const double dl = L[i], dr = R != nullptr ? R[i] : L[i];
        float wl = (float) dl, wr = (float) dr;
        eco.procesar (wl, wr);
        // mestura con potencia constante: ao 50 %, os dous a −3 dB
        const double gs = std::cos (mestS * ley::PI / 2), gm = std::sin (mestS * ley::PI / 2);
        double l = (dl * gs + wl * gm) * volS, r = (dr * gs + wr * gm) * volS;
        if (R != nullptr) teito.procesar (l, r);
        else l = r = teito.procesar (0.5 * (l + r));
        // o bypass deixa pasar o son tal cal (sen o teito, co seu atraso); os ecos seguen a soar por dentro
        const double al = atrasoL.procesar (dl), ar = atrasoR.procesar (dr);
        l = l * (1.0 - bypassS) + al * bypassS; r = r * (1.0 - bypassS) + ar * bypassS;
        L[i] = (float) l;
        if (R != nullptr) R[i] = (float) r;
        pe = std::max ({ pe, (float) std::abs (dl), (float) std::abs (dr) });
        pc = std::max ({ pc, std::abs (wl), std::abs (wr) });
        ps = std::max ({ ps, (float) std::abs (l), (float) std::abs (r) });
    }
    for (int ch = 2; ch < outs; ++ch) buffer.clear (ch, 0, n);
    nivelEntrada.store (pe); nivelEcos.store (pc); nivelSaida.store (ps);
}

// ================================================================ estado
void BLProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto st = apvts.copyState();
    st.setProperty ("preset", preset, nullptr);
    st.setProperty ("nomePreset", nome, nullptr);
    st.setProperty ("uiScale", uiScale, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void BLProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    uiScale = juce::jlimit (0.5f, 2.0f, (float) tree.getProperty ("uiScale", 1.0f));
    preset = juce::jlimit (0, ley::NUM_PRESETS - 1, (int) tree.getProperty ("preset", 0));
    nome = tree.getProperty ("nomePreset", U (ley::PRESETS[preset].nome)).toString();
    apvts.replaceState (tree);
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

juce::AudioProcessorEditor* BLProcessor::createEditor() { return new BLEditor (*this); }

#ifndef BARULLO_NO_PLUGIN_ENTRY
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new BLProcessor(); }
#endif
