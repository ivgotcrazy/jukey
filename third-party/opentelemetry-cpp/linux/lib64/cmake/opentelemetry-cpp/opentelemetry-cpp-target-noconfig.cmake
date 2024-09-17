#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "opentelemetry-cpp::proto" for configuration ""
set_property(TARGET opentelemetry-cpp::proto APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::proto PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_NOCONFIG "CXX"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_proto.a"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::proto )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::proto "${_IMPORT_PREFIX}/lib64/libopentelemetry_proto.a" )

# Import target "opentelemetry-cpp::common" for configuration ""
set_property(TARGET opentelemetry-cpp::common APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::common PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_common.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_common.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::common )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::common "${_IMPORT_PREFIX}/lib64/libopentelemetry_common.so" )

# Import target "opentelemetry-cpp::trace" for configuration ""
set_property(TARGET opentelemetry-cpp::trace APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::trace PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_trace.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_trace.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::trace )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::trace "${_IMPORT_PREFIX}/lib64/libopentelemetry_trace.so" )

# Import target "opentelemetry-cpp::metrics" for configuration ""
set_property(TARGET opentelemetry-cpp::metrics APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::metrics PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_metrics.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_metrics.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::metrics )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::metrics "${_IMPORT_PREFIX}/lib64/libopentelemetry_metrics.so" )

# Import target "opentelemetry-cpp::version" for configuration ""
set_property(TARGET opentelemetry-cpp::version APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::version PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_version.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_version.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::version )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::version "${_IMPORT_PREFIX}/lib64/libopentelemetry_version.so" )

# Import target "opentelemetry-cpp::resources" for configuration ""
set_property(TARGET opentelemetry-cpp::resources APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::resources PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_resources.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_resources.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::resources )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::resources "${_IMPORT_PREFIX}/lib64/libopentelemetry_resources.so" )

# Import target "opentelemetry-cpp::http_client_curl" for configuration ""
set_property(TARGET opentelemetry-cpp::http_client_curl APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::http_client_curl PROPERTIES
  IMPORTED_LINK_DEPENDENT_LIBRARIES_NOCONFIG "CURL::libcurl"
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_http_client_curl.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_http_client_curl.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::http_client_curl )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::http_client_curl "${_IMPORT_PREFIX}/lib64/libopentelemetry_http_client_curl.so" )

# Import target "opentelemetry-cpp::otlp_recordable" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_recordable APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_recordable PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_otlp_recordable.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_otlp_recordable.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_recordable )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_recordable "${_IMPORT_PREFIX}/lib64/libopentelemetry_otlp_recordable.so" )

# Import target "opentelemetry-cpp::otlp_grpc_client" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_grpc_client APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_grpc_client PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc_client.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_otlp_grpc_client.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_grpc_client )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_grpc_client "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc_client.so" )

# Import target "opentelemetry-cpp::otlp_grpc_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_grpc_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_grpc_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_otlp_grpc.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_grpc_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_grpc_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc.so" )

# Import target "opentelemetry-cpp::otlp_grpc_log_record_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_grpc_log_record_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_grpc_log_record_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc_log.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_otlp_grpc_log.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_grpc_log_record_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_grpc_log_record_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc_log.so" )

# Import target "opentelemetry-cpp::otlp_grpc_metrics_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_grpc_metrics_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_grpc_metrics_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc_metrics.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_otlp_grpc_metrics.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_grpc_metrics_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_grpc_metrics_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_grpc_metrics.so" )

# Import target "opentelemetry-cpp::otlp_http_client" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_http_client APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_http_client PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_http_client.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_otlp_http_client.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_http_client )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_http_client "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_http_client.so" )

# Import target "opentelemetry-cpp::otlp_http_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_http_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_http_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_http.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_otlp_http.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_http_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_http_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_http.so" )

# Import target "opentelemetry-cpp::otlp_http_metric_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::otlp_http_metric_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::otlp_http_metric_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_http_metric.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_otlp_http_metric.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_http_metric_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_http_metric_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_otlp_http_metric.so" )

# Import target "opentelemetry-cpp::ostream_span_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::ostream_span_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::ostream_span_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_ostream_span.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_ostream_span.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::ostream_span_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::ostream_span_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_ostream_span.so" )

# Import target "opentelemetry-cpp::ostream_metrics_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::ostream_metrics_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::ostream_metrics_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_ostream_metrics.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_ostream_metrics.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::ostream_metrics_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::ostream_metrics_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_ostream_metrics.so" )

# Import target "opentelemetry-cpp::in_memory_span_exporter" for configuration ""
set_property(TARGET opentelemetry-cpp::in_memory_span_exporter APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(opentelemetry-cpp::in_memory_span_exporter PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_in_memory.so"
  IMPORTED_SONAME_NOCONFIG "libopentelemetry_exporter_in_memory.so"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::in_memory_span_exporter )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::in_memory_span_exporter "${_IMPORT_PREFIX}/lib64/libopentelemetry_exporter_in_memory.so" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
