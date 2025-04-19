# 定义用户服务相关源文件
set(USER_SRC user.cc ../user.pb.cc)
# 定义好友服务相关源文件
set(FRIEND_SRC friend.cc ../friend.pb.cc)

# 生成用户服务的可执行程序
add_executable(provider_user ${USER_SRC})
# 链接用户服务依赖库
target_link_libraries(provider_user krpc protobuf)

# 生成好友服务的可执行程序
add_executable(provider_friend ${FRIEND_SRC})
# 链接好友服务依赖库
target_link_libraries(provider_friend krpc protobuf)