/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef TENSORFLOW_LITE_TOOLS_ACCURACY_ILSVRC_IMAGENET_MODEL_EVALUATOR_H_
#define TENSORFLOW_LITE_TOOLS_ACCURACY_ILSVRC_IMAGENET_MODEL_EVALUATOR_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/tools/evaluation/evaluation_delegate_provider.h"
#include "tensorflow/lite/tools/evaluation/proto/evaluation_stages.pb.h"

namespace tensorflow {
namespace metrics {

// Evaluates models accuracy for ILSVRC dataset.
//
// Generates the top-1, top-k accuracy counts where k is
// controlled by |num_ranks|.
// Usage:
// ModelInfo model_info = ..
// ImagenetModelEvaluator::Params params;
// .. set params to image, label, output label and model file path..
// SomeObserver observer;
// ImagenetModelEvaluator evaluator(model_info, params);
// evaluator.AddObserver(&observer);
// TF_CHECK_OK(evaluator.EvaluateModel());
class ImagenetModelEvaluator {
 public:
  struct Params {
    // Path to ground truth images.
    std::string ground_truth_images_path;

    // Path to labels file for ground truth image.
    // This file should be generated with the scripts.
    std::string ground_truth_labels_path;

    // This is word labels generated by the model. The category
    // indices of output probabilities generated by the model maybe different
    // from the indices in the imagenet dataset.
    std::string model_output_labels_path;

    // Path to the model file.
    std::string model_file_path;

    // Path to black list file. 1762 images were blacklisted from
    // original ILSVRC dataset. This black list file is present in
    // ILSVRC2014 devkit. Please refer to readme.txt of the ILSVRC2014
    // devkit for details.
    // This file is a list of image indices in a sorted order.
    std::string blacklist_file_path;

    // Delegate used to perform inference (if available).
    // Valid values: 'nnapi', 'gpu', 'hexagon', 'xnnpack'
    std::string delegate;

    // The maximum number of images to calculate accuracy.
    // 0 means all images, a positive number means only the specified
    // number of images.
    int number_of_images = 0;

    // Number of ranks, top K.
    int num_ranks = 10;

    // Number of interpreter threads.
    int num_interpreter_threads = 1;

    // allow fp16
    bool allow_fp16 = false;
  };

  // An evaluation observer.
  // Observers can be called from multiple threads and need to be thread safe.
  class Observer {
   public:
    Observer() = default;
    Observer(const Observer&) = delete;
    Observer& operator=(const Observer&) = delete;

    Observer(const Observer&&) = delete;
    Observer& operator=(const Observer&&) = delete;

    // Called on start of evaluation.
    // `shard_id_image_count_map` map from shard id to image count.
    virtual void OnEvaluationStart(
        const std::unordered_map<uint64_t, int>& shard_id_image_count_map) = 0;

    // Called when evaluation was complete for `image`.
    virtual void OnSingleImageEvaluationComplete(
        uint64_t shard_id,
        const tflite::evaluation::TopkAccuracyEvalMetrics& metrics,
        const std::string& image) = 0;

    virtual ~Observer() = default;
  };

  ImagenetModelEvaluator(const Params& params, const int num_threads)
      : params_(params), num_threads_(num_threads) {}

  // Factory method to create the evaluator by parsing command line arguments.
  // Note argc and argv will be updated accordingly as matching arguments will
  // be removed in argv.
  static TfLiteStatus Create(
      int* argc, char* argv[], int num_threads,
      std::unique_ptr<ImagenetModelEvaluator>* evaluator);

  // Adds an observer that can observe evaluation events..
  void AddObserver(Observer* observer) { observers_.push_back(observer); }

  const Params& params() const { return params_; }

  // Evaluates the provided model over the dataset.
  TfLiteStatus EvaluateModel(const tflite::evaluation::DelegateProviders*
                                 delegate_providers = nullptr) const;

 private:
  const Params params_;
  const int num_threads_;
  std::vector<Observer*> observers_;
};

}  // namespace metrics
}  // namespace tensorflow
#endif  // TENSORFLOW_LITE_TOOLS_ACCURACY_ILSVRC_IMAGENET_MODEL_EVALUATOR_H_
