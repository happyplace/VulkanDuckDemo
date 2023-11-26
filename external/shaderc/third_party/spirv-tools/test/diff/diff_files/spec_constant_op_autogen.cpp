// GENERATED FILE - DO NOT EDIT.
// Generated by generate_tests.py
//
// Copyright (c) 2022 Google LLC.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "../diff_test_utils.h"

#include "gtest/gtest.h"

namespace spvtools {
namespace diff {
namespace {

// Tests OpSpecConstantOp matching.
constexpr char kSrc[] = R"(               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %1 "main"
               OpExecutionMode %1 LocalSize 1 1 1
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %4 = OpTypeInt 32 0
          %5 = OpTypeVector %4 3
          %6 = OpConstant %4 1
          %7 = OpSpecConstantComposite %5 %6 %6 %6
          %8 = OpSpecConstantOp %4 CompositeExtract %7 2
          %9 = OpSpecConstantOp %4 CompositeExtract %7 1
         %10 = OpSpecConstantOp %4 CompositeExtract %7 0
          %1 = OpFunction %2 None %3
         %11 = OpLabel
               OpReturn
               OpFunctionEnd)";
constexpr char kDst[] = R"(               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %1 "main"
               OpExecutionMode %1 LocalSize 1 1 1
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %4 = OpTypeInt 32 0
          %5 = OpTypeVector %4 3
          %6 = OpConstant %4 1
          %7 = OpSpecConstantComposite %5 %6 %6 %6
          %8 = OpSpecConstantOp %4 CompositeExtract %7 2
          %9 = OpSpecConstantOp %4 CompositeExtract %7 3
         %10 = OpSpecConstantOp %4 IMul %8 %8
          %1 = OpFunction %2 None %3
         %11 = OpLabel
               OpReturn
               OpFunctionEnd
)";

TEST(DiffTest, SpecConstantOp) {
  constexpr char kDiff[] = R"( ; SPIR-V
 ; Version: 1.6
 ; Generator: Khronos SPIR-V Tools Assembler; 0
-; Bound: 12
+; Bound: 14
 ; Schema: 0
 OpCapability Shader
 OpMemoryModel Logical GLSL450
 OpEntryPoint GLCompute %1 "main"
 OpExecutionMode %1 LocalSize 1 1 1
 %2 = OpTypeVoid
 %3 = OpTypeFunction %2
 %4 = OpTypeInt 32 0
 %5 = OpTypeVector %4 3
 %6 = OpConstant %4 1
 %7 = OpSpecConstantComposite %5 %6 %6 %6
 %8 = OpSpecConstantOp %4 CompositeExtract %7 2
-%9 = OpSpecConstantOp %4 CompositeExtract %7 1
-%10 = OpSpecConstantOp %4 CompositeExtract %7 0
+%12 = OpSpecConstantOp %4 CompositeExtract %7 3
+%13 = OpSpecConstantOp %4 IMul %8 %8
 %1 = OpFunction %2 None %3
 %11 = OpLabel
 OpReturn
 OpFunctionEnd
)";
  Options options;
  DoStringDiffTest(kSrc, kDst, kDiff, options);
}

TEST(DiffTest, SpecConstantOpNoDebug) {
  constexpr char kSrcNoDebug[] = R"(               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %1 "main"
               OpExecutionMode %1 LocalSize 1 1 1
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %4 = OpTypeInt 32 0
          %5 = OpTypeVector %4 3
          %6 = OpConstant %4 1
          %7 = OpSpecConstantComposite %5 %6 %6 %6
          %8 = OpSpecConstantOp %4 CompositeExtract %7 2
          %9 = OpSpecConstantOp %4 CompositeExtract %7 1
         %10 = OpSpecConstantOp %4 CompositeExtract %7 0
          %1 = OpFunction %2 None %3
         %11 = OpLabel
               OpReturn
               OpFunctionEnd)";
  constexpr char kDstNoDebug[] = R"(               OpCapability Shader
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %1 "main"
               OpExecutionMode %1 LocalSize 1 1 1
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %4 = OpTypeInt 32 0
          %5 = OpTypeVector %4 3
          %6 = OpConstant %4 1
          %7 = OpSpecConstantComposite %5 %6 %6 %6
          %8 = OpSpecConstantOp %4 CompositeExtract %7 2
          %9 = OpSpecConstantOp %4 CompositeExtract %7 3
         %10 = OpSpecConstantOp %4 IMul %8 %8
          %1 = OpFunction %2 None %3
         %11 = OpLabel
               OpReturn
               OpFunctionEnd
)";
  constexpr char kDiff[] = R"( ; SPIR-V
 ; Version: 1.6
 ; Generator: Khronos SPIR-V Tools Assembler; 0
-; Bound: 12
+; Bound: 14
 ; Schema: 0
 OpCapability Shader
 OpMemoryModel Logical GLSL450
 OpEntryPoint GLCompute %1 "main"
 OpExecutionMode %1 LocalSize 1 1 1
 %2 = OpTypeVoid
 %3 = OpTypeFunction %2
 %4 = OpTypeInt 32 0
 %5 = OpTypeVector %4 3
 %6 = OpConstant %4 1
 %7 = OpSpecConstantComposite %5 %6 %6 %6
 %8 = OpSpecConstantOp %4 CompositeExtract %7 2
-%9 = OpSpecConstantOp %4 CompositeExtract %7 1
-%10 = OpSpecConstantOp %4 CompositeExtract %7 0
+%12 = OpSpecConstantOp %4 CompositeExtract %7 3
+%13 = OpSpecConstantOp %4 IMul %8 %8
 %1 = OpFunction %2 None %3
 %11 = OpLabel
 OpReturn
 OpFunctionEnd
)";
  Options options;
  DoStringDiffTest(kSrcNoDebug, kDstNoDebug, kDiff, options);
}

}  // namespace
}  // namespace diff
}  // namespace spvtools