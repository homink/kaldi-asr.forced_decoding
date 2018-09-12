# kaldi-asr.forced_decoding
Perform the forced decoding with target transcription

1. Install Kaldi
https://github.com/kaldi-asr/kaldi/blob/master/INSTALL

2. Copy files in ths repository to the corresponding directories

3. Build $KALDI_ROOT/src

4. Run the following script and you will see results on the screen.

```
./run.sh 
```
```
online2-wav-nnet3-latgen-faster-force --do-endpointing=true --frames-per-chunk=20 --extra-left-context-initial=0 --online=true --frame-subsampling-factor=3 --config=/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/new_lm_mix/conf/online.conf --min-active=200 --max-active=6000 --beam=11 --lattice-beam=6.0 --acoustic-scale=1.0 --word-symbol-table=/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/new_lm_mix/graph/words.txt /media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/new_lm_mix/final.mdl /media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/new_lm_mix/graph/HCLG.fst /media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/wav.scp /media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/post2.txt 'WOULD YOU LIKE CHICKEN OR BEEF' 
LOG (online2-wav-nnet3-latgen-faster-force[5.2.107~4-e8928]:ComputeDerivedVars():ivector-extractor.cc:183) Computing derived variables for iVector extractor
LOG (online2-wav-nnet3-latgen-faster-force[5.2.107~4-e8928]:ComputeDerivedVars():ivector-extractor.cc:204) Done.
LOG (online2-wav-nnet3-latgen-faster-force[5.2.107~4-e8928]:RemoveSomeNodes():nnet-nnet.cc:916) Removed 2 orphan nodes.
LOG (online2-wav-nnet3-latgen-faster-force[5.2.107~4-e8928]:RemoveOrphanComponents():nnet-nnet.cc:841) Removing 2 orphan components.
LOG (online2-wav-nnet3-latgen-faster-force[5.2.107~4-e8928]:Collapse():nnet-utils.cc:770) Added 1 components, removed 2
LOG (online2-wav-nnet3-latgen-faster-force[5.2.107~4-e8928]:CompileLooped():nnet-compile-looped.cc:336) Spent 0.171707 seconds in looped compilation.
wav-copy scp,p:/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/wav.scp ark:- 
LOG (wav-copy[5.2.107~4-e8928]:main():wav-copy.cc:68) Copied 1 wave files
extend-wav-with-silence ark:- ark:- 
LOG (extend-wav-with-silence[5.2.107~4-e8928]:main():extend-wav-with-silence.cc:107) Successfully extended 1 files.
LOG (online2-wav-nnet3-latgen-faster-force[5.2.107~4-e8928]:main():online2-wav-nnet3-latgen-faster-force.cc:321) num_post : 15
0       11      0.982088        WOULD
0       7       0.01791206      WHAT
7       4       0.01791207      WOULD
11      7       0.01791206      YOU
11      7       0.982088        YOU
18      10      1       LIKE
28      12      1       CHICKEN
40      11      0.4251907       ARE
40      11      0.4251907       OUR
40      7       0.1461729       OR
40      11      0.003445635     R
51      33      0.003445629     BEEF
47      37      0.1461723       BEEF
51      33      0.4251899       BEEF
51      33      0.4251899       BEEF
WOULD   0.982088
YOU     0.982088
LIKE    1
CHICKEN 1
OR      0.1461729
BEEF    0.4251899
0.7559231
```

The decoding process can be traceable in the [log file](https://github.com/homink/kaldi-asr.forced_decoding/blob/master/decoding.log) and in [the code lines](https://github.com/homink/kaldi-asr.forced_decoding/blob/master/kaldi_nnet3_online_decoding.pdf). More details are described in [here](http://homepages.inf.ed.ac.uk/aghoshal/pubs/icassp12-lattices.pdf).
