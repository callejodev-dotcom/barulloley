// Eco.h — O delay de Barullo Ley, sen JUCE.
//
//   entrada ─┬─────────────── (conxelado: non entra) ───────────────┐
//            └─ + ─ liña (L e R) ─ ler (movéndose) ─ ton ─ cortes ─ cor do tipo ─┬─ saída (ecos)
//               └──────────── repeticións ─────────────────────────────────────┘
//
// Os tipos:
//   DIXITAL       limpo: cada eco igual ca o anterior, máis baixo; ao cambiar o tempo, fúndese
//   CINTA         unha máquina de cinta: wow e flutter, saturación e menos agudos en cada volta;
//                 ao cambiar o tempo, a cinta acelera ou frea (o ton sobe ou baixa)
//   ANALOXICO     un pedal de BBD: escuro, morno e un chisco de chorus
//   PINGUEPONGUE  os ecos saltan dun lado ao outro
//   INVERSO       cada eco soa ao revés (dúas cabezas que se cruzan)
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace ley {

constexpr double PI = 3.14159265358979323846;

enum Tipo { DIXITAL = 0, CINTA, ANALOXICO, PINGUEPONGUE, INVERSO, NUM_TIPOS };

// as figuras para ir á par do DAW, en negras
constexpr int NUM_NOTAS = 14;
inline const char* const NOTAS[NUM_NOTAS] = { "1/32", "1/16T", "1/16", "1/16.", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2T", "1/2", "1/2.", "1/1" };
constexpr double NEGRAS[NUM_NOTAS] = { 0.125, 1.0 / 6.0, 0.25, 0.375, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 4.0 / 3.0, 2.0, 3.0, 4.0 };

constexpr double TEMPO_MAX_MS = 4000.0;     // un eco, como moito, catro segundos despois
constexpr double INVERSO_MAX_MS = 2000.0;   // ao revés, dous (a cabeza le ata o dobre)

struct Axustes
{
    int tipo = DIXITAL;
    double tempo = 375;          // ms (xa calculado: á par do DAW ou libre)
    double repeticions = 0.35;   // 0..1 (a 1, os ecos case non se apagan)
    double ton = 0.5;            // 0 escuro, 0,5 neutro, 1 fino e brillante
    double espallar = 0;         // o eco da dereita, ata un 25 % máis tarde
    double anchura = 1.0;        // 0..1,5
    double movemento = 0.2;      // wow e flutter
    double saturacion = 0;
    double corteGraves = 60;     // Hz, nas repeticións
    double corteAgudos = 16000;  // Hz
    double atenuacion = 0;       // os ecos baixan mentres soa a entrada
    bool conxelar = false;
};

// liña de atraso: a mostra máis nova está en atras = 0
struct Linea
{
    std::vector<float> b;
    int w = 0;
    void preparar (int max) { b.assign ((size_t) std::max (8, max + 4), 0.0f); w = 0; }
    void limpar() { std::fill (b.begin(), b.end(), 0.0f); }
    void escribir (float x) { if (--w < 0) w = (int) b.size() - 1; b[(size_t) w] = x; }
    float at (int i) const { int a = w + i; const int n = (int) b.size(); if (a >= n) a -= n; return b[(size_t) a]; }
    // Hermite de catro puntos: pouca perda de agudos ao ler entre mostras
    float ler (double atras) const
    {
        atras = std::clamp (atras, 1.0, (double) b.size() - 4.0);
        const int i = (int) atras;
        const float f = (float) (atras - i);
        const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
    float lerEnteiro (int atras) const { return at (std::clamp (atras, 0, (int) b.size() - 1)); }
};

struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    double run (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void paso (double fs, double f, bool alto, double q = 0.7071)   // paso alto ou baixo (RBJ)
    {
        const double w = 2 * PI * std::clamp (f, 10.0, fs * 0.45) / fs, c = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
        if (alto) { b0 = (1 + c) / 2 / a0; b1 = -(1 + c) / a0; b2 = b0; }
        else      { b0 = (1 - c) / 2 / a0; b1 = (1 - c) / a0;  b2 = b0; }
        a1 = -2 * c / a0; a2 = (1 - al) / a0;
    }
};

class Eco
{
public:
    void preparar (double frecuencia)
    {
        fs = frecuencia;
        for (auto& l : linas) l.preparar ((int) ((TEMPO_MAX_MS * 1.25 + 60.0) / 1000.0 * fs));
        limpar();
        primeira = true;
        axustar (a);
    }

    void limpar()
    {
        for (auto& l : linas) l.limpar();
        for (auto& c : canles) c = Canle();
        env = 0; faseWow = 0; faseFlutter = 0;
        primeira = true;
    }

    // unha vez por bloque
    void axustar (const Axustes& x)
    {
        a = x;
        const double maxMs = a.tipo == INVERSO ? INVERSO_MAX_MS : TEMPO_MAX_MS;
        const double t = std::clamp (a.tempo, 1.0, maxMs);
        obx[0] = t * fs / 1000.0;
        obx[1] = obx[0] * (1.0 + 0.25 * std::clamp (a.espallar, 0.0, 1.0));
        // as repeticións: 0..1 → ganancia; o ton e os cortes, en cada volta
        g = std::clamp (a.repeticions, 0.0, 1.0);
        const double ton = std::clamp (a.ton, 0.0, 1.0);
        gAgudos = ton < 0.5 ? 0.2 + 1.6 * ton : 1.0;
        gGraves = ton < 0.5 ? 1.0 : 1.0 - 1.3 * (ton - 0.5);
        cTon = 1.0 - std::exp (-2 * PI * 900.0 / fs);
        const double lpTipo = a.tipo == CINTA ? 7000.0 : a.tipo == ANALOXICO ? 3200.0 : 22000.0;
        for (auto& c : canles)
        {
            c.hp.paso (fs, std::clamp (a.corteGraves, 20.0, 1000.0), true);
            c.lp.paso (fs, std::min (std::clamp (a.corteAgudos, 1000.0, 20000.0), lpTipo), false);
        }
        // o que move: wow (lento) e flutter (rápido), segundo o tipo
        const double mov = std::clamp (a.movemento, 0.0, 1.0);
        wow = mov * (a.tipo == CINTA ? 2.2 : a.tipo == ANALOXICO ? 0.9 : 0.3) * fs / 1000.0;
        flutter = mov * (a.tipo == CINTA ? 0.12 : a.tipo == ANALOXICO ? 0.05 : 0.0) * fs / 1000.0;
        drive = std::clamp (a.saturacion, 0.0, 1.0) * 2.5 + (a.tipo == CINTA ? 0.6 : a.tipo == ANALOXICO ? 0.35 : 0.0);
        envAtaque = 1.0 - std::exp (-1.0 / (0.005 * fs));
        envSolta = std::exp (-1.0 / (0.3 * fs));
        fundido = 1.0 / (0.05 * fs);   // 50 ms ao cambiar de tempo (dixital)
        glide = 1.0 - std::exp (-1.0 / (0.12 * fs));   // a cinta tarda uns 120 ms en chegar
    }

    // unha mostra: entran l e r e saen só os ecos
    void procesar (float& l, float& r)
    {
        const double inL = l, inR = r;
        if (primeira) { for (int c = 0; c < 2; ++c) canles[(size_t) c].t = obx[c]; primeira = false; }

        const double pk = std::max (std::abs (inL), std::abs (inR));
        env = pk > env ? env + (pk - env) * envAtaque : env * envSolta;
        const double envDb = 20.0 * std::log10 (env + 1e-9);
        const double duck = a.atenuacion > 0 ? std::pow (10.0, -a.atenuacion * 24.0 * std::clamp ((envDb + 40.0) / 40.0, 0.0, 1.0) / 20.0) : 1.0;

        // wow e flutter (quedos se está conxelado)
        if (! a.conxelar)
        {
            faseWow += 0.55 / fs; if (faseWow >= 1) faseWow -= 1;
            faseFlutter += 6.3 / fs; if (faseFlutter >= 1) faseFlutter -= 1;
        }
        const double mod = wow * 0.5 * (1.0 - std::cos (2 * PI * faseWow)) + flutter * 0.5 * (1.0 - std::cos (2 * PI * faseFlutter));

        double eco[2];
        for (int c = 0; c < 2; ++c) eco[c] = ler (c, mod);

        // o son de cada volta: ton, cortes e a cor do tipo (conxelado, nada: a volta queda igual)
        double fb[2];
        for (int c = 0; c < 2; ++c)
        {
            auto& k = canles[(size_t) c];
            double y = eco[c];
            if (! a.conxelar)
            {
                k.ton += (y - k.ton) * cTon;
                y = k.ton * gGraves + (y - k.ton) * gAgudos;
                y = k.lp.run (k.hp.run (y));
                if (drive > 0.01) y = std::tanh (drive * y) / drive;
            }
            fb[c] = y;
        }

        // o que se escribe: a entrada e as repeticións
        const double ent = a.conxelar ? 0.0 : 1.0, gv = a.conxelar ? 1.0 : g;
        double wL, wR;
        if (a.tipo == PINGUEPONGUE) { wL = ent * 0.5 * (inL + inR) + gv * fb[1]; wR = gv * fb[0]; }
        else { wL = ent * inL + gv * fb[0]; wR = ent * inR + gv * fb[1]; }
        linas[0].escribir ((float) segura (wL)); linas[1].escribir ((float) segura (wR));

        // a saída: os ecos (xa coa súa cor), a anchura e a atenuación
        const double mid = 0.5 * (fb[0] + fb[1]), side = 0.5 * (fb[0] - fb[1]) * std::clamp (a.anchura, 0.0, 1.5);
        l = (float) ((mid + side) * duck); r = (float) ((mid - side) * duck);
    }

    const Axustes& axustes() const { return a; }

    // para a pantalla: cando soa o eco n (1, 2…) e por que lado (0 L, 1 R, 2 os dous), e canto
    static double tempoEco (const Axustes& x, int n, int lado)
    {
        const double t = std::clamp (x.tempo, 1.0, x.tipo == INVERSO ? INVERSO_MAX_MS : TEMPO_MAX_MS);
        const double tr = t * (1.0 + 0.25 * std::clamp (x.espallar, 0.0, 1.0));
        if (x.tipo == PINGUEPONGUE)   // L, R, L, R…: o eco n pasou n−1 veces pola liña contraria
        {
            const int nL = (n + 1) / 2, nR = n / 2;   // voltas por cada liña
            return nL * t + nR * tr;
        }
        return n * (lado == 1 ? tr : t);
    }
    static int ladoEco (const Axustes& x, int n) { return x.tipo == PINGUEPONGUE ? ((n & 1) ? 0 : 1) : 2; }
    static double nivelEco (const Axustes& x, int n)
    {
        if (x.conxelar) return 1.0;
        const double ton = std::clamp (x.ton, 0.0, 1.0);
        // perda aproximada de cada volta polo ton e a cor do tipo (para o debuxo)
        const double perda = (ton < 0.5 ? 0.9 + 0.2 * ton : 1.0 - 0.25 * (ton - 0.5)) * (x.tipo == CINTA ? 0.93 : x.tipo == ANALOXICO ? 0.9 : 1.0);
        return std::pow (std::clamp (x.repeticions, 0.0, 1.0) * perda, n - 1) * perda;
    }

private:
    struct Canle
    {
        double t = 0, tNovo = 0, xf = 0, ton = 0, fase = 0;   // o tempo (en mostras), o seguinte, o fundido, o ton e a fase do inverso
        Biquad hp, lp;
    };

    // ler un eco segundo o tipo
    double ler (int c, double mod)
    {
        auto& k = canles[(size_t) c];
        const auto& li = linas[(size_t) c];
        if (a.conxelar)   // quedo e sen interpolar: a volta non perde nada
            return li.lerEnteiro ((int) std::lround (k.t + mod));
        if (a.tipo == CINTA || a.tipo == ANALOXICO)   // o tempo esvara: o ton sobe ou baixa
        {
            k.t += std::clamp ((obx[c] - k.t) * glide, -0.6, 0.6);   // (como moito, uns −1,3 / +0,8 oitavas)
            return li.ler (k.t + mod);
        }
        if (a.tipo == INVERSO)
        {
            // dúas cabezas, media volta unha da outra: cada unha le cara atrás un anaco do tamaño
            // do eco (o atraso vai de 0 ao dobre), cunha xanela sen² (as dúas suman sempre 1)
            k.t += (obx[c] - k.t) * glide;
            double s = 0;
            for (int h = 0; h < 2; ++h)
            {
                double p = k.fase + 0.5 * h; if (p >= 1) p -= 1;
                const double w = std::sin (PI * p);
                s += li.ler (2.0 * p * k.t + mod + 1.0) * w * w;
            }
            k.fase += 1.0 / std::max (1.0, k.t);
            if (k.fase >= 1) k.fase -= 1;
            return s;
        }
        // dixital e pingue-pongue: ao cambiar o tempo, as dúas lecturas fúndense (sen glissando)
        if (k.xf <= 0 && std::abs (obx[c] - k.t) > 0.5) { k.tNovo = obx[c]; k.xf = 1e-9; }
        if (k.xf > 0)
        {
            const double v = li.ler (k.t + mod) * std::cos (k.xf * PI / 2) + li.ler (k.tNovo + mod) * std::sin (k.xf * PI / 2);
            k.xf += fundido;
            if (k.xf >= 1) { k.t = k.tNovo; k.xf = 0; }
            return v;
        }
        return li.ler (k.t + mod);
    }

    // protección dentro da volta: igual ata ±1 e despois satura suave (nunca pasa de ±2)
    static double segura (double x)
    {
        const double ax = std::abs (x);
        return ax <= 1.0 ? x : std::copysign (1.0 + std::tanh (ax - 1.0), x);
    }

    double fs = 48000;
    Axustes a;
    std::array<Linea, 2> linas;
    std::array<Canle, 2> canles;
    double obx[2] = { 0, 0 };
    double g = 0, gAgudos = 1, gGraves = 1, cTon = 0.1, wow = 0, flutter = 0, drive = 0;
    double env = 0, envAtaque = 0.01, envSolta = 0.999, fundido = 0.001, glide = 0.001;
    double faseWow = 0, faseFlutter = 0;
    bool primeira = true;
};

} // namespace ley
