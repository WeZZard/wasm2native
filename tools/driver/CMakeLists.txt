add_w2n_host_tool(w2n-frontend
  driver.cpp
)

target_link_libraries(w2n-frontend
                      PRIVATE
                        w2nDriverTool)

set_target_properties(w2n-frontend PROPERTIES ENABLE_EXPORTS 1)
