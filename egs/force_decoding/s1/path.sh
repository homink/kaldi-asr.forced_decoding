export KALDI_ROOT=`pwd`/../../..
[ -f $KALDI_ROOT/tools/env.sh ] && . $KALDI_ROOT/tools/env.sh


export SEQUITUR=$KALDI_ROOT/tools/sequitur-g2p
export LIBLBFGS=$KALDI_ROOT/tools/liblbfgs-1.10/lib
export SRILM=$KALDI_ROOT/tools/srilm

export LD_LIBRARY_PATH=$LIBLBFGS:$KALDI_ROOT/tools/openfst-1.6.3/lib:$LD_LIBRARY_PATH
export PATH=$KALDI_ROOT/tools/openfst-1.6.3/bin:$SEQUITUR/bin:$SRILM/bin:$SRILM/bin/i686-m64:$KALDI_ROOT/tools/kaldi_lm:$PWD/utils/:$KALDI_ROOT/tools/openfst/bin:$PWD:$KALDI_ROOT/tools/sph2pipe_v2.5:$PATH
[ ! -f $KALDI_ROOT/tools/config/common_path.sh ] && echo >&2 "The standard file $KALDI_ROOT/tools/config/common_path.sh is not present -> Exit!" && exit 1
. $KALDI_ROOT/tools/config/common_path.sh

PYTHON="python2.7"
sequitur=$KALDI_ROOT/tools/sequitur/g2p.py
sequitur_path="$(dirname $sequitur)/lib/$PYTHON/site-packages"

export LC_ALL=C
