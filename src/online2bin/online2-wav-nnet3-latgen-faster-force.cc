// online2bin/online2-wav-nnet3-latgen-faster.cc

// Copyright 2014  Johns Hopkins University (author: Daniel Povey)
//           2016  Api.ai (Author: Ilya Platonov)

// See ../../COPYING for clarification regarding multiple authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// THIS CODE IS PROVIDED *AS IS* BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION ANY IMPLIED
// WARRANTIES OR CONDITIONS OF TITLE, FITNESS FOR A PARTICULAR PURPOSE,
// MERCHANTABLITY OR NON-INFRINGEMENT.
// See the Apache 2 License for the specific language governing permissions and
// limitations under the License.

#include "feat/wave-reader.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "util/kaldi-thread.h"
#include "nnet3/nnet-utils.h"


#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "lat/kaldi-lattice.h"

namespace kaldi {

class ArcPosteriorComputer {
 public:
  // Note: 'clat' must be topologically sorted.
  ArcPosteriorComputer(const CompactLattice &clat,
                       BaseFloat min_post):
      clat_(clat), min_post_(min_post) { }

  // returns the number of arc posteriors that it output.
  int32 OutputPosteriors(std::string target_trans,
                         const fst::SymbolTable *word_syms,
                         std::ostream &os) {
    
    std::vector<std::string> split_trans;
    SplitStringToVector(target_trans, " \t\r", true, &split_trans);
    //for (int32 i=0;i <split_trans.size();i++ ) KALDI_LOG << split_trans[i];
    BaseFloat post[split_trans.size()];

    int32 num_post = 0;
    if (!ComputeCompactLatticeAlphas(clat_, &alpha_))
      return num_post;
    if (!ComputeCompactLatticeBetas(clat_, &beta_))
      return num_post;

    CompactLatticeStateTimes(clat_, &state_times_);
    if (clat_.Start() < 0)
      return 0;
    double tot_like = beta_[clat_.Start()];

    int32 num_states = clat_.NumStates();
    std::vector<int32> words;
    for (int32 state = 0; state < num_states; state++) {
      for (fst::ArcIterator<CompactLattice> aiter(clat_, state);
           !aiter.Done(); aiter.Next()) {
        const CompactLatticeArc &arc = aiter.Value();
        double arc_loglike = -ConvertToCost(arc.weight) +
            alpha_[state] + beta_[arc.nextstate] - tot_like;
        //os << arc.weight << '\n';
        //os << -ConvertToCost(arc.weight) << '\t' << alpha_[state] << '\t' << beta_[arc.nextstate] << '\t' << -tot_like << '\n';
        //os << arc_loglike << '\n';

        KALDI_ASSERT(arc_loglike < 0.1 &&
                     "Bad arc posterior in forward-backward computation");
        if (arc_loglike > 0.0) arc_loglike = 0.0;
        int32 num_frames = arc.weight.String().size(),
            word = arc.ilabel;
        BaseFloat arc_post = exp(arc_loglike);
        if (arc_post <= min_post_) continue;

        if (word_syms != NULL) {
          std::string word_str = word_syms->Find(word);
          if (word_str != "") {
            //os << state_times_[state] << '\t' << num_frames  << '\t' << arc_post << '\t' << word_str;
            os << state << '\t' << arc.nextstate << '\t' << arc_post << '\t' << word_str;
            for (int32 i=0;i <split_trans.size();i++ ) {
              if (word_str == split_trans[i]) {
                if (arc_post > post[i]) post[i]=arc_post;
              }
            }
          }
          //else os << state_times_[state] << '\t' << num_frames  << '\t' << arc_post << '\t' << word;
          else os << state << '\t' << arc.nextstate << '\t' << arc_post << '\t' << word_str;
        }

        os << std::endl;
        num_post++;
      }
    }
    BaseFloat post_avg = 0.0;
    for (int32 i=0;i <split_trans.size();i++ ) {
      os << split_trans[i] << '\t' << post[i] << '\n';
      post_avg = post_avg + post[i];
    }
    os << post_avg/double(split_trans.size()) << '\n';
    return num_post;
  }
 private:
  const CompactLattice &clat_;
  std::vector<double> alpha_;
  std::vector<double> beta_;
  std::vector<int32> state_times_;
  BaseFloat min_post_;
};
}

