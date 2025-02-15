/*
 * Copyright 2016 The Cartographer Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cartographer_ros/submap.h"

#include "absl/memory/memory.h"
#include "cartographer/common/port.h"
#include "cartographer/transform/transform.h"
#include "cartographer_ros/msg_conversion.h"
#include "cartographer_ros_msgs/StatusCode.h"
#include "cartographer_ros_msgs/SubmapQuery.h"

namespace cartographer_ros {

/**
 * @brief 获取SubmapTextures
 * 
 * @param[in] submap_id submap的id
 * @param[in] client SubmapQuery服务的客户端
 * @return std::unique_ptr<::cartographer::io::SubmapTextures> 
 */
std::unique_ptr<::cartographer::io::SubmapTextures> FetchSubmapTextures(
    const ::cartographer::mapping::SubmapId& submap_id,
    ros::ServiceClient* client) {
  ::cartographer_ros_msgs::SubmapQuery srv;
  srv.request.trajectory_id = submap_id.trajectory_id;
  srv.request.submap_index = submap_id.submap_index;
  // Step: 1 调用SubmapQuery服务
  if (!client->call(srv) ||
      srv.response.status.code != ::cartographer_ros_msgs::StatusCode::OK) {
    return nullptr;
  }
  if (srv.response.textures.empty()) {
    return nullptr;
  }
  // Step: 2 将srv.response.textures格式的数据 转换成io::SubmapTextures
  auto response = absl::make_unique<::cartographer::io::SubmapTextures>();
  response->version = srv.response.submap_version;
  for (const auto& texture : srv.response.textures) {
    // 获取压缩后的地图栅格数据
    const std::string compressed_cells(texture.cells.begin(),
                                       texture.cells.end());
    response->textures.emplace_back(::cartographer::io::SubmapTexture{
        // Step: 3 将地图栅格数据进行加压并填充到SubmapTexture中
        ::cartographer::io::UnpackTextureData(compressed_cells, texture.width,
                                              texture.height),
        texture.width, texture.height, texture.resolution,
        ToRigid3d(texture.slice_pose)});
  }
  return response;
}

}  // namespace cartographer_ros
