# 定义用户服务相关源文件
set(USER_SRC calluser.cc ../user.pb.cc)
# 生成用户服务的可执行程序
add_executable(consumer_user ${USER_SRC})
# 链接用户服务依赖库
target_link_libraries(consumer_user krpc protobuf)


# 定义好友服务相关源文件
set(FRIEND_SRC callfriend.cc ../friend.pb.cc)
# 生成好友服务的可执行程序
add_executable(consumer_friend ${FRIEND_SRC})
# 链接好友服务依赖库
target_link_libraries(consumer_friend krpc protobuf)


# 定义测试服务相关源文件
set(MEASURE_SRC measure.cc ../user.pb.cc)
# 生成测试服务的可执行程序
add_executable(measure ${MEASURE_SRC})
# 链接测试服务依赖库
target_link_libraries(measure krpc protobuf)