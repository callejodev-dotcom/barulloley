// Proba sen interface: o tempo dos ecos (en ms e á par do DAW), as repeticións, o pingue-pongue,
// o espallar, o inverso, conxelar, o cambio de tempo sen clics, todos os presets, a mestura e o
// bypass, o estado, o A/B, os presets propios e o desfacer, o limitador e a independencia do
// tamaño de bloque; tamén capturas da interface.
//   BLRenderTest [saida.wav]                         proba enteira (devolve o número de fallos)
//   BLRenderTest --snapshot saida.png [mais] [preset] a interface (con «mais», o panel Máis aberto)
//   BLRenderTest --render saida.wav [preset]         un preset cun son de proba (golpes e un acorde)
//   BLRenderTest --demo preset entrada.wav saida.wav un preset sobre un son (cos ecos enteiros)
#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"
#include <cstdio>
#include <cstring>

using escoita::U;

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double sr)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, (unsigned) b.getNumChannels(), 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            std::printf ("WAV: %s\n", f.getFullPathName().toRawUTF8());
        }
}

// un DAW de mentira: só o tempo
struct Cabeza : juce::AudioPlayHead
{
    double bpm = 120;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); return p; }
};

// pasa un son polo plugin en bloques (o son pode ser máis curto: despois, silencio). A saída vén á par
// da entrada, coma no DAW: sen as primeiras mostras da latencia (a anticipación do teito)
static juce::AudioBuffer<float> procesar (BLProcessor& p, const juce::AudioBuffer<float>& in, double len, int blockSize, double sr, bool preparar = true)
{
    if (preparar) { p.setPlayConfigDetails (2, 2, sr, blockSize); p.prepareToPlay (sr, blockSize); }
    const int total = (int) (len * sr), lat = p.getLatencySamples();
    juce::AudioBuffer<float> out (2, total), buf (2, blockSize);
    out.clear();
    juce::MidiBuffer midi;
    for (int pos = 0; pos < total + lat; pos += blockSize)
    {
        const int n = juce::jmin (blockSize, total + lat - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                if (pos + i < in.getNumSamples()) buf.setSample (c, i, in.getSample (c % in.getNumChannels(), pos + i));
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                if (pos + i >= lat && pos + i - lat < total) out.setSample (c, pos + i - lat, buf.getSample (c, i));
    }
    return out;
}

static void setParam (BLProcessor& p, const juce::String& id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

// só os ecos, limpos: dixital, sen movemento nin cortes, co tempo en ms
static void limpo (BLProcessor& p, float ms, float rep)
{
    setParam (p, "tipo", (float) ley::DIXITAL); setParam (p, "sincronia", 0.0f); setParam (p, "ms", ms); setParam (p, "repeticions", rep);
    setParam (p, "mestura", 100.0f); setParam (p, "ton", 50.0f); setParam (p, "movemento", 0.0f); setParam (p, "espallar", 0.0f);
    setParam (p, "corteGraves", 20.0f); setParam (p, "corteAgudos", 20000.0f); setParam (p, "saturacion", 0.0f); setParam (p, "atenuacion", 0.0f);
}

// o son de proba: catro golpes (ruído que cae axiña) e un acorde de sinusoides que se apaga
static juce::AudioBuffer<float> sonDeProba (double sr, double len = 2.0)
{
    juce::AudioBuffer<float> b (2, (int) (len * sr));
    b.clear();
    juce::Random rnd (1984);
    for (double t0 : { 0.0, 0.25, 0.5, 0.875 })
        for (int i = 0; i < (int) (0.12 * sr); ++i)
        {
            const float e = 0.5f * std::exp (-(float) i / (float) (0.025 * sr));
            const int k = (int) (t0 * sr) + i;
            b.setSample (0, k, b.getSample (0, k) + e * (rnd.nextFloat() * 2 - 1));
            b.setSample (1, k, b.getSample (1, k) + e * (rnd.nextFloat() * 2 - 1));
        }
    for (double f : { 261.63, 329.63, 392.0, 523.25 })
        for (int i = 0; i < (int) (0.8 * sr); ++i)
        {
            const int k = (int) (1.2 * sr) + i;
            if (k >= b.getNumSamples()) break;
            const float x = 0.09f * std::sin (2.0f * juce::MathConstants<float>::pi * (float) (f * i / sr)) * std::exp (-(float) i / (float) (0.25 * sr))
                            * juce::jmin (1.0f, (float) i / 100.0f);
            b.setSample (0, k, b.getSample (0, k) + x);
            b.setSample (1, k, b.getSample (1, k) + x);
        }
    return b;
}

// un golpe curto e suave (unha xanela de 33 mostras, co pico no medio): os ecos son copias del
static juce::AudioBuffer<float> golpe (double, float nivel = 0.5f)
{
    const int n = 33;
    juce::AudioBuffer<float> b (2, n);
    for (int i = 0; i < n; ++i)
    {
        const float w = std::sin (juce::MathConstants<float>::pi * (float) (i + 1) / (float) (n + 1));
        b.setSample (0, i, nivel * w * w); b.setSample (1, i, nivel * w * w);
    }
    return b;
}

static float pico (const juce::AudioBuffer<float>& b, int desde = 0, int ata = -1, int canle = -1)
{
    if (ata < 0) ata = b.getNumSamples();
    desde = juce::jlimit (0, b.getNumSamples(), desde); ata = juce::jlimit (desde, b.getNumSamples(), ata);
    if (canle >= 0) return b.getMagnitude (canle, desde, ata - desde);
    return juce::jmax (b.getMagnitude (0, desde, ata - desde), b.getMagnitude (1, desde, ata - desde));
}
static int ondePico (const juce::AudioBuffer<float>& b, int canle, int desde, int ata)
{
    int onde = desde; float mx = 0;
    for (int i = desde; i < juce::jmin (ata, b.getNumSamples()); ++i) if (std::abs (b.getSample (canle, i)) > mx) { mx = std::abs (b.getSample (canle, i)); onde = i; }
    return onde;
}
static bool finito (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i) if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}
static float diferenza (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float d = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < juce::jmin (a.getNumSamples(), b.getNumSamples()); ++i) d = juce::jmax (d, std::abs (a.getSample (c, i) - b.getSample (c, i)));
    return d;
}
static double enerxia (const juce::AudioBuffer<float>& b, double sr, double t0, double t1)
{
    double e = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = juce::jmax (0, (int) (t0 * sr)); i < juce::jmin (b.getNumSamples(), (int) (t1 * sr)); ++i) e += (double) b.getSample (c, i) * b.getSample (c, i);
    return e;
}

