#!/bin/bash
# As probas de Barullo Ley na CI común (escoita-nucleo): chámase desde a raíz con
# mac, windows, linux ou sanitizadores, coa compilación en build/
set -eo pipefail
SO="$1"
EXE=""; [ "$SO" = windows ] && EXE=".exe"
CONF=Release; [ "$SO" = sanitizadores ] && CONF=RelWithDebInfo
RT="build/BLRenderTest_artefacts/$CONF/BLRenderTest$EXE"

echo "::group::Son (os tipos, o tempo, as repeticións, conxelar, os presets e o estado)"
"$RT" proba.wav
echo "::endgroup::"