int main(int argc, char *argv[]) {
  try {
    using namespace kaldi;
    using namespace fst;

    typedef kaldi::int32 int32;
    typedef kaldi::int64 int64;

    const char *usage =
        "Reads in wav file(s) and simulates online decoding with neural nets\n"
        "(nnet3 setup), with optional iVector-based speaker adaptation and\n"
        "optional endpointing.  Note: some configuration values and inputs are\n"
        "set via config files whose filenames are passed as options\n"
        "\n"
        "Usage: online2-wav-nnet3-latgen-faster [options] <nnet3-in> <fst-in> "
        "<spk2utt-rspecifier> <wav-rspecifier> <lattice-wspecifier> <froce-decoding-output>\n"
        "The spk2utt-rspecifier can just be <utterance-id> <utterance-id> if\n"
        "you want to decode utterance by utterance.\n";

    ParseOptions po(usage);

    std::string word_syms_rxfilename;
    std::string force_decoding_trans;

    // feature_opts includes configuration for the iVector adaptation,
    // as well as the basic features.
    OnlineNnet2FeaturePipelineConfig feature_opts;
    nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
    LatticeFasterDecoderConfig decoder_opts;
    OnlineEndpointConfig endpoint_opts;

    BaseFloat chunk_length_secs = 0.18;
    bool do_endpointing = false;
    bool online = true;

    po.Register("chunk-length", &chunk_length_secs,
                "Length of chunk size in seconds, that we process.  Set to <= 0 "
                "to use all input in one chunk.");
    po.Register("word-symbol-table", &word_syms_rxfilename,
                "Symbol table for words [for debug output]");
    po.Register("do-endpointing", &do_endpointing,
                "If true, apply endpoint detection");
    po.Register("online", &online,
                "You can set this to false to disable online iVector estimation "
                "and have all the data for each utterance used, even at "
                "utterance start.  This is useful where you just want the best "
                "results and don't care about online operation.  Setting this to "
                "false has the same effect as setting "
                "--use-most-recent-ivector=true and --greedy-ivector-extractor=true "
                "in the file given to --ivector-extraction-config, and "
                "--chunk-length=-1.");
    po.Register("num-threads-startup", &g_num_threads,
                "Number of threads used when initializing iVector extractor.");

    feature_opts.Register(&po);
    decodable_opts.Register(&po);
    decoder_opts.Register(&po);
    endpoint_opts.Register(&po);


    po.Read(argc, argv);

    if (po.NumArgs() != 5) {
      po.PrintUsage();
      return 1;
    }

    std::string nnet3_rxfilename = po.GetArg(1),
        fst_rxfilename = po.GetArg(2),
        wav_rspecifier = po.GetArg(3),
        output_wxfilename = po.GetArg(4),
        target_trans = po.GetArg(5);

    std::string temp = "ark,s,cs:wav-copy scp,p:"; wav_rspecifier = temp + wav_rspecifier;
    temp = " ark:- | extend-wav-with-silence ark:- ark:- |"; wav_rspecifier = wav_rspecifier + temp;
    //spk2utt_rspecifier='ark:/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/exp_hires/split1/1/spk2utt'
    //wav_rspecifier='ark,s,cs:wav-copy scp,p:/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/exp_hires/split1/1/wav.scp ark:- | extend-wav-with-silence ark:- ark:- |'
    //clat_wspecifier='ark:|gzip -c >/media/kwon/DISK2/DEV/kaldi/egs/force_decode/s1/new_lm_mix/decode/lat.1.gz'

    OnlineNnet2FeaturePipelineInfo feature_info(feature_opts);

    if (!online) {
      feature_info.ivector_extractor_info.use_most_recent_ivector = true;
      feature_info.ivector_extractor_info.greedy_ivector_extractor = true;
      chunk_length_secs = -1.0;
    }

    TransitionModel trans_model;
    nnet3::AmNnetSimple am_nnet;
    {
      bool binary;
      Input ki(nnet3_rxfilename, &binary);
      trans_model.Read(ki.Stream(), binary);
      am_nnet.Read(ki.Stream(), binary);
      SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
      SetDropoutTestMode(true, &(am_nnet.GetNnet()));
      nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
    }

    // this object contains precomputed stuff that is used by all decodable
    // objects.  It takes a pointer to am_nnet because if it has iVectors it has
    // to modify the nnet to accept iVectors at intervals.
    nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts,
                                                        &am_nnet);


    fst::Fst<fst::StdArc> *decode_fst = ReadFstKaldiGeneric(fst_rxfilename);

    fst::SymbolTable *word_syms = NULL;
    if (word_syms_rxfilename != "")
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_rxfilename)))
        KALDI_ERR << "Could not read symbol table from file "
                  << word_syms_rxfilename;

    int32 num_done = 0;

    RandomAccessTableReader<WaveHolder> wav_reader(wav_rspecifier);

    OnlineTimingStats timing_stats;

    OnlineIvectorExtractorAdaptationState adaptation_state(
        feature_info.ivector_extractor_info);

    std::string utt = "utterance-id1";
    const WaveData &wave_data = wav_reader.Value(utt);
    // get the data for channel zero (if the signal is not mono, we only
    // take the first channel).
    SubVector<BaseFloat> data(wave_data.Data(), 0);

    OnlineNnet2FeaturePipeline feature_pipeline(feature_info);
    feature_pipeline.SetAdaptationState(adaptation_state);

    OnlineSilenceWeighting silence_weighting(trans_model,
                                            feature_info.silence_weighting_config,
                                            decodable_opts.frame_subsampling_factor);

    SingleUtteranceNnet3Decoder decoder(decoder_opts, trans_model,
                                        decodable_info,
                                        *decode_fst, &feature_pipeline);
    OnlineTimer decoding_timer(utt);

    BaseFloat samp_freq = wave_data.SampFreq();
    int32 chunk_length;
    if (chunk_length_secs > 0) {
      chunk_length = int32(samp_freq * chunk_length_secs);
      if (chunk_length == 0) chunk_length = 1;
    } else {
      chunk_length = std::numeric_limits<int32>::max();
    }

    int32 samp_offset = 0;
    std::vector<std::pair<int32, BaseFloat> > delta_weights;

    while (samp_offset < data.Dim()) {
      int32 samp_remaining = data.Dim() - samp_offset;
      int32 num_samp = chunk_length < samp_remaining ? chunk_length
                                                     : samp_remaining;

      SubVector<BaseFloat> wave_part(data, samp_offset, num_samp);
      feature_pipeline.AcceptWaveform(samp_freq, wave_part);

      samp_offset += num_samp;
      decoding_timer.WaitUntil(samp_offset / samp_freq);
      if (samp_offset == data.Dim()) {
        // no more input. flush out last frames
        feature_pipeline.InputFinished();
      }

      if (silence_weighting.Active() &&
          feature_pipeline.IvectorFeature() != NULL) {
        silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
        silence_weighting.GetDeltaWeights(feature_pipeline.NumFramesReady(),
                                          &delta_weights);
        feature_pipeline.IvectorFeature()->UpdateFrameWeights(delta_weights);
      }

      decoder.AdvanceDecoding();

      if (do_endpointing && decoder.EndpointDetected(endpoint_opts))
        break;
    }
    decoder.FinalizeDecoding();

    CompactLattice clat;
    bool end_of_utterance = true;
    decoder.GetLattice(end_of_utterance, &clat);

    //you can check what is saved
    //KALDI_LOG << "clat is ";
    //WriteCompactLattice(std::cerr, false, clat);

    kaldi::Output output(output_wxfilename, false);
    kaldi::BaseFloat acoustic_scale = 1.0, lm_scale = 1.0;
    kaldi::BaseFloat min_post = 0.0001;

    fst::ScaleLattice(fst::LatticeScale(lm_scale, acoustic_scale), &clat);
    kaldi::TopSortCompactLatticeIfNeeded(&clat);
    kaldi::ArcPosteriorComputer computer(
      clat, min_post);
    int32 num_post = computer.OutputPosteriors(target_trans, word_syms, output.Stream());
    KALDI_LOG << "num_post : " << num_post;

    num_done++;

    delete decode_fst;
    delete word_syms; // will delete if non-NULL.
    return (num_done != 0 ? 0 : 1);
  } catch(const std::exception& e) {
    std::cerr << e.what();
    return -1;
  }
} // main()
