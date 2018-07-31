#!/bin/bash
. ./cmd.sh
. ./path.sh

model=/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/new_lm_mix
wav_input="100101.G_TTS.wav"

echo "utterance-id1 $PWD/$wav_input" > $PWD/wav.scp

online2-wav-nnet3-latgen-faster-force \
    --do-endpointing=true \
    --frames-per-chunk=20 \
    --extra-left-context-initial=0 \
    --online=true \
    --frame-subsampling-factor=3 \
    --config=$model/conf/online.conf \
    --min-active=200 \
    --max-active=6000 \
    --beam=11 \
    --lattice-beam=6.0 \
    --acoustic-scale=1.0 \
    --word-symbol-table=/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/new_lm_mix/graph/words.txt \
    $model/final.mdl \
    $model/graph/HCLG.fst \
    $PWD/wav.scp \
    $PWD/result.txt \
    "WOULD YOU LIKE CHICKEN OR BEEF"

cat result.txt
