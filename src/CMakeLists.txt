aux_source_directory(. SRC_LIST)
add_library(krpc ${SRC_LIST})
target_link_libraries(krpc muduo_net muduo_base pthread zookeeper_mt)