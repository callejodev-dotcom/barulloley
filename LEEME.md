**Barullo Ley** é un delay: dixital, cinta, analóxico, pingue-pongue e inverso. Para o día a día abondan un preset, o tipo e catro mandos (tempo, repeticións, ton e mestura), á par do tempo do DAW ou marcando o tempo á man. Se queres ir a máis, detrás de **MÁIS** están o espallar, a anchura, o movemento, a saturación, os cortes, a atenuación e o volume.

Unha ferramenta de [Escoita](https://www.escoita.link).

## Descargas

| Sistema | Instalador | Á man |
|---|---|---|
| **Mac** (Apple Silicon e Intel, macOS 11+): AU, VST3, CLAP e app | `BarulloLey-…-mac.pkg` | `BarulloLey-…-mac.zip` |
| **Windows** (64 bits): VST3, CLAP e app | `BarulloLey-…-windows-setup.exe` | `BarulloLey-…-windows-x64.zip` |
| **Linux** (x86_64): VST3, LV2, CLAP e app | — | `BarulloLey-…-linux-x86_64.tar.gz` → `./instalar.sh` |

Se o sistema avisa ao abrilo:
- **Mac:** clic dereito sobre o instalador → *Abrir* → *Abrir*.
- **Windows:** «Windows protexeu o teu PC» → *Máis información* → *Executar de todos os xeitos*.

Avisan porque os instaladores non levan sinatura de pago, non porque haxa nada raro.

## Que ten

- **20 presets de fábrica**, en grupos: *Clásicos* (Corchea con puntillo, Negras, Voz con eco, Slapback, Eco longo), *Cinta* (Cinta vella, Dub, Rockabilly, Cinta gastada), *Analóxico* (Pedal escuro, Muro, Chorus de eco, Morno), *Espazo* (Pingue-pongue, Tresillos, Nube, Infinito) e *Inverso* (Ao revés, Espello, Soño). Son tamén os programas do DAW.
- **O tipo**:
  - DIXITAL: limpo; ao cambiar o tempo, os dous tempos fúndense (sen glissando);
  - CINTA: unha máquina de cinta, con wow e flutter, saturación e menos agudos en cada volta; ao cambiar o tempo, a cinta acelera ou frea e o ton escorrega;
  - ANALÓXICO: un pedal de BBD, escuro e morno, cun chisco de chorus;
  - PINGUE-PONGUE: os ecos saltan dun lado ao outro;
  - INVERSO: cada eco soa ao revés.
- **Os catro mandos**: *Tempo* (en figuras do DAW, de 1/32 a 1/1, con tresillos e puntillos; ou en ms, de 1 a 2000), *Repeticións*, *Ton* (escuro, coma entra ou fino e brillante) e *Mestura* (con potencia constante).
- **TEMPO DO DAW**: o eco vai á par do tempo do DAW (sen DAW, a 120 BPM). Apagado, o tempo vai en ms.
- **MARCAR**: preme dúas ou máis veces ao ritmo e o eco colle ese tempo.
- **CONXELAR**: o que está a repetirse queda dando voltas sen fin (sen perder nada) e xa non entra son novo.
- **MÁIS**:
  - *As repeticións*: espallar (o eco da dereita, ata un 25 % máis tarde), anchura, movemento (wow e flutter) e saturación;
  - *O son*: corte de graves e de agudos (en cada volta), atenuación (os ecos baixan mentres soa o seco) e volume.
- **A pantalla**: os ecos no tempo, cada un na súa altura e no seu lado (no inverso, cada un medra ata o seu sitio), cunha liña que corre ao tempo real; o tempo e os niveis de entrada, ecos e saída.
- **Presets propios, A/B e desfacer** (do núcleo común): os presets van en `.blpreset`, en Documentos/Barullo Ley/Presets. Cada clic é un paso de desfacer.
- **Bypass** de verdade: o son pasa tal cal e os ecos seguen a soar por dentro.
- Un **limitador de seguridade** á saída: nunca pasa de −1 dBFS.
- Entrada mono ou estéreo; sen latencia.

## Como se fixo

`Source/Eco.h`, sen JUCE:

```
entrada ─┬─────────────── (conxelado: non entra) ───────────────────────────┐
         └─ + ─ liña (L e R) ─ ler (movéndose) ─ ton ─ cortes ─ cor do tipo ─┬─ saída (ecos)
            └──────────── repeticións ──────────────────────────────────────┘
```

- **Dúas liñas** (esquerda e dereita) de ata 5 s, lidas con interpolación de Hermite (pouca perda de agudos).
- **O ton** separa cada volta en graves e agudos (900 Hz) e baixa uns ou outros; despois, os cortes e a cor do tipo (os agudos da cinta e do BBD, e a súa saturación).
- **Cambiar o tempo**: no dixital e no pingue-pongue, dúas lecturas fúndense en 50 ms; na cinta e no analóxico, o tempo esvara (como moito, unhas −1,3 / +0,8 oitavas).
- **O inverso**: dúas cabezas, media volta unha da outra, len cara atrás un anaco do tamaño do eco, cunha xanela sen² (as dúas suman sempre 1).
- **Conxelado**, as liñas quedan quedas, lense sen interpolar e a volta non pasa polos filtros: os ecos non perden nada.
- Dentro da volta, unha protección suave (igual ata ±1, nunca pasa de ±2): con todo ao máximo non se dispara.

## Estrutura

| Ficheiro | Que é |
|---|---|
| `Source/Eco.h` | O delay |
| `Source/Presets.h` | Os presets de fábrica |
| `Source/PluginProcessor.*` | O plugin: parámetros, tempo do DAW, presets, estado e a mestura |
| `Source/PluginEditor.*` | A interface |
| `tools/RenderTest.cpp` | A proba |

Usa o núcleo común de Escoita ([escoita-nucleo](https://github.com/callejodev-dotcom/escoita-nucleo), v0.3.0): o texto en galego, o aviso das versións novas, os presets propios, o A/B e o desfacer (`escoita::Estado`), o limitador de seguridade (`escoita::Teito`), a CI e os instaladores.

## Probas

- **Ao abrir**: o primeiro preset, cos parámetros nos seus valores de partida.
- **Tempo**: os ecos caen no seu sitio (250, 500, 750 ms) e cada volta baixa o que din as repeticións.
- **Á par do DAW**: 1/8. a 120 BPM (sen DAW), 1/4 a 90 e 1/8T a 150.
- **Pingue-pongue e espallar**: o primeiro eco só pola esquerda e o segundo só pola dereita; espallar atrasa a dereita un 25 %.
- **Inverso**: un glissando que sobe sae baixando.
- **Mestura a 0 e bypass**: o son sae tal cal, bit a bit.
- **Bloques**: con bloques de 512 e de 37 mostras sae o mesmo, mostra a mostra (e a súa pegada).
- **Cambio de tempo** mentres soa, en cada tipo: sen clics.
- **Conxelar**: os ecos non perden nada en 4 s e despois apáganse.
- **Todo ao máximo**, en cada tipo e 20 s: estable e baixo o teito.
- **Os 20 presets** soan, sen valores non numéricos e sen tocar o teito.
- **Atenuación**, **mono a estéreo**, **estado**, **A/B**, **presets propios**, **desfacer**, **programas do DAW e cola** e **limitador**.
- **Validación**: pluginval ao nivel máximo (10), auval no Mac e clap-validator.

Na CI van en Mac, Windows e Linux, e tamén con AddressSanitizer e UBSan (`tools/probas-ci.sh`).

## Compilar

Nun Mac: `./build_mac.sh` (compila e instala no teu usuario). Á man:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBARULLO_TESTS=ON
cmake --build build --config Release
build/BLRenderTest_artefacts/Release/BLRenderTest
```

Outras ordes da proba: `--snapshot saida.png [mais] [preset]` (a interface), `--render saida.wav [preset]` (un preset cun son de proba) e `--demo preset entrada.wav saida.wav` (un preset sobre un son teu).

## Publicar unha versión

```
git tag v0.1.0 && git push origin v0.1.0
```

A versión sae da liña `project(BarulloLey VERSION …)` de `CMakeLists.txt`; o texto da versión, de `packaging/NOTAS-VERSION.md`.

## Créditos e licenzas

- **Barullo Ley é software libre**, baixo a licenza **GNU Affero General Public License v3.0** (ficheiro `LICENSE`): podes usalo, estudalo, modificalo e compartilo; se distribúes unha versión modificada, ten que ir co seu código e coa mesma licenza. Usa JUCE 8 coa súa licenza AGPLv3.
- Tipografías Montserrat e Inter (licenza OFL, `Resources/Montserrat-OFL.txt` e `Resources/Inter-OFL.txt`).