// a pegada dun son: FNV-1a de 64 bits das mostras (un cambio de organización non pode movela nin un bit)
static uint64_t pegada (const juce::AudioBuffer<float>& b)
{
    uint64_t h = 1469598103934665603ull;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x = b.getSample (c, i);
            uint32_t u; std::memcpy (&u, &x, 4);
            for (int k = 0; k < 4; ++k) { h ^= (u >> (8 * k)) & 0xFF; h *= 1099511628211ull; }
        }
    return h;
}

static void snapshot (const juce::File& png, bool mais, int preset)
{
    BLProcessor p;
    const double sr = 48000;
    p.setPlayConfigDetails (2, 2, sr, 512);
    p.prepareToPlay (sr, 512);
    p.cargarPreset (preset >= 0 ? preset : (mais ? 5 : 0));
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto* ce = dynamic_cast<BLEditor*> (ed.get());
    ed->setSize (ui::MainView::W, ui::MainView::H);
    if (mais) ce->view().mostrarMais (true);
    const auto son = sonDeProba (sr);
    juce::AudioBuffer<float> buf (2, 512);
    for (int blk = 0; blk < 60; ++blk)
    {
        buf.clear();
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, son, c, blk * 512, 512);
        juce::MidiBuffer m; p.processBlock (buf, m);
        ce->view().timerCallback();
    }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    png.deleteFile();
    juce::FileOutputStream os (png);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("PNG: %s\n", png.getFullPathName().toRawUTF8());
}

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String();

    if (mode == "--snapshot" && argc > 2)
    {
        const bool mais = argc > 3 && juce::String (argv[3]) == "mais";
        const int pr = argc > 4 ? atoi (argv[4]) : (argc > 3 && ! mais ? atoi (argv[3]) : -1);
        snapshot (juce::File (juce::String (argv[2])), mais, pr);
        return 0;
    }

    if (mode == "--render" && argc > 2)
    {
        BLProcessor p;
        p.cargarPreset (argc > 3 ? atoi (argv[3]) : 0);
        writeWav (juce::File (juce::String (argv[2])), procesar (p, sonDeProba (sr), 2.0 + juce::jmin (10.0, p.getTailLengthSeconds()), 512, sr), sr);
        return 0;
    }

    if (mode == "--demo" && argc > 4)
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (juce::File (juce::String (argv[3]))));
        if (r == nullptr) { std::printf ("non se pode ler %s\n", argv[3]); return 1; }
        juce::AudioBuffer<float> in ((int) r->numChannels, (int) r->lengthInSamples);
        r->read (&in, 0, (int) r->lengthInSamples, 0, true, true);
        BLProcessor p;
        p.cargarPreset (atoi (argv[2]));
        const double len = (double) in.getNumSamples() / r->sampleRate + juce::jmin (10.0, p.getTailLengthSeconds());
        writeWav (juce::File (juce::String (argv[4])), procesar (p, in, len, 512, r->sampleRate), r->sampleRate);
        return 0;
    }

    int fails = 0;
    auto check = [&fails] (bool ok, const char* what, const juce::String& extra = {})
    {
        std::printf ("%-52s %s %s\n", what, ok ? "OK" : "FALLO", extra.toRawUTF8());
        fails += ok ? 0 : 1;
    };

    {   // ao abrir: o primeiro preset, cos parámetros nos seus valores de partida (sen editar)
        BLProcessor p;
        juce::String mal;
        for (auto* prm : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
                if (std::abs (r->getValue() - r->getDefaultValue()) > 1e-6f) mal << r->getParameterID() << " ";
        check (mal.isEmpty() && p.getPreset() == 0 && ! p.presetEditado() && p.nomePreset() == U (ley::PRESETS[0].nome),
               "ao abrir: o primeiro preset, nos valores de partida", mal);
    }
    {   // o tempo en ms: os ecos caen no seu sitio e cada un baixa o que din as repeticións
        BLProcessor p;
        limpo (p, 250.0f, 50.0f);
        const auto o = procesar (p, golpe (sr), 1.2, 512, sr);
        const int e1 = ondePico (o, 0, (int) (0.1 * sr), (int) (0.4 * sr)), e2 = ondePico (o, 0, (int) (0.4 * sr), (int) (0.65 * sr));
        const int e3 = ondePico (o, 0, (int) (0.65 * sr), (int) (0.9 * sr));
        const int pk0 = ondePico (golpe (sr), 0, 0, (int) (0.01 * sr));
        const double ms1 = (e1 - pk0) * 1000.0 / sr, ms2 = (e2 - pk0) * 1000.0 / sr, ms3 = (e3 - pk0) * 1000.0 / sr;
        const float p1 = pico (o, (int) (0.1 * sr), (int) (0.4 * sr)), p2 = pico (o, (int) (0.4 * sr), (int) (0.65 * sr));
        const double baixa = juce::Decibels::gainToDecibels (p2 / p1);
        check (std::abs (ms1 - 250) < 0.5 && std::abs (ms2 - 500) < 0.5 && std::abs (ms3 - 750) < 0.5 && std::abs (baixa + 6.02) < 0.6
               && std::abs (p1 - 0.5f) < 0.03f, "ecos no seu tempo (ms) e repeticións",
               juce::String::formatted ("(%.1f, %.1f, %.1f ms; %.2f dB por volta)", ms1, ms2, ms3, baixa));
    }
    {   // á par do DAW: sen DAW, 120 BPM; cun DAW a 90, unha negra son 666,7 ms
        auto primeiro = [&] (int nota, double bpm)
        {
            BLProcessor p;
            Cabeza cab; cab.bpm = bpm;
            if (bpm > 0) p.setPlayHead (&cab);
            limpo (p, 100.0f, 0.0f);
            setParam (p, "sincronia", 1.0f); setParam (p, "nota", (float) nota);
            const auto o = procesar (p, golpe (sr), 1.5, 512, sr);
            p.setPlayHead (nullptr);
            return (ondePico (o, 0, (int) (0.05 * sr), (int) (1.4 * sr)) - ondePico (golpe (sr), 0, 0, (int) (0.01 * sr))) * 1000.0 / sr;
        };
        const double a = primeiro (ley::N8P, 0), b = primeiro (ley::N4, 90.0), c = primeiro (ley::N8T, 150.0);
        check (std::abs (a - 375.0) < 0.5 && std::abs (b - 666.67) < 0.5 && std::abs (c - 133.33) < 0.5, U ("á par do DAW (1/8. a 120, 1/4 a 90, 1/8T a 150)").toRawUTF8(),
               juce::String::formatted ("(%.1f, %.1f, %.1f ms)", a, b, c));
    }
    {   // pingue-pongue: o primeiro eco só pola esquerda, o segundo só pola dereita; espallar atrasa a dereita
        BLProcessor p;
        limpo (p, 200.0f, 60.0f);
        setParam (p, "tipo", (float) ley::PINGUEPONGUE);
        const auto o = procesar (p, golpe (sr), 1.0, 512, sr);
        const float l1 = pico (o, (int) (0.15 * sr), (int) (0.3 * sr), 0), r1 = pico (o, (int) (0.15 * sr), (int) (0.3 * sr), 1);
        const float l2 = pico (o, (int) (0.35 * sr), (int) (0.5 * sr), 0), r2 = pico (o, (int) (0.35 * sr), (int) (0.5 * sr), 1);
        BLProcessor q;
        limpo (q, 200.0f, 0.0f);
        setParam (q, "espallar", 100.0f);
        const auto e = procesar (q, golpe (sr), 0.6, 512, sr);
        const int k0 = ondePico (golpe (sr), 0, 0, (int) (0.01 * sr));
        const double msL = (ondePico (e, 0, (int) (0.1 * sr), (int) (0.4 * sr)) - k0) * 1000.0 / sr, msR = (ondePico (e, 1, (int) (0.1 * sr), (int) (0.4 * sr)) - k0) * 1000.0 / sr;
        check (l1 > 0.2f && r1 < 1e-4f && r2 > 0.1f && l2 < 1e-4f && std::abs (msL - 200) < 0.5 && std::abs (msR - 250) < 0.5, "pingue-pongue e espallar",
               juce::String::formatted ("(espallar: %.1f / %.1f ms)", msL, msR));
    }
    {   // inverso: un glissando que sobe sae baixando (cóntanse os pasos por cero en cada anaco)
        auto tendencia = [&] (int tipo)
        {
            BLProcessor p;
            limpo (p, 500.0f, 0.0f);
            setParam (p, "tipo", (float) tipo);
            juce::AudioBuffer<float> gl (2, (int) (0.4 * sr));
            double fase = 0;
            for (int i = 0; i < gl.getNumSamples(); ++i)
            {
                const double t = i / sr, f = 200.0 + 1800.0 * t / 0.4;
                fase += 2 * ley::PI * f / sr;
                const float w = (float) std::sin (ley::PI * t / 0.4);
                gl.setSample (0, i, 0.4f * w * (float) std::sin (fase)); gl.setSample (1, i, gl.getSample (0, i));
            }
            const auto o = procesar (p, gl, 1.6, 512, sr);
            // en anacos de 20 ms con son (despois do seco), os pasos por cero: entre anacos seguidos,
            // canto soben ou baixan (o inverso le en anacos ao revés: entre eles hai saltos, sen son)
            const int an = (int) (0.02 * sr);
            const float teito = pico (o) * 0.2f;
            int antes = -1, suma = 0;
            for (int k = (int) (0.45 * sr); k + an < o.getNumSamples(); k += an)
            {
                if (pico (o, k, k + an, 0) < teito) { antes = -1; continue; }
                int z = 0;
                for (int i = k + 1; i < k + an; ++i) if ((o.getSample (0, i - 1) < 0) != (o.getSample (0, i) < 0)) ++z;
                if (antes >= 0) suma += z - antes;
                antes = z;
            }
            return suma;
        };
        const int dix = tendencia (ley::DIXITAL), inv = tendencia (ley::INVERSO);
        check (dix > 20 && inv < -20, "inverso: o eco soa ao revés", juce::String::formatted ("(o glissando: %+d / %+d)", dix, inv));
    }
    {   // a mestura a 0 é o son tal cal; o bypass, tamén (cos ecos soando)
        const auto son = sonDeProba (sr, 1.0);
        BLProcessor a; setParam (a, "mestura", 0.0f);
        const auto seco = procesar (a, son, 1.0, 512, sr);
        BLProcessor b; setParam (b, "bypass", 1.0f); setParam (b, "volume", 6.0f);
        const auto byp = procesar (b, son, 1.0, 512, sr);
        check (diferenza (seco, son) == 0.0f && diferenza (byp, son) == 0.0f, "mestura 0 e bypass: o son tal cal",
               juce::String::formatted ("(%g, %g)", diferenza (seco, son), diferenza (byp, son)));
    }
    {   // bloques: con bloques de 512 e de 37 mostras sae o mesmo, mostra a mostra (cinta: wow, saturación, atenuación)
        auto render = [&] (int bloque)
        {
            BLProcessor p;
            p.cargarPreset (6);   // Dub
            setParam (p, "atenuacion", 40.0f); setParam (p, "espallar", 30.0f);
            return procesar (p, sonDeProba (sr), 4.0, bloque, sr);
        };
        const auto x = render (512), y = render (37);
        check (diferenza (x, y) == 0.0f && finito (x) && pico (x) > 0.05f, "mesmo son con bloques de 512 e de 37");
        std::printf ("pegada da saída: %016llx\n", (unsigned long long) pegada (x));
        if (argc > 1 && ! mode.startsWith ("--")) writeWav (juce::File (mode), x, sr);
    }
    {   // cambiar o tempo mentres soa: dixital fúndese e a cinta escorrega, sen clics
        bool ok = true; juce::String que;
        for (int tipo : { (int) ley::DIXITAL, (int) ley::CINTA, (int) ley::PINGUEPONGUE, (int) ley::INVERSO })
        {
            BLProcessor p;
            limpo (p, 250.0f, 60.0f);
            setParam (p, "tipo", (float) tipo);
            juce::AudioBuffer<float> seno (2, (int) (3 * sr));
            for (int i = 0; i < seno.getNumSamples(); ++i) { const float x = 0.3f * std::sin (2.0f * juce::MathConstants<float>::pi * 220.0f * (float) i / (float) sr); seno.setSample (0, i, x); seno.setSample (1, i, x); }
            p.setPlayConfigDetails (2, 2, sr, 512); p.prepareToPlay (sr, 512);
            juce::AudioBuffer<float> a1 (2, (int) (1.5 * sr)), a2 (2, (int) (1.5 * sr));
            for (int c = 0; c < 2; ++c) { a1.copyFrom (c, 0, seno, c, 0, a1.getNumSamples()); a2.copyFrom (c, 0, seno, c, a1.getNumSamples(), a2.getNumSamples()); }
            const auto o1 = procesar (p, a1, 1.5, 512, sr, false);
            setParam (p, "ms", 400.0f);
            const auto o2 = procesar (p, a2, 1.5, 512, sr, false);
            float salto = 0;
            for (int c = 0; c < 2; ++c)
                for (int i = 1; i < o2.getNumSamples(); ++i) salto = juce::jmax (salto, std::abs (o2.getSample (c, i) - o2.getSample (c, i - 1)));
            const float normal = juce::jmax (pico (o1, (int) (1.0 * sr)) * 2.0f * juce::MathConstants<float>::pi * 220.0f / (float) sr * 2.0f, 0.005f);
            // a cinta escorrega (o ton cambia un intre): pode ir algo máis rápido, pero sen saltos
            ok = ok && finito (o2) && salto < normal * (tipo == ley::CINTA ? 4.0f : 1.6f);
            que << juce::String (salto / normal, 2) << " ";
        }
        check (ok, "cambio de tempo sen clics", "(" + que.trim() + U (" × o normal)"));
    }
    {   // conxelar: o que está dando voltas queda (sen perder nada) e o que entra despois xa non entra
        BLProcessor p;
        p.cargarPreset (5);   // Cinta vella: wow, saturación e cortes (conxelado, nada diso)
        setParam (p, "mestura", 100.0f);
        p.setPlayConfigDetails (2, 2, sr, 512); p.prepareToPlay (sr, 512);
        const auto son = sonDeProba (sr, 1.0);
        const auto antes = procesar (p, son, 1.0, 512, sr, false);
        setParam (p, "conxelar", 1.0f);
        const auto durante = procesar (p, son, 6.0, 512, sr, false);   // outra vez o son: non debe entrar
        const double e0 = enerxia (durante, sr, 0.5, 1.5), e1 = enerxia (durante, sr, 4.5, 5.5);
        const double perda = 10.0 * std::log10 ((e1 + 1e-30) / (e0 + 1e-30));
        setParam (p, "conxelar", 0.0f);
        juce::AudioBuffer<float> silencio (2, 1);
        silencio.clear();   // (un AudioBuffer novo non vén baleiro: en Windows trae lixo)
        const auto despois = procesar (p, silencio, 12.0, 512, sr, false);
        const double fin = 10.0 * std::log10 ((enerxia (despois, sr, 11.0, 12.0) + 1e-30) / (e0 + 1e-30));
        check (perda > -1.0 && perda < 1.0 && fin < -40.0 && finito (durante) && pico (antes) > 0.01f, "conxelar: os ecos quedan e despois apáganse",
               juce::String::formatted ("(%.1f dB en 4 s; ao soltar, %.0f dB)", perda, fin));
    }
    {   // todo ao máximo (repeticións, saturación, ton, movemento) en cada tipo, 20 s: estable e acotado
        bool ok = true; juce::String que;
        for (int t = 0; t < ley::NUM_TIPOS; ++t)
        {
            BLProcessor p;
            setParam (p, "tipo", (float) t); setParam (p, "repeticions", 100.0f); setParam (p, "saturacion", 100.0f); setParam (p, "ton", 100.0f);
            setParam (p, "movemento", 100.0f); setParam (p, "mestura", 100.0f); setParam (p, "corteGraves", 20.0f); setParam (p, "corteAgudos", 20000.0f);
            setParam (p, "sincronia", 0.0f); setParam (p, "ms", 90.0f);
            auto son = sonDeProba (sr);
            son.applyGain (2.0f);
            const auto o = procesar (p, son, 20.0, 512, sr);
            const float pk = pico (o);
            ok = ok && finito (o) && pk <= 0.8913f;
            que << juce::String (pk, 2) << " ";
        }
        check (ok, U ("ao máximo: estable e baixo o teito").toRawUTF8(), "(" + que.trim() + ")");
    }
    {   // os presets: todos soan (con ecos despois do son), sen valores non numéricos e sen tocar o teito
        int mal = 0; float peor = 0; juce::String cal;
        for (int i = 0; i < ley::NUM_PRESETS; ++i)
        {
            BLProcessor p;
            p.cargarPreset (i);
            const auto o = procesar (p, sonDeProba (sr), 4.0, 512, sr);
            const float pk = pico (o), ecos = pico (o, (int) (2.05 * sr), (int) (3.5 * sr));
            if (! finito (o) || ecos < 0.003f || p.presetEditado()) { ++mal; std::printf ("   mal: %s (%.4f)\n", ley::PRESETS[i].nome, (double) ecos); }
            if (pk > peor) { peor = pk; cal = U (ley::PRESETS[i].nome); }
        }
        check (mal == 0 && peor < 0.8913f, "os presets soan", "(" + juce::String (ley::NUM_PRESETS) + U ("; o máis alto, ") + cal
               + juce::String::formatted (": %.1f dBFS)", juce::Decibels::gainToDecibels (peor)));
    }
    {   // a atenuación: mentres soa o seco, os ecos baixan
        auto ecos = [&] (float at)
        {
            BLProcessor p;
            limpo (p, 150.0f, 60.0f);
            setParam (p, "atenuacion", at);
            juce::AudioBuffer<float> tono (2, (int) (2 * sr));
            for (int i = 0; i < tono.getNumSamples(); ++i) { const float x = 0.3f * std::sin (2.0f * juce::MathConstants<float>::pi * 220.0f * (float) i / (float) sr); tono.setSample (0, i, x); tono.setSample (1, i, x); }
            return enerxia (procesar (p, tono, 2.0, 512, sr), sr, 1.0, 2.0);
        };
        const double db = 10.0 * std::log10 (ecos (100.0f) / ecos (0.0f));
        check (db < -6.0, "atenuación: os ecos baixan mentres soa o seco", juce::String::formatted ("(%.1f dB)", db));
    }
    {   // mono → estéreo: unha entrada mono sae en estéreo (o pingue-pongue abre o son)
        BLProcessor p;
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (juce::AudioChannelSet::mono()); l.outputBuses.add (juce::AudioChannelSet::stereo());
        const bool soporta = p.setBusesLayout (l);
        p.cargarPreset (13);
        setParam (p, "mestura", 100.0f);
        p.prepareToPlay (sr, 512);
        auto son = sonDeProba (sr, 1.0);
        juce::AudioBuffer<float> buf (2, 512);
        double dif = 0;
        for (int pos = 0; pos + 512 <= son.getNumSamples(); pos += 512)
        {
            buf.clear(); buf.copyFrom (0, 0, son, 0, pos, 512);
            juce::MidiBuffer m; p.processBlock (buf, m);
            for (int i = 0; i < 512; ++i) dif += std::abs (buf.getSample (0, i) - buf.getSample (1, i));
        }
        check (soporta && dif > 1.0, "mono a estéreo");
    }
    {   // estado: os parámetros, o preset e a escala gárdanse e recupéranse; soa igual
        BLProcessor a;
        a.cargarPreset (8);
        setParam (a, "ton", 33.0f); setParam (a, "volume", -3.0f);
        a.uiScale = 0.8f;
        juce::MemoryBlock mb; a.getStateInformation (mb);
        BLProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool ok = b.getPreset() == 8 && b.nomePreset() == a.nomePreset() && b.presetEditado() && std::abs (b.uiScale - 0.8f) < 1e-6f;
        const auto x = procesar (a, sonDeProba (sr), 2.5, 512, sr), y = procesar (b, sonDeProba (sr), 2.5, 512, sr);
        const float d = diferenza (x, y);
        // (os mandos con curva, coma o tempo en ms, poden volver cun bit de menos: 1e-6 é inaudible)
        check (ok && d < 1e-6f, "estado gardado/recuperado (soa igual)", "(" + juce::String ((int) mb.getSize()) + " bytes)");
    }
    {   // A/B e presets propios (do núcleo), co nome do preset
        BLProcessor p;
        p.cargarPreset (0);
        p.estado.cambiarLado();
        p.cargarPreset (10);
        p.estado.cambiarLado();
        bool ok = p.estado.lado() == 0 && p.getPreset() == 0 && ! p.presetEditado();
        p.estado.cambiarLado();
        ok = ok && p.getPreset() == 10 && p.nomePreset() == U (ley::PRESETS[10].nome);
        check (ok, "A/B (os mandos e o preset)");

        const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("barulloley_presets_proba");
        dir.deleteRecursively();
        p.estado.setCarpeta (dir);
        setParam (p, "repeticions", 77.0f);
        const bool saved = p.estado.gardar ("Proba: muro longo");
        p.cargarPreset (3);
        const auto list = p.estado.presets();
        const bool loaded = list.size() == 1 && list[0].hasFileExtension ("blpreset") && p.estado.cargar (list[0]);
        const float rep = p.apvts.getRawParameterValue ("repeticions")->load();
        check (saved && loaded && std::abs (rep - 77.0f) < 0.01f && p.getPreset() == 10, "presets propios: gardar e cargar");
        dir.deleteRecursively();
    }
    {   // desfacer e refacer: cargar un preset e mover un mando, cada un no seu paso
        BLProcessor p;
        p.undo.clearUndoHistory();
        p.estado.novoPaso(); p.cargarPreset (5); p.apvts.copyState(); p.estado.novoPaso();
        setParam (p, "repeticions", 90.0f); p.apvts.copyState(); p.estado.novoPaso();
        p.undo.undo();
        const bool u1 = p.getPreset() == 5 && ! p.presetEditado();
        p.undo.undo();
        const bool u2 = p.getPreset() == 0 && ! p.presetEditado();
        p.undo.redo(); p.undo.redo();
        check (u1 && u2 && p.getPreset() == 5 && p.presetEditado(), "desfacer e refacer (preset e mandos)");
    }
    {   // os programas do DAW son os presets; a cola dura o que duran os ecos
        BLProcessor p;
        p.setCurrentProgram (12);
        const bool prog = p.getNumPrograms() == ley::NUM_PRESETS && p.getCurrentProgram() == 12 && p.getProgramName (12) == U (ley::PRESETS[12].nome);
        p.cargarPreset (0);
        const double c0 = p.getTailLengthSeconds();
        p.cargarPreset (16);
        const double c16 = p.getTailLengthSeconds();
        check (prog && c0 > 1.0 && c0 < 5.0 && c16 > 20.0 && c16 <= 30.0, "programas do DAW = presets e a cola", juce::String::formatted ("(%.1f s, %.1f s)", c0, c16));
    }
    {   // o limitador do núcleo: un son forte, cos ecos altos e a +6 dB, non pasa de −1 dBFS; a latencia é a anticipación (1,5 ms)
        BLProcessor p;
        p.cargarPreset (10);
        setParam (p, "mestura", 60.0f); setParam (p, "volume", 6.0f);
        auto son = sonDeProba (sr);
        son.applyGain (3.0f);
        const float pk = pico (procesar (p, son, 3.0, 512, sr));
        check (pk <= 0.8913f && p.getLatencySamples() == 72, "teito a -1 dBFS, con 1,5 ms de anticipación", juce::String::formatted ("(pico %.4f; latencia %d)", pk, p.getLatencySamples()));
    }
    return fails;
}
