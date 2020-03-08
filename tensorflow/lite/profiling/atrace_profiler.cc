/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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
#include "tensorflow/lite/profiling/atrace_profiler.h"

#include <dlfcn.h>

#include "absl/strings/str_cat.h"

namespace tflite {
namespace profiling {

ATraceProfiler::ATraceProfiler() {
  handle_ = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
  if (handle_) {
    // Use dlsym() to prevent crashes on devices running Android 5.1
    // (API level 22) or lower.
    atrace_is_enabled_ =
        reinterpret_cast<FpIsEnabled>(dlsym(handle_, "ATrace_isEnabled"));
    atrace_begin_section_ =
        reinterpret_cast<FpBeginSection>(dlsym(handle_, "ATrace_beginSection"));
    atrace_end_section_ =
        reinterpret_cast<FpEndSection>(dlsym(handle_, "ATrace_endSection"));

    if (!atrace_is_enabled_ || !atrace_begin_section_ || !atrace_end_section_) {
      dlclose(handle_);
      handle_ = nullptr;
    }
  }
}

ATraceProfiler::~ATraceProfiler() {
  if (handle_) {
    dlclose(handle_);
  }
}

uint32_t ATraceProfiler::BeginEvent(const char* tag, EventType event_type,
                                    uint32_t event_metadata,
                                    uint32_t event_subgraph_index) {
  if (handle_ && atrace_is_enabled_()) {
    // Note: When recording an OPERATOR_INVOKE_EVENT, we have recorded the op
    // name as tag and node index as event_metadata. See the macro
    // TFLITE_SCOPED_TAGGED_OPERATOR_PROFILE defined in
    // tensorflow/lite/core/api/profiler.h for details.
    // op_name@node_index/subgraph_index
    std::string trace_event_tag =
        absl::StrCat(tag, "@", event_metadata, "/", event_subgraph_index);
    atrace_begin_section_(trace_event_tag.c_str());
  }
  return 0;
}

void ATraceProfiler::EndEvent(uint32_t event_handle) {
  if (handle_) {
    atrace_end_section_();
  }
}

}  // namespace profiling
}  // namespace tflite
