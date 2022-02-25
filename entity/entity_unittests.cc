// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/testing/testing.h"
#include "impeller/entity/entity.h"
#include "impeller/entity/entity_playground.h"
#include "impeller/geometry/path_builder.h"
#include "impeller/playground/playground.h"

namespace impeller {
namespace testing {

using EntityTest = EntityPlayground;

TEST_F(EntityTest, CanCreateEntity) {
  Entity entity;
  ASSERT_TRUE(entity.GetTransformation().IsIdentity());
}

TEST_F(EntityTest, CanDrawRect) {
  Entity entity;
  entity.SetPath(PathBuilder{}.AddRect({100, 100, 100, 100}).TakePath());
  entity.SetContents(SolidColorContents::Make(Color::Red()));
  ASSERT_TRUE(OpenPlaygroundHere(entity));
}

TEST_F(EntityTest, ThreeStrokesInOnePath) {
  Path path = PathBuilder{}
                  .MoveTo({100, 100})
                  .LineTo({100, 200})
                  .MoveTo({100, 300})
                  .LineTo({100, 400})
                  .MoveTo({100, 500})
                  .LineTo({100, 600})
                  .TakePath();

  Entity entity;
  entity.SetPath(path);
  auto contents = std::make_unique<SolidStrokeContents>();
  contents->SetColor(Color::Red());
  contents->SetStrokeSize(5.0);
  entity.SetContents(std::move(contents));
  ASSERT_TRUE(OpenPlaygroundHere(entity));
}

TEST_F(EntityTest, TriangleInsideASquare) {
  Path path = PathBuilder{}
                  .MoveTo({10, 10})
                  .LineTo({210, 10})
                  .LineTo({210, 210})
                  .LineTo({10, 210})
                  .Close()
                  .MoveTo({50, 50})
                  .LineTo({100, 50})
                  .LineTo({50, 150})
                  .Close()
                  .TakePath();

  Entity entity;
  entity.SetPath(path);
  auto contents = std::make_unique<SolidStrokeContents>();
  contents->SetColor(Color::Red());
  contents->SetStrokeSize(5.0);
  entity.SetContents(std::move(contents));
  ASSERT_TRUE(OpenPlaygroundHere(entity));
}

TEST_F(EntityTest, DISABLED_BadCubicCurveTest) {
  // Compare with https://fiddle.skia.org/c/b3625f26122c9de7afe7794fcf25ead3
  Path path =
      PathBuilder{}
          .MoveTo({237.164, 125.003})
          .CubicCurveTo({236.709, 125.184}, {236.262, 125.358},
                        {235.81, 125.538})
          .CubicCurveTo({235.413, 125.68}, {234.994, 125.832},
                        {234.592, 125.977})
          .CubicCurveTo({234.592, 125.977}, {234.591, 125.977},
                        {234.59, 125.977})
          .CubicCurveTo({222.206, 130.435}, {207.708, 135.753},
                        {192.381, 141.429})
          .CubicCurveTo({162.77, 151.336}, {122.17, 156.894}, {84.1123, 160})
          .Close()
          .TakePath();
  Entity entity;
  entity.SetPath(path);
  entity.SetContents(SolidColorContents::Make(Color::Red()));
  ASSERT_TRUE(OpenPlaygroundHere(entity));
}

TEST_F(EntityTest, DISABLED_BadCubicCurveAndOverlapTest) {
  // Compare with https://fiddle.skia.org/c/7a05a3e186c65a8dfb732f68020aae06
  Path path =
      PathBuilder{}
          .MoveTo({359.934, 96.6335})
          .CubicCurveTo({358.189, 96.7055}, {356.436, 96.7908},
                        {354.673, 96.8895})
          .CubicCurveTo({354.571, 96.8953}, {354.469, 96.9016},
                        {354.367, 96.9075})
          .CubicCurveTo({352.672, 97.0038}, {350.969, 97.113},
                        {349.259, 97.2355})
          .CubicCurveTo({349.048, 97.2506}, {348.836, 97.2678},
                        {348.625, 97.2834})
          .CubicCurveTo({347.019, 97.4014}, {345.407, 97.5299},
                        {343.789, 97.6722})
          .CubicCurveTo({343.428, 97.704}, {343.065, 97.7402},
                        {342.703, 97.7734})
          .CubicCurveTo({341.221, 97.9086}, {339.736, 98.0505},
                        {338.246, 98.207})
          .CubicCurveTo({337.702, 98.2642}, {337.156, 98.3292},
                        {336.612, 98.3894})
          .CubicCurveTo({335.284, 98.5356}, {333.956, 98.6837},
                        {332.623, 98.8476})
          .CubicCurveTo({332.495, 98.8635}, {332.366, 98.8818},
                        {332.237, 98.8982})
          .LineTo({332.237, 102.601})
          .LineTo({321.778, 102.601})
          .LineTo({321.778, 100.382})
          .CubicCurveTo({321.572, 100.413}, {321.367, 100.442},
                        {321.161, 100.476})
          .CubicCurveTo({319.22, 100.79}, {317.277, 101.123},
                        {315.332, 101.479})
          .CubicCurveTo({315.322, 101.481}, {315.311, 101.482},
                        {315.301, 101.484})
          .LineTo({310.017, 105.94})
          .LineTo({309.779, 105.427})
          .LineTo({314.403, 101.651})
          .CubicCurveTo({314.391, 101.653}, {314.379, 101.656},
                        {314.368, 101.658})
          .CubicCurveTo({312.528, 102.001}, {310.687, 102.366},
                        {308.846, 102.748})
          .CubicCurveTo({307.85, 102.955}, {306.855, 103.182}, {305.859, 103.4})
          .CubicCurveTo({305.048, 103.579}, {304.236, 103.75},
                        {303.425, 103.936})
          .LineTo({299.105, 107.578})
          .LineTo({298.867, 107.065})
          .LineTo({302.394, 104.185})
          .LineTo({302.412, 104.171})
          .CubicCurveTo({301.388, 104.409}, {300.366, 104.67},
                        {299.344, 104.921})
          .CubicCurveTo({298.618, 105.1}, {297.89, 105.269}, {297.165, 105.455})
          .CubicCurveTo({295.262, 105.94}, {293.36, 106.445},
                        {291.462, 106.979})
          .CubicCurveTo({291.132, 107.072}, {290.802, 107.163},
                        {290.471, 107.257})
          .CubicCurveTo({289.463, 107.544}, {288.455, 107.839},
                        {287.449, 108.139})
          .CubicCurveTo({286.476, 108.431}, {285.506, 108.73},
                        {284.536, 109.035})
          .CubicCurveTo({283.674, 109.304}, {282.812, 109.579},
                        {281.952, 109.859})
          .CubicCurveTo({281.177, 110.112}, {280.406, 110.377},
                        {279.633, 110.638})
          .CubicCurveTo({278.458, 111.037}, {277.256, 111.449},
                        {276.803, 111.607})
          .CubicCurveTo({276.76, 111.622}, {276.716, 111.637},
                        {276.672, 111.653})
          .CubicCurveTo({275.017, 112.239}, {273.365, 112.836},
                        {271.721, 113.463})
          .LineTo({271.717, 113.449})
          .CubicCurveTo({271.496, 113.496}, {271.238, 113.559},
                        {270.963, 113.628})
          .CubicCurveTo({270.893, 113.645}, {270.822, 113.663},
                        {270.748, 113.682})
          .CubicCurveTo({270.468, 113.755}, {270.169, 113.834},
                        {269.839, 113.926})
          .CubicCurveTo({269.789, 113.94}, {269.732, 113.957},
                        {269.681, 113.972})
          .CubicCurveTo({269.391, 114.053}, {269.081, 114.143},
                        {268.756, 114.239})
          .CubicCurveTo({268.628, 114.276}, {268.5, 114.314},
                        {268.367, 114.354})
          .CubicCurveTo({268.172, 114.412}, {267.959, 114.478},
                        {267.752, 114.54})
          .CubicCurveTo({263.349, 115.964}, {258.058, 117.695},
                        {253.564, 119.252})
          .CubicCurveTo({253.556, 119.255}, {253.547, 119.258},
                        {253.538, 119.261})
          .CubicCurveTo({251.844, 119.849}, {250.056, 120.474},
                        {248.189, 121.131})
          .CubicCurveTo({248, 121.197}, {247.812, 121.264}, {247.621, 121.331})
          .CubicCurveTo({247.079, 121.522}, {246.531, 121.715},
                        {245.975, 121.912})
          .CubicCurveTo({245.554, 122.06}, {245.126, 122.212},
                        {244.698, 122.364})
          .CubicCurveTo({244.071, 122.586}, {243.437, 122.811},
                        {242.794, 123.04})
          .CubicCurveTo({242.189, 123.255}, {241.58, 123.472},
                        {240.961, 123.693})
          .CubicCurveTo({240.659, 123.801}, {240.357, 123.909},
                        {240.052, 124.018})
          .CubicCurveTo({239.12, 124.351}, {238.18, 124.687}, {237.22, 125.032})
          .LineTo({237.164, 125.003})
          .CubicCurveTo({236.709, 125.184}, {236.262, 125.358},
                        {235.81, 125.538})
          .CubicCurveTo({235.413, 125.68}, {234.994, 125.832},
                        {234.592, 125.977})
          .CubicCurveTo({234.592, 125.977}, {234.591, 125.977},
                        {234.59, 125.977})
          .CubicCurveTo({222.206, 130.435}, {207.708, 135.753},
                        {192.381, 141.429})
          .CubicCurveTo({162.77, 151.336}, {122.17, 156.894}, {84.1123, 160})
          .LineTo({360, 160})
          .LineTo({360, 119.256})
          .LineTo({360, 106.332})
          .LineTo({360, 96.6307})
          .CubicCurveTo({359.978, 96.6317}, {359.956, 96.6326},
                        {359.934, 96.6335})
          .Close()
          .MoveTo({337.336, 124.143})
          .CubicCurveTo({337.274, 122.359}, {338.903, 121.511},
                        {338.903, 121.511})
          .CubicCurveTo({338.903, 121.511}, {338.96, 123.303},
                        {337.336, 124.143})
          .Close()
          .MoveTo({340.082, 121.849})
          .CubicCurveTo({340.074, 121.917}, {340.062, 121.992},
                        {340.046, 122.075})
          .CubicCurveTo({340.039, 122.109}, {340.031, 122.142},
                        {340.023, 122.177})
          .CubicCurveTo({340.005, 122.26}, {339.98, 122.346},
                        {339.952, 122.437})
          .CubicCurveTo({339.941, 122.473}, {339.931, 122.507},
                        {339.918, 122.544})
          .CubicCurveTo({339.873, 122.672}, {339.819, 122.804},
                        {339.75, 122.938})
          .CubicCurveTo({339.747, 122.944}, {339.743, 122.949},
                        {339.74, 122.955})
          .CubicCurveTo({339.674, 123.08}, {339.593, 123.205},
                        {339.501, 123.328})
          .CubicCurveTo({339.473, 123.366}, {339.441, 123.401},
                        {339.41, 123.438})
          .CubicCurveTo({339.332, 123.534}, {339.243, 123.625},
                        {339.145, 123.714})
          .CubicCurveTo({339.105, 123.75}, {339.068, 123.786},
                        {339.025, 123.821})
          .CubicCurveTo({338.881, 123.937}, {338.724, 124.048},
                        {338.539, 124.143})
          .CubicCurveTo({338.532, 123.959}, {338.554, 123.79},
                        {338.58, 123.626})
          .CubicCurveTo({338.58, 123.625}, {338.58, 123.625}, {338.58, 123.625})
          .CubicCurveTo({338.607, 123.455}, {338.65, 123.299},
                        {338.704, 123.151})
          .CubicCurveTo({338.708, 123.14}, {338.71, 123.127},
                        {338.714, 123.117})
          .CubicCurveTo({338.769, 122.971}, {338.833, 122.838},
                        {338.905, 122.712})
          .CubicCurveTo({338.911, 122.702}, {338.916, 122.69200000000001},
                        {338.922, 122.682})
          .CubicCurveTo({338.996, 122.557}, {339.072, 122.444},
                        {339.155, 122.34})
          .CubicCurveTo({339.161, 122.333}, {339.166, 122.326},
                        {339.172, 122.319})
          .CubicCurveTo({339.256, 122.215}, {339.339, 122.12},
                        {339.425, 122.037})
          .CubicCurveTo({339.428, 122.033}, {339.431, 122.03},
                        {339.435, 122.027})
          .CubicCurveTo({339.785, 121.687}, {340.106, 121.511},
                        {340.106, 121.511})
          .CubicCurveTo({340.106, 121.511}, {340.107, 121.645},
                        {340.082, 121.849})
          .Close()
          .MoveTo({340.678, 113.245})
          .CubicCurveTo({340.594, 113.488}, {340.356, 113.655},
                        {340.135, 113.775})
          .CubicCurveTo({339.817, 113.948}, {339.465, 114.059},
                        {339.115, 114.151})
          .CubicCurveTo({338.251, 114.379}, {337.34, 114.516},
                        {336.448, 114.516})
          .CubicCurveTo({335.761, 114.516}, {335.072, 114.527},
                        {334.384, 114.513})
          .CubicCurveTo({334.125, 114.508}, {333.862, 114.462},
                        {333.605, 114.424})
          .CubicCurveTo({332.865, 114.318}, {332.096, 114.184},
                        {331.41, 113.883})
          .CubicCurveTo({330.979, 113.695}, {330.442, 113.34},
                        {330.672, 112.813})
          .CubicCurveTo({331.135, 111.755}, {333.219, 112.946},
                        {334.526, 113.833})
          .CubicCurveTo({334.54, 113.816}, {334.554, 113.8}, {334.569, 113.784})
          .CubicCurveTo({333.38, 112.708}, {331.749, 110.985},
                        {332.76, 110.402})
          .CubicCurveTo({333.769, 109.82}, {334.713, 111.93},
                        {335.228, 113.395})
          .CubicCurveTo({334.915, 111.889}, {334.59, 109.636},
                        {335.661, 109.592})
          .CubicCurveTo({336.733, 109.636}, {336.408, 111.889},
                        {336.07, 113.389})
          .CubicCurveTo({336.609, 111.93}, {337.553, 109.82},
                        {338.563, 110.402})
          .CubicCurveTo({339.574, 110.984}, {337.942, 112.708},
                        {336.753, 113.784})
          .CubicCurveTo({336.768, 113.8}, {336.782, 113.816},
                        {336.796, 113.833})
          .CubicCurveTo({338.104, 112.946}, {340.187, 111.755},
                        {340.65, 112.813})
          .CubicCurveTo({340.71, 112.95}, {340.728, 113.102},
                        {340.678, 113.245})
          .Close()
          .MoveTo({346.357, 106.771})
          .CubicCurveTo({346.295, 104.987}, {347.924, 104.139},
                        {347.924, 104.139})
          .CubicCurveTo({347.924, 104.139}, {347.982, 105.931},
                        {346.357, 106.771})
          .Close()
          .MoveTo({347.56, 106.771})
          .CubicCurveTo({347.498, 104.987}, {349.127, 104.139},
                        {349.127, 104.139})
          .CubicCurveTo({349.127, 104.139}, {349.185, 105.931},
                        {347.56, 106.771})
          .Close()
          .TakePath();
  Entity entity;
  entity.SetPath(path);
  entity.SetContents(SolidColorContents::Make(Color::Red()));
  ASSERT_TRUE(OpenPlaygroundHere(entity));
}

}  // namespace testing
}  // namespace impeller
