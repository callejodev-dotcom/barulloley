// Presets.h — Os presets de fábrica de Barullo Ley, por grupos. O primeiro é o de partida (os
// parámetros saen del). Os que van á par do DAW levan tamén os ms a 120 BPM (por se se solta).
#pragma once
#include "Eco.h"

namespace ley {

struct Preset
{
    const char* nome;
    const char* grupo;
    const char* descricion;
    int tipo;
    bool sincronia;
    int nota;
    double ms, repeticions, ton, mestura, espallar, anchura, movemento, saturacion, corteGraves, corteAgudos, atenuacion;
};

// as figuras (índices de NOTAS)
enum { N32 = 0, N16T, N16, N16P, N8T, N8, N8P, N4T, N4, N4P, N2T, N2, N2P, N1 };

inline const Preset PRESETS[] = {
    //                                                    tipo          sinc  nota  ms    rep   ton   mest  espa  anch  mov   sat   corteG corteA aten
    { "Corchea con puntillo", "Clásicos", "O eco de sempre: unha corchea con puntillo, limpo, por detrás da guitarra ou da voz.",
      DIXITAL,      true,  N8P, 375, 0.35, 0.50, 0.25, 0.00, 1.00, 0.10, 0.00,  80, 12000, 0.0 },
    { "Negras", "Clásicos", "Un eco por negra, ben colocado no tempo.",
      DIXITAL,      true,  N4,  500, 0.30, 0.50, 0.22, 0.00, 1.00, 0.10, 0.00,  80, 12000, 0.0 },
    { "Voz con eco", "Clásicos", "Unha corchea que baixa mentres cantas e aparece nos ocos.",
      DIXITAL,      true,  N8,  250, 0.30, 0.45, 0.28, 0.10, 1.00, 0.10, 0.00, 150,  9000, 0.5 },
    { "Slapback", "Clásicos", "Un só rebote moi curto, coma nos discos dos cincuenta.",
      CINTA,        false, N4,  110, 0.05, 0.50, 0.30, 0.00, 0.60, 0.30, 0.20, 100,  9000, 0.0 },
    { "Eco longo", "Clásicos", "Unha branca que vai e volve moitas veces: para notas soltas.",
      DIXITAL,      true,  N2, 1000, 0.60, 0.45, 0.25, 0.15, 1.10, 0.20, 0.00, 100, 10000, 0.2 },

    { "Cinta vella", "Cinta", "Unha máquina de cinta: cada volta máis morna e cun chisco de wow.",
      CINTA,        true,  N4,  500, 0.45, 0.40, 0.28, 0.00, 1.00, 0.45, 0.30,  80,  9000, 0.0 },
    { "Dub", "Cinta", "Moitas repeticións escuras que se van saturando: para caixas e acordes soltos.",
      CINTA,        true,  N4P, 750, 0.72, 0.30, 0.30, 0.10, 1.10, 0.40, 0.50, 200,  7000, 0.0 },
    { "Rockabilly", "Cinta", "Un slapback de cinta, con corpo, para guitarras e voces.",
      CINTA,        false, N4,   90, 0.15, 0.55, 0.35, 0.00, 0.50, 0.25, 0.35, 120, 10000, 0.0 },
    { "Cinta gastada", "Cinta", "A cinta xa está cansa: o ton balancea e os ecos escurecen axiña.",
      CINTA,        true,  N8,  250, 0.50, 0.30, 0.30, 0.00, 1.00, 0.90, 0.40, 100,  6000, 0.0 },

    { "Pedal escuro", "Analóxico", "Un pedal de BBD: os ecos morren mornos e escuros.",
      ANALOXICO,    true,  N8,  250, 0.45, 0.40, 0.30, 0.00, 1.00, 0.30, 0.20,  80,  8000, 0.0 },
    { "Muro", "Analóxico", "Semicorcheas que se amontoan nun muro morno.",
      ANALOXICO,    true,  N16, 125, 0.75, 0.35, 0.22, 0.15, 1.20, 0.35, 0.30, 120,  8000, 0.3 },
    { "Chorus de eco", "Analóxico", "Ecos curtos que se moven: o son abre e respira.",
      ANALOXICO,    false, N4,  180, 0.35, 0.50, 0.30, 0.30, 1.30, 0.80, 0.10, 100,  9000, 0.0 },
    { "Morno", "Analóxico", "Unha negra suave e baixiña, para pór fondo sen que se note.",
      ANALOXICO,    true,  N4,  500, 0.35, 0.35, 0.18, 0.10, 1.00, 0.25, 0.10, 150,  7000, 0.3 },

    { "Pingue-pongue", "Espazo", "Os ecos saltan dun lado ao outro, en corcheas.",
      PINGUEPONGUE, true,  N8,  250, 0.45, 0.50, 0.28, 0.00, 1.00, 0.10, 0.00, 100, 12000, 0.0 },
    { "Tresillos", "Espazo", "Corcheas de tresillo dun lado ao outro: un balanceo.",
      PINGUEPONGUE, true,  N8T, 167, 0.45, 0.55, 0.25, 0.00, 1.00, 0.10, 0.00, 120, 12000, 0.0 },
    { "Nube", "Espazo", "Moitas repeticións, abertas e movéndose: case unha reverb.",
      PINGUEPONGUE, true,  N8P, 375, 0.75, 0.35, 0.32, 0.30, 1.30, 0.50, 0.10, 150,  8000, 0.2 },
    { "Infinito", "Espazo", "As repeticións case non acaban (con CONXELAR, xa non acaban).",
      DIXITAL,      true,  N2, 1000, 0.92, 0.40, 0.32, 0.20, 1.20, 0.20, 0.00, 150,  9000, 0.0 },

    { "Ao revés", "Inverso", "Cada eco soa ao revés: o son vén cara ti.",
      INVERSO,      true,  N4,  500, 0.30, 0.50, 0.35, 0.00, 1.00, 0.00, 0.00,  80, 12000, 0.0 },
    { "Espello", "Inverso", "Brancas ao revés que se repiten: para guitarras e voces que soñan.",
      INVERSO,      true,  N2, 1000, 0.50, 0.45, 0.35, 0.20, 1.20, 0.20, 0.00, 100, 10000, 0.2 },
    { "Soño", "Inverso", "Ao revés, escuro e morno: un fondo que respira.",
      INVERSO,      true,  N4P, 750, 0.60, 0.30, 0.38, 0.25, 1.30, 0.30, 0.20, 150,  7000, 0.3 },
};

constexpr int NUM_PRESETS = (int) (sizeof (PRESETS) / sizeof (PRESETS[0]));

} // namespace ley
