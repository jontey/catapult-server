cmake_minimum_required(VERSION 3.2)

catapult_define_extension_src(sync)
target_link_libraries(catapult.sync catapult.consumers)
